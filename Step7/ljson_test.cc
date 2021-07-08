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

#define EXPECT_NULL(val) \
do { \
  test_count++; \
  if (val == nullptr) \
    test_pass++;\
  else {      \
    fprintf(stderr, "%s:%d: expect: nullptr actual not\n", __FILE__, __LINE__);\
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
  int ret = LJSON_PARSE_OK;
  auto value = ljson_value::parse("null", &ret);
  EXPECT_EQ_INT(LJSON_PARSE_OK, ret);
  EXPECT_EQ_INT(LJSON_NULL, value->get_type());
}

static void test_parse_true() {
  int ret = LJSON_PARSE_OK;
  auto value = ljson_value::parse("true", &ret);
  EXPECT_EQ_INT(LJSON_PARSE_OK, ret);
  EXPECT_EQ_INT(LJSON_TRUE, value->get_type());
}

static void test_parse_false() {
  int ret = LJSON_PARSE_OK;
  auto value = ljson_value::parse("false", &ret);
  EXPECT_EQ_INT(LJSON_PARSE_OK, ret);
  EXPECT_EQ_INT(LJSON_FALSE, value->get_type());
}


#define TEST_NUMBER(expect, json)\
    do {\
        int ret = LJSON_PARSE_OK;\
        auto value = ljson_value::parse(json, &ret);\
        EXPECT_EQ_INT(LJSON_PARSE_OK, ret);\
        EXPECT_EQ_INT(LJSON_NUMBER, value->get_type());\
        auto number = ljson_number::get_value_helper(value->get_value());\
        EXPECT_EQ_DOUBLE(expect, number);\
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
        int ret = 0;\
        auto value = ljson_value::parse(json, &ret);\
        EXPECT_EQ_INT(LJSON_PARSE_OK, ret);\
        EXPECT_EQ_INT(LJSON_STRING, value->get_type());\
        auto str = ljson_string::get_value_helper(value->get_value());\
        EXPECT_EQ_STRING(expect, str.data(), str.size());\
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
    int ret = 0;
    auto value = ljson_value::parse("[ ]", &ret);
    EXPECT_EQ_INT(LJSON_PARSE_OK, ret);
    EXPECT_EQ_INT(LJSON_ARRAY, value->get_type());
    auto array= ljson_array::get_value_helper(value->get_value());
    EXPECT_EQ_SIZE_T(0, array.size());
  }
  {
    int ret = 0;
    auto value = ljson_value::parse("[ null, false, true, 123, \"abs\" ]", &ret);
    EXPECT_EQ_INT(LJSON_PARSE_OK, ret);
    EXPECT_EQ_INT(LJSON_ARRAY, value->get_type());
    auto array= ljson_array::get_value_helper(value->get_value());
    EXPECT_EQ_SIZE_T(5, array.size());
    EXPECT_EQ_INT(LJSON_NULL, array[0]->get_type());
    EXPECT_EQ_INT(LJSON_FALSE, array[1]->get_type());
    EXPECT_EQ_INT(LJSON_TRUE, array[2]->get_type());
    EXPECT_EQ_INT(LJSON_NUMBER, array[3]->get_type());
    EXPECT_EQ_INT(LJSON_STRING, array[4]->get_type());
  }
  {
    int ret = 0;
    auto value = ljson_value::parse("[ [ ] , [ 0 ], [ 0 , 1 ], [ 0 , 1 , 2 ] ]", &ret);
    EXPECT_EQ_INT(LJSON_PARSE_OK, ret);
    EXPECT_EQ_INT(LJSON_ARRAY, value->get_type());
    auto array= ljson_array::get_value_helper(value->get_value());
    EXPECT_EQ_SIZE_T(4, array.size());
    for (size_t i = 0; i < 4; ++i) {
      auto element = array[i];
      EXPECT_EQ_INT(LJSON_ARRAY, element->get_type());
      auto element_array = ljson_array::get_value_helper(element->get_value());
      EXPECT_EQ_SIZE_T(i, element_array.size());
      for (size_t j = 0; j < i; j++) {
        auto inside = element_array[j];
        EXPECT_EQ_INT(LJSON_NUMBER, inside->get_type());
        auto inside_val = ljson_number::get_value_helper(inside->get_value());
        EXPECT_EQ_DOUBLE((double)j, inside_val);
      }
    }
  }
}

