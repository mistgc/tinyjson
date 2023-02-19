#ifndef _TINYJSON_H_
#define _TINYJSON_H_

#include <cassert>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>

namespace tinyjson {

enum Error {
  NON_STRING = -1,
};

enum Type { NIL, FALSE, TRUE, NUMBER, STRING, ARRAY, OBJECT };

enum Parse {
  OK = 0,
  EXPECT_VALUE,
  INVALID_VALUE,
  ROOT_NOT_SINGULAR,
  NUMBER_TOO_BIG,
  MISS_QUOTATION_MARK,
  INVALID_STRING_ESCAPE,
  INVALID_STRING_CHAR,
  INVALID_UNICODE_HEX,
  INVALID_UNICODE_SURROGATE,
};

class Value {
public:
  Type type;

  /* string */
  std::string s;
  /* number */
  double n;

  Value() {
    this->type = Type::NIL;
    this->n = 0;
  }

  Parse parse(std::shared_ptr<const std::string> json);

  void set_string(std::shared_ptr<const std::string> str);
  void set_cstring(const char *str, size_t len);
  size_t get_string_len();
  std::string get_string();

  void set_boolean(bool b);
  bool get_boolean();

  void set_number(double n);
  double get_number();

  Type get_type();
};

class Context {
private:
  char *stack;
  size_t size, top;

  void stack_grow();
public:
  std::shared_ptr<const std::string> json;
  int64_t offset;

  Context() noexcept;
  ~Context();

  inline void putc(char ch);
  inline char popc();
  void push(const char *str, size_t len);
  const char *pop(size_t len);

  void parse_whitespace();
  Parse parse_string(Value &v);
  Parse parse_value(Value &v);
  Parse parse_true(Value &v);
  Parse parse_false(Value &v);
  Parse parse_null(Value &v);
  Parse parse_literal(Value &v);
  Parse parse_number(Value &v);
  Parse parse_hex4(int64_t *offset, uint32_t *u);
  void encode_utf8(uint32_t u);
};

} // namespace tinyjson
#endif /* _TINYJSON_H_ */
