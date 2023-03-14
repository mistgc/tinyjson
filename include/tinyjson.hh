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
  MISS_COMMA_OR_SQUARE_BRACKET,
  MISS_KEY,
  MISS_COLON,
  MISS_COMMA_OR_CURLY_BRACKET,
  INVALID_STRING_ESCAPE,
  INVALID_STRING_CHAR,
  INVALID_UNICODE_HEX,
  INVALID_UNICODE_SURROGATE,
};

class Member;

class Value {
public:
  Type type;

  /* string */
  std::string s;
  /* number */
  double n;
  /* array */
  union {
    struct {
      Value **elems;
      size_t array_len;
    };
    struct {
      Member **members;
      size_t members_len;
    };
  };

  Value() {
    this->type = Type::NIL;
    this->n = 0;
    this->elems = nullptr;
    this->array_len = 0;
  }

  ~Value() {
    if (this->type == Type::ARRAY) {
      for (int i = 0; i < this->array_len; i++) {
        delete elems[i];
      }
      delete elems;
      elems = nullptr;
    } else if (this->type == Type::OBJECT) {
      for (int i = 0; i < this->members_len; i++) {
        delete members[i];
      }
      delete members;
      members = nullptr;
    }
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

  size_t get_array_size();
  Value *get_array_elem(size_t index);

  size_t get_object_size();
  Value *get_object_value(size_t index);
  std::string get_object_key(size_t index);
  size_t get_object_key_len(size_t index);

  Type get_type();
};

class Member {
public:
  std::string key;
  Value value;

  Member() {}
  Member(std::string k, Value v) : key(k), value(v) {}

  std::string get_key();
  size_t get_key_len();
  Value get_value();
};

class Context {
private:
  char *stack;
  size_t size, top;

  void stack_grow();
  void stack_grow_size(size_t len);

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
  Parse parse_string_raw(char **str, size_t *strlen);
  Parse parse_string(Value &v);
  Parse parse_value(Value &v);
  Parse parse_true(Value &v);
  Parse parse_false(Value &v);
  Parse parse_null(Value &v);
  Parse parse_literal(Value &v);
  Parse parse_number(Value &v);
  Parse parse_hex4(int64_t *offset, uint32_t *u);
  Parse parse_array(Value &v);
  Parse parse_object(Value &v);
  void encode_utf8(uint32_t u);
};

} // namespace tinyjson
#endif /* _TINYJSON_H_ */