static void test_parse_objects() {
  {
    int ret = LJSON_PARSE_OK;
    auto value = ljson_value::parse(" { } ", &ret);
    EXPECT_EQ_INT(LJSON_PARSE_OK, ret);
    EXPECT_EQ_INT(LJSON_OBJECT, value->get_type());
    auto members = ljson_objects::get_value_helper(value->get_value());
    EXPECT_EQ_SIZE_T(0, members.size());
  }
  {
    int ret = LJSON_PARSE_OK;
    auto value = ljson_value::parse(
    " { "
    "\"n\" : null , "
    "\"f\" : false , "
    "\"t\" : true , "
    "\"i\" : 123 , "
    "\"s\" : \"abc\", "
    "\"a\" : [ 1, 2, 3 ],"
    "\"o\" : { \"1\" : 1, \"2\" : 2, \"3\" : 3 }"
    " } "
    , &ret);
    EXPECT_EQ_INT(LJSON_PARSE_OK, ret);
    EXPECT_EQ_INT(LJSON_OBJECT, value->get_type());
    auto objects = ljson_objects::get_value_helper(value->get_value());
    EXPECT_EQ_SIZE_T(7, objects.size());
    EXPECT_EQ_STRING("n", objects[0]->key.data(), objects[0]->key.size());
    EXPECT_EQ_INT(LJSON_NULL, objects[0]->value->get_type());
    EXPECT_EQ_STRING("f", objects[1]->key.data(), objects[1]->key.size());
    EXPECT_EQ_INT(LJSON_FALSE, objects[1]->value->get_type());
    EXPECT_EQ_STRING("t", objects[2]->key.data(), objects[2]->key.size());
    EXPECT_EQ_INT(LJSON_TRUE, objects[2]->value->get_type());
    EXPECT_EQ_STRING("i", objects[3]->key.data(), objects[3]->key.size());
    EXPECT_EQ_INT(LJSON_NUMBER, objects[3]->value->get_type());
    EXPECT_EQ_DOUBLE(123.0, ljson_number::get_value_helper(objects[3]->value->get_value()));
    EXPECT_EQ_STRING("s", objects[4]->key.data(), objects[4]->key.size());
    EXPECT_EQ_INT(LJSON_STRING, objects[4]->value->get_type());
    auto str = ljson_string::get_value_helper(objects[4]->value->get_value());
    EXPECT_EQ_STRING("abc", str.data(), str.length());
    EXPECT_EQ_STRING("a", objects[5]->key.data(), objects[5]->key.size());
    EXPECT_EQ_INT(LJSON_ARRAY, objects[5]->value->get_type());
    auto array = ljson_array::get_value_helper(objects[5]->value->get_value());
    EXPECT_EQ_SIZE_T(3, array.size());
    for (size_t i = 0; i < 3; ++i) {
      auto element = array[i];
      EXPECT_EQ_INT(LJSON_NUMBER, element->get_type());
      EXPECT_EQ_DOUBLE(i + 1.0, ljson_number::get_value_helper(element->get_value()));
    }
    EXPECT_EQ_STRING("o", objects[6]->key.data(), objects[6]->key.size());
    {
      EXPECT_EQ_INT(LJSON_OBJECT, objects[6]->value->get_type());
      auto inside_objects = ljson_objects::get_value_helper(objects[6]->value->get_value());
      for (size_t i = 0; i < 3; ++i) {
        auto inside = inside_objects[i];
        char expect = '1' + i;
        EXPECT_TRUE(expect == inside->key[0]);
        EXPECT_EQ_INT(LJSON_NUMBER, inside->value->get_type());
        EXPECT_EQ_DOUBLE(i + 1.0, ljson_number::get_value_helper(inside->value->get_value()));
      }
    }
  }
}

#define TEST_ERROR(error, json)\
    do {\
        int ret = LJSON_PARSE_OK;\
        auto value = ljson_value::parse(json, &ret);\
        EXPECT_EQ_INT(error, ret);\
        EXPECT_NULL(value);\
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


static void test_parse_miss_comma_or_square_bracket() {
  TEST_ERROR(LJSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1");
  TEST_ERROR(LJSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1}");
  TEST_ERROR(LJSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1 2");
  TEST_ERROR(LJSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[[]");
}

static void test_parse_miss_key() {
  TEST_ERROR(LJSON_PARSE_MISS_KEY, "{:1,");
  TEST_ERROR(LJSON_PARSE_MISS_KEY, "{1:1,");
  TEST_ERROR(LJSON_PARSE_MISS_KEY, "{true:1,");
  TEST_ERROR(LJSON_PARSE_MISS_KEY, "{false:1,");
  TEST_ERROR(LJSON_PARSE_MISS_KEY, "{null:1,");
  TEST_ERROR(LJSON_PARSE_MISS_KEY, "{[]:1,");
  TEST_ERROR(LJSON_PARSE_MISS_KEY, "{{}:1,");
  TEST_ERROR(LJSON_PARSE_MISS_KEY, "{\"a\":1,");
}

static void test_parse_miss_colon() {
  TEST_ERROR(LJSON_PARSE_MISS_COLON, "{\"a\"}");
  TEST_ERROR(LJSON_PARSE_MISS_COLON, "{\"a\",\"b\"}");
}

static void test_parse_miss_comma_or_curly_bracket() {
  TEST_ERROR(LJSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "{\"a\":1");
  TEST_ERROR(LJSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "{\"a\":1]");
  TEST_ERROR(LJSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "{\"a\":1 \"b\"");
  TEST_ERROR(LJSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "{\"a\":{}");
}

static void test_access_null() {
  auto value = ljson_null::create();
  EXPECT_EQ_INT(LJSON_NULL, value->get_type());
}

static void test_access_boolean() {
  auto value = ljson_true::create();
  EXPECT_TRUE(value->get_type());
  value = ljson_false::create();
  EXPECT_FALSE(value->get_type());
}

static void test_access_number() {
  auto value = ljson_number::create(123.45);
  auto number = std::static_pointer_cast<double>(value->get_value());
  EXPECT_EQ_DOUBLE(123.45, *number);
}

static void test_access_string() {
  auto value = ljson_string::create("");
  auto str = ljson_string::get_value_helper(value->get_value());
  EXPECT_EQ_STRING("", str.data(), str.size());
  value = ljson_string::create("Hello");
  str = ljson_string::get_value_helper(value->get_value());
  EXPECT_EQ_STRING("Hello", str.data(), str.size());
}

static void test_parse() {
  test_parse_null();
  test_parse_false();
  test_parse_true();
  test_parse_number();
  test_parse_string();
  test_parse_array();
  test_parse_objects();
  test_parse_expect_value();
  test_parse_invalid_value();
  test_parse_root_not_singular();
  test_parse_number_too_big();
  test_parse_missing_quotation_mark();
  test_parse_invalid_string_escape();
  test_parse_invalid_string_char();
  test_parse_invalid_unicode_hex();
  test_parse_invalid_unicode_surrogate();
  test_parse_miss_colon();
  test_parse_miss_comma_or_curly_bracket();
  test_parse_miss_key();
  test_parse_miss_comma_or_square_bracket();
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









