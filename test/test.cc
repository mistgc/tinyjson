#include "tinyjson.hh"
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

#define F_NRM "\x1B[0m"
#define F_FGRN "\x1B[32m"
#define F_FYEL "\x1B[33m"

#define EXPECT_EQ_BASE(equality, expect, actual, format)                       \
  do {                                                                         \
    test_count++;                                                              \
    if (equality)                                                              \
      test_pass++;                                                             \
    else {                                                                     \
      std::fprintf(stderr, "%s:%d expect: " format " actual: " format "\n",    \
                   __FILE__, __LINE__, expect, actual);                        \
      main_ret = 1;                                                            \
    }                                                                          \
  } while (0)

#define EXPECT_EQ_INT(expect, actual)                                          \
  EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")

#define EXPECT_TRUE(actual)                                                    \
  EXPECT_EQ_BASE((0) != (actual), "true", "false", "%s")

#define EXPECT_EQ_DOUBLE(expect, actual)                                       \
  EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%f")

#define EXPECT_EQ_STRING(expect, actual, length)                               \
  EXPECT_EQ_BASE(sizeof(expect) - 1 == length &&                               \
                     !std::memcmp(expect, actual, length),                     \
                 expect, actual, "%s")

#ifdef _MSC_VER
#define EXPECT_EQ_SIZE_T(expect, actual)                                       \
  EXPECT_EQ_BASE((expect) == (actual), (size_t)expect, (size_t)actual, "%lu")
#else
#define EXPECT_EQ_SIZE_T(expect, actual)                                       \
  EXPECT_EQ_BASE((expect) == (actual), (size_t)expect, (size_t)actual, "%zu")
#endif

#define TEST_ERROR(error, json)                                                \
  do {                                                                         \
    tinyjson::Value v;                                                         \
    v.type = tinyjson::Type::FALSE;                                            \
    EXPECT_EQ_INT(error,                                                       \
                  v.parse(std::make_shared<std::string>(std::string(json))));  \
    EXPECT_EQ_INT(tinyjson::Type::NIL, v.get_type());                          \
  } while (0)

#define TEST_NUMBER(expect, json)                                              \
  do {                                                                         \
    tinyjson::Value v;                                                         \
    EXPECT_EQ_INT(tinyjson::Parse::OK,                                         \
                  v.parse(std::make_shared<std::string>(std::string(json))));  \
    EXPECT_EQ_INT(tinyjson::Type::NUMBER, v.get_type());                       \
    EXPECT_EQ_DOUBLE(expect, v.get_number());                                  \
  } while (0)

#define TEST_STRING(expect, json)                                              \
  do {                                                                         \
    tinyjson::Value v;                                                         \
    EXPECT_EQ_INT(tinyjson::Parse::OK,                                         \
                  v.parse(std::make_shared<std::string>(std::string(json))));  \
    EXPECT_EQ_INT(tinyjson::Type::STRING, v.get_type());                       \
    EXPECT_EQ_STRING(expect, v.get_string().c_str(), v.get_string().length()); \
  } while (0)

#define TEST_ARRAY(expect, expect_size, json)                                  \
  do {                                                                         \
    tinyjson::Value v;                                                         \
    EXPECT_EQ_INT(tinyjson::Parse::OK,                                         \
                  v.parse(std::make_shared<std::string>(std::string(json))));  \
    EXPECT_EQ_INT(tinyjson::Type::ARRAY, v.get_type());                        \
    EXPECT_EQ_SIZE_T(expect_size, v.get_array_size());                         \
  } while (0)

#define TEST_GETTER_STRING(expect, json) TEST_STRING(expect, json)

#define TEST_GETTER_NUMBER(expect, json) TEST_NUMBER(expect, json)

