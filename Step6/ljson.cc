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

}

struct ljson_context {
public:
  explicit ljson_context(const char* json) : json_(json), stack_(nullptr), size_(0), top_(0) {}
  ~ljson_context();
  void parse_whitespace();
  LJSON_STATE parse_literal(const std::shared_ptr<ljson_value> &value, const char* literal, LJSON_TYPE type);
  LJSON_STATE parse_number(const std::shared_ptr<ljson_value> &value);
  LJSON_STATE parse_value(const std::shared_ptr<ljson_value> &value);
  void encode_uft8(unsigned u);
  LJSON_STATE parse_string_raw(char **str, size_t *len);
  LJSON_STATE parse_string(const std::shared_ptr<ljson_value> &value);
  LJSON_STATE parse_array(const std::shared_ptr<ljson_value> &value);
  LJSON_STATE parse_object(const std::shared_ptr<ljson_value> &value);

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

LJSON_STATE ljson_context::parse_literal(const std::shared_ptr<ljson_value> &value,
                                         const char *literal,
                                         LJSON_TYPE type) {
  size_t i = 0;
  expect_next(literal[0]);
  for (; literal[i+1]; i++)
    if (json_[i] != literal[i+1])
      return LJSON_PARSE_INVALID_VALUE;
  json_ += i;
  value->set_type(type);
  return LJSON_PARSE_OK;
}

LJSON_STATE ljson_context::parse_number(const std::shared_ptr<ljson_value> &value) {
  const char* p = json_;
  // process sign character
  if (*p == '-') p++;
  // process only one '0'
  if (*p == '0') p++;
  else {
    if (!isdigit1to9(*p)) return LJSON_PARSE_INVALID_VALUE;
    // process all digits
    for (p++; isdigit(*p); ++p);
  }
  if (*p == '.') {
    p++;
    if (!isdigit(*p)) return LJSON_PARSE_INVALID_VALUE;
    // process all digits
    for (p++; isdigit(*p); ++p);
  }
  if (*p == 'e' || *p == 'E') {
    p++;
    if (*p == '+' || *p == '-') p++;
    if (!isdigit(*p)) return LJSON_PARSE_INVALID_VALUE;
    // process all digits
    for (p++; isdigit(*p); ++p);
  }
  errno = 0;
  auto number = strtod(json_, nullptr);
  // in set_number we set the type to LJSON_NUMBER
  value->set_number(number);
  if (errno == ERANGE && (number == HUGE_VAL || number == -HUGE_VAL)) {
    // we have to set back value type to LSJON_NULL
    value->set_type(LJSON_NULL);
    return LJSON_PARSE_NUMBER_TOO_BIG;
  }
  json_ = p;
  return LJSON_PARSE_OK;
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

LJSON_STATE ljson_context::parse_string(const std::shared_ptr<ljson_value> &value) {
  LJSON_STATE ret = LJSON_PARSE_OK;
  char *str = nullptr;
  size_t len = 0;
  if ((ret = parse_string_raw(&str, &len)) == LJSON_PARSE_OK) {
    value->set_string(str, len);
  }
  return ret;
}

LJSON_STATE ljson_context::parse_array(const std::shared_ptr<ljson_value> &value) {
  size_t size = 0;
  expect_next('[');
  parse_whitespace();
  if (*json_ == ']') {
    json_++;
    value->set_type(LJSON_ARRAY);
    return LJSON_PARSE_OK;
  }
  LJSON_STATE ret;
  for (;;) {
    auto tmp_value = ljson_value::create();
    if ((ret = parse_value(tmp_value)) != LJSON_PARSE_OK)
      break;
    push_buffer(tmp_value);
    size++;
    parse_whitespace();
    if (*json_ == ',') {
      json_++;
      parse_whitespace();
    } else if (*json_ == ']') {
      json_++;
      value->set_type(LJSON_ARRAY);
      auto buffer = pop_buffer<ljson_value>(size);
      value->set_array(buffer);
      return LJSON_PARSE_OK;
    } else {
      ret = LJSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
      break;
    }
  }
  // Pop the value in the buffer
  pop_buffer<ljson_value>(size);
  return ret;
}

LJSON_STATE ljson_context::parse_object(const std::shared_ptr<ljson_value> &value) {
  size_t size = 0;
  LJSON_STATE ret = LJSON_PARSE_OK;
  expect_next('{');
  parse_whitespace();
  if (*json_ == '}') {
    json_++;
    value->set_type(LJSON_OBJECT);
    return LJSON_PARSE_OK;
  }
  for (;;) {
    /// we have to declare obj here, because @func push_buffer is shallow copy
    auto obj = std::make_shared<ljson_member>();
    char *key = nullptr;
    if (*json_ != '"') {
      ret = LJSON_PARSE_MISS_KEY;
      break;
    }
    if ((ret = parse_string_raw(&key, &obj->len)) != LJSON_PARSE_OK) {
      break;
    }
    memcpy(obj->k = static_cast<char *>(malloc(obj->len + 1)), key, obj->len);
    obj->k[obj->len] = '\0';
    parse_whitespace();
    if (*json_ != ':') {
      ret = LJSON_PARSE_MISS_COLON;
      break;
    }
    json_++;
    parse_whitespace();
    /// parse value
    if ((ret = parse_value(obj->value)) != LJSON_PARSE_OK) {
      break;
    }
    /// notice here are shallow copy
    push_buffer(obj);
    size++;
    parse_whitespace();
    if (*json_ == ',') {
      json_++;
      parse_whitespace();
    } else if (*json_ == '}') {
      json_++;
      auto buffer = pop_buffer<ljson_member>(size);
      value->set_objects(buffer);
      return LJSON_PARSE_OK;
    } else {
      ret = LJSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
      break;
    }
  }
  /// pop and free members in the buffer
  pop_buffer<ljson_member>(size);
  return ret;
}

LJSON_STATE ljson_context::parse_value(const std::shared_ptr<ljson_value> &value) {
  switch (*json_) {
    case 't': return parse_literal(value, "true", LJSON_TRUE);
    case 'f': return parse_literal(value, "false", LJSON_FALSE);
    case 'n': return parse_literal(value, "null", LJSON_NULL);
    default: return parse_number(value);
    case '"': return parse_string(value);
    case '[': return parse_array(value);
    case '{': return parse_object(value);
    case '\0': return LJSON_PARSE_EXPECT_VALUE;
  }
}


std::shared_ptr<ljson_value> ljson_value::create() {
  return std::shared_ptr<ljson_value>(new ljson_value);
  // we can't call make_shared here.
  // see:https://stackoverflow.com/a/45127266/6546412
  // return std::make_shared<ljson_value>();
}

ljson_value::~ljson_value() {
  destroy();
}

void ljson_value::destroy() {
  if (type_ == LJSON_STRING) {
    free(str_.str);
  }
  type_ = LJSON_NULL;
}

const char * ljson_value::get_string() const {
  assert_type(LJSON_STRING);
  return str_.str;
}

size_t ljson_value::get_string_length() const {
  assert_type(LJSON_STRING);
  return str_.len;
}

size_t ljson_value::get_array_size() const {
  assert_type(LJSON_ARRAY);
  return elements_.size();
}

std::shared_ptr<ljson_value> ljson_value::get_array_element(size_t index) const {
  assert_type(LJSON_ARRAY);
  assert(index < elements_.size());
  return elements_[index];
}

size_t ljson_value::get_objects_size() const {
  assert_type(LJSON_OBJECT);
  return members_.size();
}

const char* ljson_value::get_object_key(size_t index) const {
  assert_type(LJSON_OBJECT);
  assert(index < members_.size());
  return members_[index]->k;
}

size_t ljson_value::get_object_key_length(size_t index) const {
  assert_type(LJSON_OBJECT);
  assert(index < members_.size());
  return members_[index]->len;
}

std::shared_ptr<ljson_value> ljson_value::get_object_value(size_t index) {
  assert_type(LJSON_OBJECT);
  assert(index < members_.size());
  return members_[index]->value;
}

void ljson_value::set_string(const char *s, size_t len) {
  assert(s != nullptr || len == 0);
  destroy();
  str_.str = (char *)malloc(len + 1);
  memcpy(str_.str, s, len);
  str_.str[len] = '\0';
  str_.len = len;
  type_ = LJSON_STRING;
}

void ljson_value::set_array(std::vector<std::shared_ptr<ljson_value>> array) {
  set_type(LJSON_ARRAY);
  elements_ = std::move(array);
}

//void ljson_value::array_push(std::shared_ptr<ljson_value> value) {
//  elements_.push_back(std::move(value));
//}

void ljson_value::set_objects(std::vector<std::shared_ptr<ljson_member> > members) {
  set_type(LJSON_OBJECT);
  members_ = std::move(members);
}

int ljson_value::parse(const char *json) {
  ljson_context context(json);
  context.parse_whitespace();
  type_ = LJSON_NULL;
  int ret = LJSON_PARSE_OK;
  if ((ret = context.parse_value(getptr())) == LJSON_PARSE_OK) {
    context.parse_whitespace();
    if (*context.json_ != '\0') {
      type_ = LJSON_NULL;
      ret = LJSON_PARSE_ROOT_NOT_SINGULAR;
    }
  }
  return ret;
}







} // namespace ljson
