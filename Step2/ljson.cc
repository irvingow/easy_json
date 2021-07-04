//
// Created by 刘文景 on 2021/7/3.
//

#include "ljson.h"
#include <cassert> // assert()
#include <cmath>   // HUGE_VAL

namespace ljson {

namespace {

bool isdigit(const char& ch) { return ch >= '0' && ch <= '9'; }

bool isdigit1to9(const char& ch) { return ch >= '1' && ch <= '9'; }

}

struct ljson_context {
public:
  explicit ljson_context(const char* json) : json_(json) {}
  void parse_whitespace();
  LJSON_STATE parse_literal(const std::shared_ptr<ljson_value> &value, const char* literal, LJSON_TYPE type);
  LJSON_STATE parse_number(const std::shared_ptr<ljson_value> &value);
  LJSON_STATE parse_value(const std::shared_ptr<ljson_value> &value);

public:
  const char *json_;
  /*
   * @ch: next expected character
   * @noted: exit if next character is not ch
   */
  void expect_next(const char &ch);
};

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
  value->set_number(number);
  if (errno == ERANGE && (number == HUGE_VAL || number == -HUGE_VAL))
    return LJSON_PARSE_NUMBER_TOO_BIG;
  value->set_type(LJSON_NUMBER);
  json_ = p;
  return LJSON_PARSE_OK;
}

LJSON_STATE ljson_context::parse_value(const std::shared_ptr<ljson_value> &value) {
  switch (*json_) {
    case 't': return parse_literal(value, "true", LJSON_TRUE);
    case 'f': return parse_literal(value, "false", LJSON_FALSE);
    case 'n': return parse_literal(value, "null", LJSON_NULL);
    default: return parse_number(value);
    case '\0': return LJSON_PARSE_EXPECT_VALUE;
  }
}


std::shared_ptr<ljson_value> ljson_value::create() {
  return std::shared_ptr<ljson_value>(new ljson_value);
  // we can't call make_shared here.
  // see:https://stackoverflow.com/a/45127266/6546412
  // return std::make_shared<ljson_value>();
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