#define TEST_GETTER_BOOLEAN(expect, json)                                      \
  do {                                                                         \
    tinyjson::Value v;                                                         \
    int iexpect = -1;                                                          \
    switch (expect) {                                                          \
    case tinyjson::Type::TRUE:                                                 \
      iexpect = 1;                                                             \
      break;                                                                   \
    case tinyjson::Type::FALSE:                                                \
      iexpect = 0;                                                             \
    default:                                                                   \
      break;                                                                   \
    }                                                                          \
    EXPECT_EQ_INT(tinyjson::Parse::OK,                                         \
                  v.parse(std::make_shared<std::string>(std::string(json))));  \
    EXPECT_EQ_INT(expect, v.get_type());                                       \
    EXPECT_EQ_INT(iexpect, (int)(v.get_boolean()));                            \
  } while (0)

static void test_parse_null() {
  tinyjson::Value v;
  v.type = tinyjson::Type::TRUE;
  EXPECT_EQ_INT(tinyjson::Parse::OK,
                v.parse(std::make_shared<std::string>(std::string("null"))));
  EXPECT_EQ_INT(tinyjson::Type::NIL, v.get_type());
}

static void test_parse_expect_value() {
  TEST_ERROR(tinyjson::Parse::EXPECT_VALUE, "");
  TEST_ERROR(tinyjson::Parse::EXPECT_VALUE, " ");
}

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
  TEST_NUMBER(1.0000000000000002,
              "1.0000000000000002"); /* the smallest number > 1 */
  TEST_NUMBER(4.9406564584124654e-324,
              "4.9406564584124654e-324"); /* minimum denormal */
  TEST_NUMBER(-4.9406564584124654e-324, "-4.9406564584124654e-324");
  TEST_NUMBER(2.2250738585072009e-308,
              "2.2250738585072009e-308"); /* Max subnormal double */
  TEST_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308");
  TEST_NUMBER(2.2250738585072014e-308,
              "2.2250738585072014e-308"); /* Min normal positive double */
  TEST_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308");
  TEST_NUMBER(1.7976931348623157e+308,
              "1.7976931348623157e+308"); /* Max double */
  TEST_NUMBER(-1.7976931348623157e+308, "-1.7976931348623157e+308");
}

static void test_parse_number_too_big() {
  TEST_ERROR(tinyjson::Parse::NUMBER_TOO_BIG, "1e309");
  TEST_ERROR(tinyjson::Parse::NUMBER_TOO_BIG, "-1e309");
}

static void test_parse_invalid_value() {
  TEST_ERROR(tinyjson::Parse::INVALID_VALUE, "+0");
  TEST_ERROR(tinyjson::Parse::INVALID_VALUE, "+1");
  TEST_ERROR(tinyjson::Parse::INVALID_VALUE, ".123");
  TEST_ERROR(tinyjson::Parse::INVALID_VALUE, "1.");
  TEST_ERROR(tinyjson::Parse::INVALID_VALUE, "INF");
  TEST_ERROR(tinyjson::Parse::INVALID_VALUE, "inf");
  TEST_ERROR(tinyjson::Parse::INVALID_VALUE, "NAN");
  TEST_ERROR(tinyjson::Parse::INVALID_VALUE, "nan");
}

static void test_getter_and_setter() {
  TEST_GETTER_NUMBER(0.1, "0.1");
  TEST_GETTER_NUMBER(0.0, "-0");
  TEST_GETTER_NUMBER(0.0, "-0.0");
  TEST_GETTER_NUMBER(1.0, "1");
  TEST_GETTER_NUMBER(-1.0, "-1");
  TEST_GETTER_NUMBER(1.5, "1.5");
  TEST_GETTER_NUMBER(-1.5, "-1.5");
  TEST_GETTER_NUMBER(3.1416, "3.1416");
  TEST_GETTER_NUMBER(1E10, "1E10");
  TEST_GETTER_NUMBER(1e10, "1e10");
  TEST_GETTER_NUMBER(1E+10, "1E+10");
  TEST_GETTER_NUMBER(1E-10, "1E-10");
  TEST_GETTER_NUMBER(-1E10, "-1E10");
  TEST_GETTER_NUMBER(-1e10, "-1e10");
  TEST_GETTER_NUMBER(-1E+10, "-1E+10");
  TEST_GETTER_NUMBER(-1E-10, "-1E-10");

  TEST_GETTER_STRING("123", "\"123\"");
  TEST_GETTER_STRING("null", "\"null\"");
  TEST_GETTER_STRING("nil", "\"nil\"");
  TEST_GETTER_STRING("hello, world\n", "\"hello, world\\n\"");

  TEST_GETTER_BOOLEAN(tinyjson::Type::TRUE, "true");
  TEST_GETTER_BOOLEAN(tinyjson::Type::FALSE, "false");
}

