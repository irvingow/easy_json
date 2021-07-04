//
// Created by 刘文景 on 2021/7/3.
//

#include "ljson.h"
#include <cassert> // assert()
#include <cmath>   // HUGE_VAL

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
  LJSON_STATE parse_string(const std::shared_ptr<ljson_value> &value);

public:
  void put_char(char ch);
  void *push(size_t size);
  void *pop(size_t size);

public:
  const char *json_;
  char *stack_;
  size_t size_, top_;
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
  // notice that here, we also set value.type to LJSON_NUMBER
  value->set_number(number);
  if (errno == ERANGE && (number == HUGE_VAL || number == -HUGE_VAL)) {
    // because number is invalid, so we have to set the
    // type to LJSON_NULL
    value->set_type(LJSON_NULL);
    return LJSON_PARSE_NUMBER_TOO_BIG;
  }
  json_ = p;
  return LJSON_PARSE_OK;
}

LJSON_STATE ljson_context::parse_string(const std::shared_ptr<ljson_value> &value) {
  size_t head = top_, len = 0;
  expect_next('\"');
  const char* p = json_;
  for (;;) {
    char ch = *p++;
    switch (ch) {
      case '\"':
        // whole string has been processed.
        len = top_ - head;
        value->set_string(static_cast<char *>(pop(len)), len);
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
          default:
            top_ = head;
            return LJSON_PARSE_INVALID_STRING_ESCAPE;
        }
        break;
      case '\0':
        // reset top_
        top_ = head;
        return LJSON_PARSE_MISS_QUOTATION_MARK;
      default:
        if (static_cast<unsigned char>(ch) < 0x20) {
          // reset top_
          top_ = head;
          return LJSON_PARSE_INVALID_STRING_CHAR;
        }
        put_char(ch);
    }
  }
}

LJSON_STATE ljson_context::parse_value(const std::shared_ptr<ljson_value> &value) {
  switch (*json_) {
    case 't': return parse_literal(value, "true", LJSON_TRUE);
    case 'f': return parse_literal(value, "false", LJSON_FALSE);
    case 'n': return parse_literal(value, "null", LJSON_NULL);
    default: return parse_number(value);
    case '"': return parse_string(value);
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
  assert_is_string();
  return str_.str;
}

size_t ljson_value::get_string_length() const {
  assert_is_string();
  return str_.len;
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
