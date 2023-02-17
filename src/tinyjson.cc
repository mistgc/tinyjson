#include "tinyjson.hh"
#include <cassert>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <iostream>

namespace tinyjson {

#define EXPECT(c, idx, ch)                                                     \
  do {                                                                         \
    assert((c) == (ch));                                                       \
    (*idx)++;                                                                  \
  } while (0)

#define ISDIGIT(ch) ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch) ((ch) >= '1' && (ch) <= '9')

Parse Value::parse(std::shared_ptr<const std::string> json) {
  Context c;
  c.json = json;
  this->type = Type::NIL;
  c.parse_whitespace();
  return c.parse_value(*this);
};

Type Value::get_type() { return this->type; }

Context::Context() noexcept {
  this->offset = 0;
  this->json = nullptr;
}

void Context::parse_whitespace() {
  size_t i = this->offset;
  while ((*this->json)[i] == ' ' || (*this->json)[i] == '\t' ||
         (*this->json)[i] == '\n' || (*this->json)[i] == '\r')
    i++;
  this->offset = i;
}

Parse Context::parse_null(Value &v) {
  size_t i = this->offset;
  EXPECT((*this->json)[i], &i, 'n');
  if ((*this->json)[i++] != 'u' || (*this->json)[i++] != 'l' ||
      (*this->json)[i++] != 'l')
    return Parse::INVALID_VALUE;
  this->offset = i;
  v.type = Type::NIL;
  return Parse::OK;
}

/*
 *  In C++, whether the function std::stold() do underflow is uncertain.
 *  So that we use the C function std::strtold().
 */
Parse Context::parse_number(Value &v) {
  // size_t offset = static_cast<size_t>(this->offset);
  //
  // try {
  //   v.n = std::stold(*this->json, &offset);
  // } catch (const std::invalid_argument &e) {
  //   std::fprintf(stderr, "%s", e.what());
  //   return Parse::INVALID_VALUE;
  // } catch (const std::out_of_range &e) {
  //   if ((*this->json)[this->offset] != '-') {
  //     std::fprintf(stderr, "%s", e.what());
  //     this->offset = offset;
  //     v.type = Type::NUMBER;
  //     v.n = 0.0;
  //     return Parse::NUMBER_TOO_BIG;
  //   }
  // }
  //
  // this->offset = offset;
  // v.type = Type::NUMBER;
  // return Parse::OK;

  char *end;
  const char *p, *cstr = (*this->json).c_str();
  p = cstr;
  if (*p == '-')
    p++;
  if (*p == '0')
    p++;
  else {
    if (!ISDIGIT1TO9(*p))
      return Parse::INVALID_VALUE;
    for (p++; ISDIGIT(*p); p++)
      ;
  }
  if (*p == '.') {
    p++;
    if (!ISDIGIT(*p))
      return Parse::INVALID_VALUE;
    for (p++; ISDIGIT(*p); p++)
      ;
  }
  if (*p == 'e' || *p == 'E') {
    p++;
    if (*p == '+' || *p == '-')
      p++;
    if (!ISDIGIT(*p))
      return Parse::INVALID_VALUE;
    for (p++; ISDIGIT(*p); p++)
      ;
  }

  v.n = std::strtold(cstr, &end);

  errno = 0;
  if (errno == ERANGE && (v.n == HUGE_VAL || v.n == -HUGE_VAL))
    return Parse::NUMBER_TOO_BIG;

  size_t offset = end - cstr;
  this->offset = offset;
  v.type = Type::NUMBER;
  return Parse::OK;
}

Parse Context::parse_true(Value &v) {
  size_t i = this->offset;
  EXPECT((*this->json)[i], &i, 't');
  if ((*this->json)[i++] != 'r' && (*this->json)[i++] != 'u' &&
      (*this->json)[i++] != 'e')
    return Parse::INVALID_VALUE;
  v.type = Type::TRUE;
  this->offset = i;
  return Parse::OK;
}

Parse Context::parse_false(Value &v) {
  size_t i = this->offset;
  EXPECT((*this->json)[i], &i, 'f');
  if ((*this->json)[i++] != 'a' && (*this->json)[i++] != 'l' &&
      (*this->json)[i++] != 's' && (*this->json)[i++] != 'e')
    return Parse::INVALID_VALUE;
  v.type = Type::FALSE;
  this->offset = i;
  return Parse::OK;
}

Parse Context::parse_literal(Value &v) {
  switch ((*this->json)[this->offset]) {
  case 'n':
    return this->parse_null(v);
  case 't':
    return this->parse_true(v);
  case 'f':
    return this->parse_false(v);
  default:
    return Parse::EXPECT_VALUE;
  }
}

Parse Context::parse_value(Value &v) {
  switch ((*this->json)[this->offset]) {
  case 'n':
  case 't':
  case 'f':
    return this->parse_literal(v);
  default:
    return this->parse_number(v);
  case '\0':
    return Parse::EXPECT_VALUE;
  }
}

double Value::get_number() {
  assert(this->type == Type::NUMBER);
  return this->n;
}

} // namespace tinyjson
