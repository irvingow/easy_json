//
// Created by 刘文景 on 2021/7/3.
//

#include "ljson.h"
#include <cassert> // assert()
#include <cstdlib> // nullptr

namespace ljson {

struct ljson_context {
public:
  explicit ljson_context(const char* json) : json_(json) {}
  void parse_whitespace();
  LJSON_STATE parse_true(const std::shared_ptr<ljson_value> &value);
  LJSON_STATE parse_false(const std::shared_ptr<ljson_value> &value);
  LJSON_STATE parse_null(const std::shared_ptr<ljson_value> &value);
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

LJSON_STATE ljson_context::parse_true(const std::shared_ptr<ljson_value> &value) {
  expect_next('t');
  if (json_[0] != 'r' || json_[1] != 'u' || json_[2] != 'e')
    return LJSON_PARSE_INVALID_VALUE;
  json_ += 3;
  value->set_type(LJSON_TRUE);
  return LJSON_PARSE_OK;
}

LJSON_STATE ljson_context::parse_false(const std::shared_ptr<ljson_value> &value) {
  expect_next('f');
  if (json_[0] != 'a' || json_[1] != 'l' || json_[2] != 's' || json_[3] != 'e')
    return LJSON_PARSE_INVALID_VALUE;
  json_ += 4;
  value->set_type(LJSON_FALSE);
  return LJSON_PARSE_OK;
}

LJSON_STATE ljson_context::parse_null(const std::shared_ptr<ljson_value> &value) {
  expect_next('n');
  if (json_[0] != 'u' || json_[1] != 'l' || json_[2] != 'l')
    return LJSON_PARSE_INVALID_VALUE;
  json_ += 3;
  value->set_type(LJSON_NULL);
  return LJSON_PARSE_OK;
}

LJSON_STATE ljson_context::parse_value(const std::shared_ptr<ljson_value> &value) {
  switch (*json_) {
    case 't': return parse_true(value);
    case 'f': return parse_false(value);
    case 'n': return parse_null(value);
    case '\0': return LJSON_PARSE_EXPECT_VALUE;
    default: return LJSON_PARSE_INVALID_VALUE;
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
    if (*context.json_ != '\0')
      ret = LJSON_PARSE_ROOT_NOT_SINGULAR;
  }
  return ret;
}







} // namespace ljson
