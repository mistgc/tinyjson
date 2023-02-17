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
      std::fprintf(stderr, "%s:%d expect: " format " actual:" format "\n",     \
                   __FILE__, __LINE__, expect, actual);                        \
      main_ret = 1;                                                            \
    }                                                                          \
  } while (0)

#define EXPECT_EQ_INT(expect, actual)                                          \
  EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")

#define EXPECT_EQ_DOUBLE(expect, actual)                                       \
  EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%f")

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

static void test_parse() {
  test_parse_null();
  test_parse_expect_value();
  test_parse_number();
  test_parse_number_too_big();
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
