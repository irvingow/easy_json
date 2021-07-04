//
// Created by 刘文景 on 2021/7/3.
//

#include "ljson.h"

using namespace ljson;

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

#define EXPECT_EQ_BASE(equality, expect, actual, format) \
do { \
  test_count++; \
  if (equality) \
    test_pass++;\
  else {      \
    fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", __FILE__, __LINE__, expect, actual);\
    main_ret = 1;\
    }\
} while(0)\

#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect == actual), expect, actual, "%d")


static void test_parse_null() {
  auto value = ljson_value::create();
  EXPECT_EQ_INT(LJSON_PARSE_OK, value->parse("null"));
  EXPECT_EQ_INT(LJSON_NULL, value->get_type());
}

static void test_parse_true() {
  auto value = ljson_value::create();
  EXPECT_EQ_INT(LJSON_PARSE_OK, value->parse("true"));
  EXPECT_EQ_INT(LJSON_TRUE, value->get_type());
}

static void test_parse_false() {
  auto value = ljson_value::create();
  EXPECT_EQ_INT(LJSON_PARSE_OK, value->parse("false"));
  EXPECT_EQ_INT(LJSON_FALSE, value->get_type());
}

static void test_parse_expect_value() {
  auto value = ljson_value::create();

  value->set_type(LJSON_FALSE);
  EXPECT_EQ_INT(LJSON_PARSE_EXPECT_VALUE, value->parse(""));
  EXPECT_EQ_INT(LJSON_NULL, value->get_type());

  value->set_type(LJSON_FALSE);
  EXPECT_EQ_INT(LJSON_PARSE_EXPECT_VALUE, value->parse(" "));
  EXPECT_EQ_INT(LJSON_NULL, value->get_type());
}

static void test_parse_invalid_value() {
  auto value = ljson_value::create();

  value->set_type(LJSON_FALSE);
  EXPECT_EQ_INT(LJSON_PARSE_INVALID_VALUE, value->parse("nul"));
  EXPECT_EQ_INT(LJSON_NULL, value->get_type());

  value->set_type(LJSON_FALSE);
  EXPECT_EQ_INT(LJSON_PARSE_INVALID_VALUE, value->parse("?"));
  EXPECT_EQ_INT(LJSON_NULL, value->get_type());
}

static void test_parse_root_not_singular() {
  auto value = ljson_value::create();

  value->set_type(LJSON_FALSE);
  EXPECT_EQ_INT(LJSON_PARSE_ROOT_NOT_SINGULAR, value->parse("null x"));
  EXPECT_EQ_INT(LJSON_NULL, value->get_type());
}

static void test_parse() {
  test_parse_null();
  test_parse_false();
  test_parse_true();
  test_parse_expect_value();
  test_parse_invalid_value();
  test_parse_root_not_singular();
}

int main() {
  test_parse();
  printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
  return main_ret;
}