static void test_access_string() {
  TEST_STRING("123", "\"123\"");
  TEST_STRING("null", "\"null\"");
  TEST_STRING("nil", "\"nil\"");
  TEST_STRING("hello, world\n", "\"hello, world\\n\"");
}

static void test_parse_missing_quotation_mark() {
  TEST_ERROR(tinyjson::Parse::MISS_QUOTATION_MARK, "\"");
  TEST_ERROR(tinyjson::Parse::MISS_QUOTATION_MARK, "\"abc");
}

static void test_parse_invalid_string_escape() {
  TEST_ERROR(tinyjson::Parse::INVALID_STRING_ESCAPE, "\"\\v\"");
  TEST_ERROR(tinyjson::Parse::INVALID_STRING_ESCAPE, "\"\\'\"");
  TEST_ERROR(tinyjson::Parse::INVALID_STRING_ESCAPE, "\"\\0\"");
  TEST_ERROR(tinyjson::Parse::INVALID_STRING_ESCAPE, "\"\\x12\"");
}

static void test_parse_invalid_string_char() {
  TEST_ERROR(tinyjson::Parse::INVALID_STRING_CHAR, "\"\x01\"");
  TEST_ERROR(tinyjson::Parse::INVALID_STRING_CHAR, "\"\x1F\"");
}

static void test_parse_invalid_unicode_hex() {
  TEST_ERROR(tinyjson::Parse::INVALID_UNICODE_HEX, "\"\\u\"");
  TEST_ERROR(tinyjson::Parse::INVALID_UNICODE_HEX, "\"\\u0\"");
  TEST_ERROR(tinyjson::Parse::INVALID_UNICODE_HEX, "\"\\u01\"");
  TEST_ERROR(tinyjson::Parse::INVALID_UNICODE_HEX, "\"\\u012\"");
  TEST_ERROR(tinyjson::Parse::INVALID_UNICODE_HEX, "\"\\u/000\"");
  TEST_ERROR(tinyjson::Parse::INVALID_UNICODE_HEX, "\"\\uG000\"");
  TEST_ERROR(tinyjson::Parse::INVALID_UNICODE_HEX, "\"\\u0/00\"");
  TEST_ERROR(tinyjson::Parse::INVALID_UNICODE_HEX, "\"\\u0G00\"");
  TEST_ERROR(tinyjson::Parse::INVALID_UNICODE_HEX, "\"\\u00/0\"");
  TEST_ERROR(tinyjson::Parse::INVALID_UNICODE_HEX, "\"\\u00G0\"");
  TEST_ERROR(tinyjson::Parse::INVALID_UNICODE_HEX, "\"\\u000/\"");
  TEST_ERROR(tinyjson::Parse::INVALID_UNICODE_HEX, "\"\\u000G\"");
  TEST_ERROR(tinyjson::Parse::INVALID_UNICODE_HEX, "\"\\u 123\"");
}

static void test_parse_invalid_unicode_surrogate() {
  TEST_ERROR(tinyjson::Parse::INVALID_UNICODE_SURROGATE, "\"\\uD800\"");
  TEST_ERROR(tinyjson::Parse::INVALID_UNICODE_SURROGATE, "\"\\uDBFF\"");
  TEST_ERROR(tinyjson::Parse::INVALID_UNICODE_SURROGATE, "\"\\uD800\\\\\"");
  TEST_ERROR(tinyjson::Parse::INVALID_UNICODE_SURROGATE, "\"\\uD800\\uDBFF\"");
  TEST_ERROR(tinyjson::Parse::INVALID_UNICODE_SURROGATE, "\"\\uD800\\uE000\"");
}

