//
// Created by 刘文景 on 2021/7/3.
//

#include "ljson.h"
#include <cassert> // assert()
#include <cmath>   // HUGE_VAL
#include <algorithm>

#ifndef LJSON_PARSE_STACK_INIT_SIZE
#define LJSON_PARSE_STACK_INIT_SIZE 256
#endif

namespace ljson {

namespace {

bool isdigit(const char& ch) { return ch >= '0' && ch <= '9'; }

bool isdigit1to9(const char& ch) { return ch >= '1' && ch <= '9'; }

std::shared_ptr<ljson_value> create_ljson_value_by_type(LJSON_TYPE type) {
  switch (type) {
    case LJSON_NULL:
      return ljson_null::create();
    case LJSON_TRUE:
      return ljson_true::create();
    case LJSON_FALSE:
      return ljson_false::create();
    case LJSON_STRING:
      return ljson_string::create();
    case LJSON_ARRAY:
      return ljson_array::create();
    case LJSON_OBJECT:
      return ljson_objects::create();
    default:
      return nullptr;
  }
}

}

struct ljson_context {
public:
  explicit ljson_context(const char* json) : json_(json), stack_(nullptr), size_(0), top_(0) {}
  ~ljson_context();
  void parse_whitespace();
  std::shared_ptr<ljson_value> parse_literal(const char* literal, LJSON_TYPE type, int *ret);
  std::shared_ptr<ljson_value> parse_number(int *ret);
  std::shared_ptr<ljson_value> parse_value(int *ret);
  void encode_uft8(unsigned u);
  LJSON_STATE parse_string_raw(char **str, size_t *len);
  std::shared_ptr<ljson_value> parse_string(int *ret);
  std::shared_ptr<ljson_value> parse_array(int *ret);
  std::shared_ptr<ljson_value> parse_object(int *ret);

public:
  static const char* parse_hex4(const char* p, unsigned *u);
  void put_char(char ch);
  void *push(size_t size);
  void *pop(size_t size);
  void push_buffer(std::shared_ptr<void> value);

