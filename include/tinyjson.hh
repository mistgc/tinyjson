#ifndef _TINYJSON_H_
#define _TINYJSON_H_

#include <memory>
#include <string>

namespace tinyjson {

enum Type { NIL, FALSE, TRUE, NUMBER, STRING, ARRAY, OBJECT };

enum Parse {
  OK = 0,
  EXPECT_VALUE,
  INVALID_VALUE,
  ROOT_NOT_SINGULAR,
  NUMBER_TOO_BIG,
};

class Value {
public:
  Type type;
  double n;

  Parse parse(std::shared_ptr<const std::string> json);
  Type get_type();
  double get_number();
};

class Context {
public:
  std::shared_ptr<const std::string> json;
  int64_t offset;

  Context() noexcept;

  void parse_whitespace();
  Parse parse_value(Value &v);
  Parse parse_true(Value &v);
  Parse parse_false(Value &v);
  Parse parse_null(Value &v);
  Parse parse_literal(Value &v);
  Parse parse_number(Value &v);
};

} // namespace tinyjson
#endif /* _TINYJSON_H_ */