static void test_parse_array() {
  tinyjson::Value v;
  EXPECT_EQ_INT(tinyjson::Parse::OK,
                v.parse(std::make_shared<std::string>(std::string("[ ]"))));
  EXPECT_EQ_INT(tinyjson::Type::ARRAY, v.get_type());
  EXPECT_EQ_SIZE_T(0, v.get_array_size());

  v = tinyjson::Value();
  EXPECT_EQ_INT(tinyjson::Parse::OK,
                v.parse(std::make_shared<std::string>(
                    std::string("[ null , false , true , 123 , \"abc\" ]"))));
  EXPECT_EQ_INT(tinyjson::Type::ARRAY, v.get_type());
  EXPECT_EQ_SIZE_T(5, v.get_array_size());
  EXPECT_EQ_INT(tinyjson::NIL, v.get_array_elem(0)->get_type());
  EXPECT_EQ_INT(tinyjson::FALSE, v.get_array_elem(1)->get_type());
  EXPECT_EQ_INT(tinyjson::TRUE, v.get_array_elem(2)->get_type());
  EXPECT_EQ_INT(tinyjson::NUMBER, v.get_array_elem(3)->get_type());
  EXPECT_EQ_INT(tinyjson::STRING, v.get_array_elem(4)->get_type());
  EXPECT_EQ_DOUBLE(123.0, v.get_array_elem(3)->get_number());
  EXPECT_EQ_STRING("abc", v.get_array_elem(4)->get_string().c_str(),
                   v.get_array_elem(4)->get_string_len());

  v = tinyjson::Value();
  EXPECT_EQ_INT(tinyjson::Parse::OK,
                v.parse(std::make_shared<std::string>(std::string(
                    "[ [ ] , [ 0 ] , [ 0 , 1 ] , [ 0 , 1 , 2 ] ]"))));
  EXPECT_EQ_INT(tinyjson::Type::ARRAY, v.get_type());
  EXPECT_EQ_SIZE_T(4, v.get_array_size());
  for (int i = 0; i < 4; i++) {
    tinyjson::Value *e = v.get_array_elem(i);
    EXPECT_EQ_INT(tinyjson::Type::ARRAY, e->get_type());
    EXPECT_EQ_SIZE_T(i, e->get_array_size());
    for (int j = 0; j < i; j++) {
      tinyjson::Value *ee = e->get_array_elem(j);
      EXPECT_EQ_INT(tinyjson::Type::NUMBER, ee->get_type());
      EXPECT_EQ_DOUBLE((double)j, ee->get_number());
    }
  }
}

static void test_parse_miss_key() {
  TEST_ERROR(tinyjson::Parse::MISS_KEY, "{:1,");
  TEST_ERROR(tinyjson::Parse::MISS_KEY, "{1:1,");
  TEST_ERROR(tinyjson::Parse::MISS_KEY, "{true:1,");
  TEST_ERROR(tinyjson::Parse::MISS_KEY, "{false:1,");
  TEST_ERROR(tinyjson::Parse::MISS_KEY, "{null:1,");
  TEST_ERROR(tinyjson::Parse::MISS_KEY, "{[]:1,");
  TEST_ERROR(tinyjson::Parse::MISS_KEY, "{{}:1,");
  TEST_ERROR(tinyjson::Parse::MISS_KEY, "{\"a\":1,");
}

static void test_parse_miss_colon() {
  TEST_ERROR(tinyjson::Parse::MISS_COLON, "{\"a\"}");
  TEST_ERROR(tinyjson::Parse::MISS_COLON, "{\"a\",\"b\"}");
}