  template<typename T>
  std::vector<std::shared_ptr<T>> pop_buffer(size_t size);

public:
  const char *json_;
  char *stack_;
  size_t size_, top_;
  std::vector<std::shared_ptr<void>> array_buffer_;
  /*
   * @ch: next expected character
   * @noted: exit if next character is not ch
   */
  void expect_next(const char &ch);
};

ljson_context::~ljson_context() {
  assert(top_ == 0);
  if (stack_ != nullptr)
    free(stack_);
}

void ljson_context::put_char(char ch) {
  auto address = static_cast<char *>(push(sizeof(char)));
  *address = ch;
}

void * ljson_context::push(size_t size) {
  assert(size > 0);
  void *ret = nullptr;
  if (top_ + size >= size_) {
    if (size_ == 0)
      size_ = LJSON_PARSE_STACK_INIT_SIZE;
    while (top_ + size >= size_)
      size_ += size_ >> 1; /* size_ * 1.5 */
    stack_ = static_cast<char *>(realloc(stack_, size_));
  }
  ret = stack_ + top_;
  top_ += size;
  return ret;
}

void * ljson_context::pop(size_t size) {
  assert(top_ >= size);
  return stack_ + (top_ -= size);
}

void ljson_context::push_buffer(std::shared_ptr<void> value) {
  array_buffer_.push_back(std::move(value));
}

template <typename T>
std::vector<std::shared_ptr<T> > ljson_context::pop_buffer(size_t size) {
  assert(size <= array_buffer_.size());
  std::vector<std::shared_ptr<T>> pops;
  while (size > 0) {
    size--;
    auto back = std::static_pointer_cast<T>(array_buffer_.back());
    pops.push_back(back);
    array_buffer_.pop_back();
  }
  // notice that reverse is must, we need to keep the order as elements were pushed.
  std::reverse(pops.begin(), pops.end());
  return pops;
}

void ljson_context::expect_next(const char &ch) {
  assert(*json_ == ch);
  json_++;
}

void ljson_context::parse_whitespace() {
  const char* p = json_;
  while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
    p++;
  json_ = p;
}

std::shared_ptr<ljson_value> ljson_context::parse_literal(const char *literal, LJSON_TYPE type, int *ret) {
  size_t i = 0;
  expect_next(literal[0]);
  *ret = LJSON_PARSE_OK;
  for (; literal[i+1]; i++)
    if (json_[i] != literal[i+1]) {
      *ret = LJSON_PARSE_INVALID_VALUE;
      return nullptr;
    }
  json_ += i;
  return create_ljson_value_by_type(type);
}

std::shared_ptr<ljson_value> ljson_context::parse_number(int *ret) {
  const char* p = json_;
  *ret = LJSON_PARSE_OK;
  // process sign character
  if (*p == '-') p++;
  // process only one '0'
  if (*p == '0') p++;
  else {
    if (!isdigit1to9(*p)) {
      *ret = LJSON_PARSE_INVALID_VALUE;
      return nullptr;
    }
    // process all digits
    for (p++; isdigit(*p); ++p);
  }
  if (*p == '.') {
    p++;
    if (!isdigit(*p)) {
      *ret = LJSON_PARSE_INVALID_VALUE;
      return nullptr;
    }
    // process all digits
    for (p++; isdigit(*p); ++p);
  }
  if (*p == 'e' || *p == 'E') {
    p++;
    if (*p == '+' || *p == '-') p++;
    if (!isdigit(*p)) {
      *ret = LJSON_PARSE_INVALID_VALUE;
      return nullptr;
    }
    // process all digits
    for (p++; isdigit(*p); ++p);
  }
  errno = 0;
  auto number = strtod(json_, nullptr);
  // in set_number we set the type to LJSON_NUMBER
  auto value = ljson_number::create(number);
  if (errno == ERANGE && (number == HUGE_VAL || number == -HUGE_VAL)) {
    // we have to set back value type to LSJON_NULL
    *ret = LJSON_PARSE_NUMBER_TOO_BIG;
    return nullptr;
  }
  json_ = p;
  return value;
}

// reference: https://zhuanlan.zhihu.com/p/22731540
const char* ljson_context::parse_hex4(const char *p, unsigned int *u) {
  *u = 0;
  for (int i = 0; i < 4; ++i) {
    char ch = *p++;
    *u <<= 4;
    if (ch >= '0' && ch <= '9') *u |= ch - '0';
    else if (ch >= 'A' && ch <= 'F') *u |= ch - ('A' - 10);
    else if (ch >= 'a' && ch <= 'f') *u |= ch - ('a' - 10);
    else return nullptr;
  }
  return p;
}

void ljson_context::encode_uft8(unsigned int u) {
  if (u <= 0x7F)
    put_char(u & 0xFF);
  else if (u <= 0x7FF) {
    put_char(0xC0 | ((u >> 6) & 0xFF));
    put_char(0x80 | ( u & 0x3F));
  }
  else if (u <= 0xFFFF) {
    put_char(0xE0 | ((u >> 12) & 0xFF));
    put_char(0x80 | ((u >>  6) & 0x3F));
    put_char(0x80 | ( u        & 0x3F));
  }
  else {
    assert(u <= 0x10FFFF);
    put_char(0xF0 | ((u >> 18) & 0xFF));
    put_char(0x80 | ((u >> 12) & 0x3F));
    put_char(0x80 | ((u >>  6) & 0x3F));
    put_char(0x80 | ( u        & 0x3F));
  }
}

#define STRING_ERROR(ret) do { top_ = head; return ret; } while(0)

LJSON_STATE ljson_context::parse_string_raw(char **str, size_t *len) {
  size_t head = top_;
  expect_next('\"');
  const char* p = json_;
  for (;;) {
    char ch = *p++;
    switch (ch) {
      case '\"':
        // whole string has been processed.
        *len = top_ - head;
        *str = static_cast<char*>(pop(*len));
        json_ = p;
        return LJSON_PARSE_OK;
      case '\\':
        switch (*p++) {
          case '\"': put_char( '\"'); break;
          case '\\': put_char('\\'); break;
          case '/':  put_char('/' ); break;
          case 'b':  put_char( '\b'); break;
          case 'f':  put_char('\f'); break;
          case 'n':  put_char('\n'); break;
          case 'r':  put_char('\r'); break;
          case 't':  put_char('\t'); break;
          case 'u':
            unsigned u, u2;
            if (!(p = parse_hex4(p, &u)))
              STRING_ERROR(LJSON_PARSE_INVALID_UNICODE_HEX);
            if (u >= 0xD800 && u <= 0xDBFF) {
              if (*p++ != '\\')
                STRING_ERROR(LJSON_PARSE_INVALID_UNICODE_SURROGATE);
              if (*p++ != 'u')
                STRING_ERROR(LJSON_PARSE_INVALID_UNICODE_SURROGATE);
              if (!(p = parse_hex4(p, &u2)))
                STRING_ERROR(LJSON_PARSE_INVALID_UNICODE_HEX);
              if (u2 < 0xDC00 || u2 > 0xDFFF)
                STRING_ERROR(LJSON_PARSE_INVALID_UNICODE_SURROGATE);
              u = (((u - 0xD800) << 10) | (u2 - 0xDC00)) + 0x10000;
            }
            encode_uft8(u);
            break;
          default:
            STRING_ERROR(LJSON_PARSE_INVALID_STRING_ESCAPE);
        }
        break;
      case '\0':
        STRING_ERROR(LJSON_PARSE_MISS_QUOTATION_MARK);
      default:
        if (static_cast<unsigned char>(ch) < 0x20)
          STRING_ERROR(LJSON_PARSE_INVALID_STRING_CHAR);
        put_char(ch);
    }
  }
}

std::shared_ptr<ljson_value> ljson_context::parse_string(int *ret) {
  *ret = LJSON_PARSE_OK;
  char *str = nullptr;
  size_t len = 0;
  *ret = parse_string_raw(&str, &len);
  if (*ret == LJSON_PARSE_OK) {
    std::string val(str, len);
    return ljson_string::create(val);
  }
  return nullptr;
}

std::shared_ptr<ljson_value> ljson_context::parse_array(int *ret) {
  size_t size = 0;
  expect_next('[');
  parse_whitespace();
  *ret = LJSON_PARSE_OK;
  if (*json_ == ']') {
    json_++;
    return ljson_array::create();
  }
  for (;;) {
    std::shared_ptr<ljson_value> tmp_value = parse_value(ret);
    if (*ret != LJSON_PARSE_OK)
      break;
    push_buffer(tmp_value);
    size++;
    parse_whitespace();
    if (*json_ == ',') {
      json_++;
      parse_whitespace();
    } else if (*json_ == ']') {
      json_++;
      auto buffer = pop_buffer<ljson_value>(size);
      return ljson_array::create(buffer);
    } else {
      *ret = LJSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
      break;
    }
  }
  // Pop the value in the buffer
  pop_buffer<ljson_value>(size);
  return nullptr;
}

std::shared_ptr<ljson_value> ljson_context::parse_object(int *ret) {
  size_t size = 0;
  *ret = LJSON_PARSE_OK;
  expect_next('{');
  parse_whitespace();
  if (*json_ == '}') {
    json_++;
    return ljson_objects::create();
  }
  for (;;) {
    /// we have to declare obj here, because @func push_buffer is shallow copy
    char *key = nullptr;
    size_t len = 0;
    if (*json_ != '"') {
      *ret = LJSON_PARSE_MISS_KEY;
      break;
    }
    *ret = parse_string_raw(&key, &len);
    std::string k(key, len);
    if (*ret != LJSON_PARSE_OK) {
      break;
    }
    parse_whitespace();
    if (*json_ != ':') {
      *ret = LJSON_PARSE_MISS_COLON;
      break;
    }
    json_++;
    parse_whitespace();
    /// parse value
    auto value = parse_value(ret);
    if (*ret != LJSON_PARSE_OK) {
      break;
    }
    /// notice here are shallow copy
    auto obj = std::make_shared<ljson_member>(k, value);
    push_buffer(obj);
    size++;
    parse_whitespace();
    if (*json_ == ',') {
      json_++;
      parse_whitespace();
    } else if (*json_ == '}') {
      json_++;
      auto buffer = pop_buffer<ljson_member>(size);
      return ljson_objects::create(buffer);
    } else {
      *ret = LJSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
      break;
    }
  }
  /// pop and free members in the buffer
  pop_buffer<ljson_member>(size);
  return nullptr;
}

std::shared_ptr<ljson_value> ljson_context::parse_value(int *ret) {
  switch (*json_) {
    case 't': return parse_literal("true", LJSON_TRUE, ret);
    case 'f': return parse_literal("false", LJSON_FALSE, ret);
    case 'n': return parse_literal("null", LJSON_NULL, ret);
    default: return parse_number(ret);
    case '"': return parse_string(ret);
    case '[': return parse_array(ret);
    case '{': return parse_object(ret);
    case '\0': { *ret = LJSON_PARSE_EXPECT_VALUE; return nullptr; }
  }
}

std::shared_ptr<ljson_value> ljson_value::parse(const char *json, int *ret) {
  ljson_context context(json);
  context.parse_whitespace();
  *ret = LJSON_PARSE_OK;
  auto value = context.parse_value(ret);
  if (*ret == LJSON_PARSE_OK) {
    context.parse_whitespace();
    if (*context.json_ == '\0') {
      return value;
    }
    *ret = LJSON_PARSE_ROOT_NOT_SINGULAR;
  }
  return nullptr;
}







} // namespace ljson
