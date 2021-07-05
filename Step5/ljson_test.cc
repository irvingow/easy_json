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
#define EXPECT_EQ_DOUBLE(expect, actual) EXPECT_EQ_BASE((expect == actual), expect, actual, "%.17g")
#define EXPECT_EQ_STRING(expect, actual, length) \
    EXPECT_EQ_BASE(sizeof(expect) - 1 == length && memcmp(expect, actual, length) == 0, expect, actual, "%s")
#define EXPECT_TRUE(actual) EXPECT_EQ_BASE((actual) != 0, "true", "false", "%s")
#define EXPECT_FALSE(actual) EXPECT_EQ_BASE((actual) != 0, "false", "true", "%s")

#define EXPECT_EQ_SIZE_T(expect, actual) EXPECT_EQ_BASE((expect) == (actual), (size_t) expect, (size_t)actual, "%zu")

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


#define TEST_NUMBER(expect, json)\
    do {\
        auto value = ljson_value::create();\
        EXPECT_EQ_INT(LJSON_PARSE_OK, value->parse(json));\
        EXPECT_EQ_INT(LJSON_NUMBER, value->get_type());\
        EXPECT_EQ_DOUBLE(expect, value->get_number());\
    } while(0)

static void test_parse_number() {
  TEST_NUMBER(0.0, "0");
  TEST_NUMBER(0.0, "-0");
  TEST_NUMBER(0.0, "-0.0");
  TEST_NUMBER(1.0, "1");
  TEST_NUMBER(-1.0, "-1");
  TEST_NUMBER(1.5, "1.5");
  TEST_NUMBER(-1.5, "-1.5");
  TEST_NUMBER(3.1416, "3.1416");
  TEST_NUMBER(1E10, "1E10");
  TEST_NUMBER(1e10, "1e10");
  TEST_NUMBER(1E+10, "1E+10");
  TEST_NUMBER(1E-10, "1E-10");
  TEST_NUMBER(-1E10, "-1E10");
  TEST_NUMBER(-1e10, "-1e10");
  TEST_NUMBER(-1E+10, "-1E+10");
  TEST_NUMBER(-1E-10, "-1E-10");
  TEST_NUMBER(1.234E+10, "1.234E+10");
  TEST_NUMBER(1.234E-10, "1.234E-10");
  TEST_NUMBER(0.0, "1e-10000"); /* must underflow */

  TEST_NUMBER(1.0000000000000002, "1.0000000000000002"); /* the smallest number > 1 */
  TEST_NUMBER( 4.9406564584124654e-324, "4.9406564584124654e-324"); /* minimum denormal */
  TEST_NUMBER(-4.9406564584124654e-324, "-4.9406564584124654e-324");
  TEST_NUMBER( 2.2250738585072009e-308, "2.2250738585072009e-308");  /* Max subnormal double */
  TEST_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308");
  TEST_NUMBER( 2.2250738585072014e-308, "2.2250738585072014e-308");  /* Min normal positive double */
  TEST_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308");
  TEST_NUMBER( 1.7976931348623157e+308, "1.7976931348623157e+308");  /* Max double */
  TEST_NUMBER(-1.7976931348623157e+308, "-1.7976931348623157e+308");
}

#define TEST_STRING(expect, json)\
    do {\
        auto value = ljson_value::create();\
        EXPECT_EQ_INT(LJSON_PARSE_OK, value->parse(json));\
        EXPECT_EQ_INT(LJSON_STRING, value->get_type());\
        EXPECT_EQ_STRING(expect, value->get_string(), value->get_string_length());\
    } while(0)