static void test_parse_miss_comma_or_curly_bracket() {
  TEST_ERROR(tinyjson::Parse::MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1");
  TEST_ERROR(tinyjson::Parse::MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1]");
  TEST_ERROR(tinyjson::Parse::MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1 \"b\"");
  TEST_ERROR(tinyjson::Parse::MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":{}");
}

static void test_parse_object() {
  tinyjson::Value v;
  EXPECT_EQ_INT(tinyjson::Parse::OK,
                v.parse(std::make_shared<std::string>(
                    " { "
                    "\"n\" : null , "
                    "\"f\" : false , "
                    "\"t\" : true , "
                    "\"i\" : 123 , "
                    "\"s\" : \"abc\", "
                    "\"a\" : [ 1, 2, 3 ],"
                    "\"o\" : { \"1\" : 1, \"2\" : 2, \"3\" : 3 }"
                    " } ")));

  EXPECT_EQ_INT(tinyjson::Type::OBJECT, v.get_type());
  EXPECT_EQ_SIZE_T(7, v.get_object_size());
  EXPECT_EQ_STRING("n", v.get_object_key(0).c_str(), v.get_object_key_len(0));
  EXPECT_EQ_INT(tinyjson::Type::NIL, v.get_object_value(0)->get_type());
  EXPECT_EQ_STRING("f", v.get_object_key(1).c_str(), v.get_object_key_len(1));
  EXPECT_EQ_INT(tinyjson::Type::FALSE, v.get_object_value(1)->get_type());
  EXPECT_EQ_STRING("t", v.get_object_key(2).c_str(), v.get_object_key_len(2));
  EXPECT_EQ_INT(tinyjson::Type::TRUE, v.get_object_value(2)->get_type());
  EXPECT_EQ_STRING("i", v.get_object_key(3).c_str(), v.get_object_key_len(3));
  EXPECT_EQ_INT(tinyjson::Type::NUMBER, v.get_object_value(3)->get_type());
  EXPECT_EQ_DOUBLE(123.0, v.get_object_value(3)->get_number());
  EXPECT_EQ_STRING("s", v.get_object_key(4).c_str(), v.get_object_key_len(4));
  EXPECT_EQ_INT(tinyjson::Type::STRING, v.get_object_value(4)->get_type());
  EXPECT_EQ_STRING("abc", v.get_object_value(4)->get_string().c_str(),
                   v.get_object_value(4)->get_string_len());
  EXPECT_EQ_STRING("a", v.get_object_key(5).c_str(), v.get_object_key_len(5));
  EXPECT_EQ_INT(tinyjson::Type::ARRAY, v.get_object_value(5)->get_type());
  EXPECT_EQ_SIZE_T(3, v.get_object_value(5)->get_array_size());
  for (int i = 0; i < 3; i++) {
    tinyjson::Value *e = v.get_object_value(5)->get_array_elem(i);
    EXPECT_EQ_INT(tinyjson::Type::NUMBER, e->get_type());
    EXPECT_EQ_DOUBLE(i + 1.0, e->get_number());
  }
  EXPECT_EQ_STRING("o", v.get_object_key(6).c_str(), v.get_object_key_len(6));
  {
    tinyjson::Value *o = v.get_object_value(6);
    EXPECT_EQ_INT(tinyjson::Type::OBJECT, o->get_type());
    for (int i = 0; i < 3; i++) {
      tinyjson::Value *ov = o->get_object_value(i);
      EXPECT_TRUE('1' + i == o->get_object_key(i).c_str()[0]);
      EXPECT_EQ_SIZE_T(1, o->get_object_key_len(i));
      EXPECT_EQ_INT(tinyjson::NUMBER, ov->get_type());
      EXPECT_EQ_DOUBLE(i + 1.0, ov->get_number());
    }
  }
}

static void test_parse() {
  test_parse_null();
  test_parse_expect_value();
  test_parse_number();
  test_parse_number_too_big();
  test_access_string();
  test_getter_and_setter();
  test_parse_invalid_string_escape();
  test_parse_invalid_string_char();
  test_parse_invalid_unicode_hex();
  test_parse_invalid_unicode_surrogate();
  test_parse_array();
  test_parse_miss_key();
  test_parse_miss_colon();
  test_parse_miss_comma_or_curly_bracket();
  test_parse_object();
}

int main() {
  test_parse();

  if ((test_pass / test_count) < 1) {
    std::printf(F_FYEL "%d/%d (%3.2f%%) passed\n" F_NRM, test_pass, test_count,
                test_pass * 100.0 / test_count);
  } else {
    std::printf(F_FGRN "%d/%d (%3.2f%%) passed\n" F_NRM, test_pass, test_count,
                test_pass * 100.0 / test_count);
  }
}
