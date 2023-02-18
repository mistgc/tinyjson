#include "tinyjson.hh"

namespace tinyjson {
#ifndef CONTEXT_STACK_INIT_SIZE
#define CONTEXT_STACK_INIT_SIZE 256
#endif

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

void Value::set_number(double n) {
  this->type = Type::NUMBER;
  this->n = n;
}

double Value::get_number() {
  assert(this->type == Type::NUMBER);
  return this->n;
}

void Value::set_boolean(bool b) {
  if (b)
    this->type = Type::TRUE;
  else
    this->type = Type::FALSE;
}

bool Value::get_boolean() {
  assert(this->type == Type::TRUE || this->type == Type::FALSE);
  if (this->type == Type::TRUE)
    return true;
  else
    return false;
}

void Value::set_string(std::shared_ptr<const std::string> str) {
  this->type = Type::STRING;
  this->s = std::string(str->c_str());
}

void Value::set_cstring(const char *str, size_t len) {
  this->type = Type::STRING;
  this->s = std::string(str, len);
}

size_t Value::get_string_len() {
  assert(this->get_type() == Type::STRING);
  return this->s.length();
}

std::string Value::get_string() {
  assert(this->get_type() == Type::STRING);
  return this->s;
}

void Context::stack_grow() {
  if (this->size != 0)
    this->size += this->size >> 1; /* size = size * 1.5 */
  else
    this->size = CONTEXT_STACK_INIT_SIZE;
  this->stack = (char *)std::realloc(this->stack, this->size);
}

Context::Context() noexcept {
  this->offset = 0;
  this->json = nullptr;
  this->stack = nullptr;
  this->top = this->size = 0;
}

inline void Context::putc(char ch) {
  if (this->top == this->size)
    this->stack_grow();
  this->stack[++top] = ch;
}

inline char Context::popc() {
  assert(this->stack != nullptr);
  char ch = this->stack[top--];
  this->size -= 1;
  return ch;
}

void Context::push(const char *str, size_t len) {
  while (this->top + len > this->size)
    this->stack_grow();
  std::memcpy(this->stack + top + 1, str, len);
}

const char *Context::pop(size_t len) {
  assert(this->stack != nullptr);
  assert(this->top - len >= 0);
  this->top -= len;
  return this->stack + this->top + 1;
}

Context::~Context() {
  if (this->stack != nullptr && this->size > 0) {
    std::free(this->stack);
    this->size = 0;
    this->top = 0;
  }
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

Parse Context::parse_string(Value &v) {
  size_t head = this->top, len, i = this->offset;
  EXPECT((*this->json)[i], &i, '\"');
  while (1) {
    char ch = (*this->json)[i++];
    switch (ch) {
    case '\"':
      len = this->top - head;
      v.set_cstring(this->pop(len), len);
      this->offset = i;
      return Parse::OK;
    case '\0':
      this->top = head;
      return Parse::MISS_QUOTATION_MARK;
    default:
      this->putc(ch);
    }
  }
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

  errno = 0;
  v.n = std::strtod(cstr, &end);
  if (errno == ERANGE && (v.n == HUGE_VAL || v.n == -HUGE_VAL)) {
    v.type = Type::NIL;
    return Parse::NUMBER_TOO_BIG;
  }

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
  case '\"':
    return this->parse_string(v);
  default:
    return this->parse_number(v);
  case '\0':
    return Parse::EXPECT_VALUE;
  }
}

} // namespace tinyjson