static void test_parse_string() {
  TEST_STRING("","\"\"");
  TEST_STRING("Hello", "\"Hello\"");
  TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
  TEST_STRING("\" \\ / \b \f \n \r \t", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
  TEST_STRING("Hello\0World", "\"Hello\\u0000World\"");
  TEST_STRING("\x24", "\"\\u0024\"");         /* Dollar sign U+0024 */
  TEST_STRING("\xC2\xA2", "\"\\u00A2\"");     /* Cents sign U+00A2 */
  TEST_STRING("\xE2\x82\xAC", "\"\\u20AC\""); /* Euro sign U+20AC */
  TEST_STRING("\xF0\x9D\x84\x9E", "\"\\uD834\\uDD1E\"");  /* G clef sign U+1D11E */
  TEST_STRING("\xF0\x9D\x84\x9E", "\"\\ud834\\udd1e\"");  /* G clef sign U+1D11E */
}

static void test_parse_array() {
  {
    auto value = ljson_value::create();
    EXPECT_EQ_INT(LJSON_PARSE_OK, value->parse("[ ]"));
    EXPECT_EQ_INT(LJSON_ARRAY, value->get_type());
    EXPECT_EQ_SIZE_T(0, value->get_array_size());
  }
  {
    auto value = ljson_value::create();
    EXPECT_EQ_INT(LJSON_PARSE_OK, value->parse("[ null, false, true, 123, \"abs\" ]"));
    EXPECT_EQ_INT(LJSON_ARRAY, value->get_type());
    EXPECT_EQ_SIZE_T(5, value->get_array_size());
    EXPECT_EQ_INT(LJSON_NULL, value->get_array_element(0)->get_type());
    EXPECT_EQ_INT(LJSON_FALSE, value->get_array_element(1)->get_type());
    EXPECT_EQ_INT(LJSON_TRUE, value->get_array_element(2)->get_type());
    EXPECT_EQ_INT(LJSON_NUMBER, value->get_array_element(3)->get_type());
    EXPECT_EQ_INT(LJSON_STRING, value->get_array_element(4)->get_type());
  }
  {
    auto value = ljson_value::create();
    EXPECT_EQ_INT(LJSON_PARSE_OK, value->parse("[ [ ] , [ 0 ], [ 0 , 1 ], [ 0 , 1 , 2 ] ]"));
    EXPECT_EQ_INT(LJSON_ARRAY, value->get_type());
    EXPECT_EQ_SIZE_T(4, value->get_array_size());
    for (size_t i = 0; i < 4; ++i) {
      auto element = value->get_array_element(i);
      EXPECT_EQ_INT(LJSON_ARRAY, element->get_type());
      EXPECT_EQ_SIZE_T(i, element->get_array_size());
      for (size_t j = 0; j < i; j++) {
        auto inside = element->get_array_element(j);
        EXPECT_EQ_INT(LJSON_NUMBER, inside->get_type());
        EXPECT_EQ_DOUBLE((double)j, inside->get_number());
      }
    }
  }
}

#define TEST_ERROR(error, json)\
    do {\
        auto value = ljson_value::create();\
        value->set_type(LJSON_FALSE);\
        EXPECT_EQ_INT(error, value->parse(json));\
        EXPECT_EQ_INT(LJSON_NULL, value->get_type());\
    } while(0)


static void test_parse_expect_value() {
  TEST_ERROR(LJSON_PARSE_EXPECT_VALUE, "");
  TEST_ERROR(LJSON_PARSE_EXPECT_VALUE, " ");
}


static void test_parse_invalid_value() {
  TEST_ERROR(LJSON_PARSE_INVALID_VALUE, "nul");
  TEST_ERROR(LJSON_PARSE_INVALID_VALUE, "?");

  /* invalid number */
  TEST_ERROR(LJSON_PARSE_INVALID_VALUE, "+0");
  TEST_ERROR(LJSON_PARSE_INVALID_VALUE, "+1");
  TEST_ERROR(LJSON_PARSE_INVALID_VALUE, ".123"); /* at least one digit before '.' */
  TEST_ERROR(LJSON_PARSE_INVALID_VALUE, "1.");   /* at least one digit after '.' */
  TEST_ERROR(LJSON_PARSE_INVALID_VALUE, "INF");
  TEST_ERROR(LJSON_PARSE_INVALID_VALUE, "inf");
  TEST_ERROR(LJSON_PARSE_INVALID_VALUE, "NAN");
  TEST_ERROR(LJSON_PARSE_INVALID_VALUE, "nan");
}

static void test_parse_root_not_singular() {
  TEST_ERROR(LJSON_PARSE_ROOT_NOT_SINGULAR, "null x");

  /* invalid number */
  TEST_ERROR(LJSON_PARSE_ROOT_NOT_SINGULAR, "0123"); /* after zero should be '.' or nothing */
  TEST_ERROR(LJSON_PARSE_ROOT_NOT_SINGULAR, "0x0");
  TEST_ERROR(LJSON_PARSE_ROOT_NOT_SINGULAR, "0x123");
}

static void test_parse_number_too_big() {
  TEST_ERROR(LJSON_PARSE_NUMBER_TOO_BIG, "1e309");
  TEST_ERROR(LJSON_PARSE_NUMBER_TOO_BIG, "-1e309");
}

static void test_parse_missing_quotation_mark() {
  TEST_ERROR(LJSON_PARSE_MISS_QUOTATION_MARK, "\"");
  TEST_ERROR(LJSON_PARSE_MISS_QUOTATION_MARK, "\"abs");
}

static void test_parse_invalid_string_escape() {
  TEST_ERROR(LJSON_PARSE_INVALID_STRING_ESCAPE, "\"\\v\"");
  TEST_ERROR(LJSON_PARSE_INVALID_STRING_ESCAPE, "\"\\'\"");
  TEST_ERROR(LJSON_PARSE_INVALID_STRING_ESCAPE, "\"\\0\"");
  TEST_ERROR(LJSON_PARSE_INVALID_STRING_ESCAPE, "\"\\x12\"");
}

static void test_parse_invalid_string_char() {
  TEST_ERROR(LJSON_PARSE_INVALID_STRING_CHAR, "\"\x01\"");
  TEST_ERROR(LJSON_PARSE_INVALID_STRING_CHAR, "\"\x1F\"");
}

static void test_parse_invalid_unicode_hex() {
  TEST_ERROR(LJSON_PARSE_INVALID_UNICODE_HEX, "\"\\u\"");
  TEST_ERROR(LJSON_PARSE_INVALID_UNICODE_HEX, "\"\\u0\"");
  TEST_ERROR(LJSON_PARSE_INVALID_UNICODE_HEX, "\"\\u01\"");
  TEST_ERROR(LJSON_PARSE_INVALID_UNICODE_HEX, "\"\\u012\"");
  TEST_ERROR(LJSON_PARSE_INVALID_UNICODE_HEX, "\"\\u/000\"");
  TEST_ERROR(LJSON_PARSE_INVALID_UNICODE_HEX, "\"\\uG000\"");
  TEST_ERROR(LJSON_PARSE_INVALID_UNICODE_HEX, "\"\\u0/00\"");
  TEST_ERROR(LJSON_PARSE_INVALID_UNICODE_HEX, "\"\\u0G00\"");
  TEST_ERROR(LJSON_PARSE_INVALID_UNICODE_HEX, "\"\\u00/0\"");
  TEST_ERROR(LJSON_PARSE_INVALID_UNICODE_HEX, "\"\\u00G0\"");
  TEST_ERROR(LJSON_PARSE_INVALID_UNICODE_HEX, "\"\\u000/\"");
  TEST_ERROR(LJSON_PARSE_INVALID_UNICODE_HEX, "\"\\u000G\"");
  TEST_ERROR(LJSON_PARSE_INVALID_UNICODE_HEX, "\"\\u 123\"");
}

static void test_parse_invalid_unicode_surrogate() {
  TEST_ERROR(LJSON_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\"");
  TEST_ERROR(LJSON_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uDBFF\"");
  TEST_ERROR(LJSON_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\\\\"");
  TEST_ERROR(LJSON_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uDBFF\"");
  TEST_ERROR(LJSON_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uE000\"");
}

static void test_access_null() {
  auto value = ljson_value::create();
  value->set_string("1", 1);
  value->set_type(LJSON_NULL);
  EXPECT_EQ_INT(LJSON_NULL, value->get_type());
}

static void test_access_boolean() {
  auto value = ljson_value::create();
  value->set_string("a", 1);
  value->set_type(LJSON_TRUE);
  EXPECT_TRUE(value->get_type());
  value->set_type(LJSON_FALSE);
  EXPECT_FALSE(value->get_type());
}

static void test_access_number() {
  auto value = ljson_value::create();
  value->set_string("a", 1);
  value->set_number(123.45);
  EXPECT_EQ_DOUBLE(123.45, value->get_number());
}

static void test_access_string() {
  auto value = ljson_value::create();
  value->set_string("", 0);
  EXPECT_EQ_STRING("", value->get_string(), value->get_string_length());
  value->set_string("Hello", 5);
  EXPECT_EQ_STRING("Hello", value->get_string(), value->get_string_length());
}

static void test_parse() {
  test_parse_null();
  test_parse_false();
  test_parse_true();
  test_parse_number();
  test_parse_string();
  test_parse_array();
  test_parse_expect_value();
  test_parse_invalid_value();
  test_parse_root_not_singular();
  test_parse_number_too_big();
  test_parse_missing_quotation_mark();
  test_parse_invalid_string_escape();
  test_parse_invalid_string_char();
  test_parse_invalid_unicode_hex();
  test_parse_invalid_unicode_surrogate();
}

static void test_access() {
  test_access_null();
  test_access_boolean();
  test_access_number();
  test_access_string();
}

int main() {
  test_parse();
  test_access();
  printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
  return main_ret;
}









