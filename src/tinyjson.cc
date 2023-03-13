#include "tinyjson.hh"
#include <cassert>

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

size_t Value::get_array_size() {
  assert(Type::ARRAY == this->type);
  return this->array_len;
}

Value *Value::get_array_elem(size_t index) {
  assert(Type::ARRAY == this->type);
  assert(this->elems != nullptr);
  assert(index < this->array_len);
  return this->elems[index];
}

void Context::stack_grow() {
  if (this->size != 0)
    this->size += this->size >> 1; /* size = size * 1.5 */
  else
    this->size = CONTEXT_STACK_INIT_SIZE;
  this->stack = (char *)std::realloc(this->stack, this->size);
}

void Context::stack_grow_size(size_t len) {
  while (this->top + len > this->size)
    this->stack_grow();
}

Context::Context() noexcept {
  this->offset = 0;
  this->json = nullptr;
  this->stack = nullptr;
  this->top = this->size = 0;
}

Context::~Context() {
  if (this->stack != nullptr) {
    free(this->stack);
    this->stack = nullptr;
  }
  this->top = this->size = 0;
}

inline void Context::putc(char ch) {
  if (this->top == this->size)
    this->stack_grow();
  this->stack[top++] = ch;
}

inline char Context::popc() {
  assert(this->stack != nullptr);
  char ch = this->stack[top--];
  this->size -= 1;
  return ch;
}

void Context::push(const char *str, size_t len) {
  this->stack_grow_size(len);
  std::memcpy(this->stack + this->top, str, len);
}

const char *Context::pop(size_t len) {
  assert(this->stack != nullptr);
  assert(this->top - len >= 0);
  this->top -= len;
  return this->stack + this->top;
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
  size_t head = this->top, len;
  int64_t i = this->offset;
  EXPECT((*this->json)[i], &i, '\"');
  while (1) {
    char ch = (*this->json)[i++];
    switch (ch) {
    case '\"':
      len = this->top - head;
      v.set_cstring(this->pop(len), len);
      this->offset = i;
      return Parse::OK;
    case '\\':
      ch = (*this->json)[i++];
      switch (ch) {
      case '\"':
        this->putc('\"');
        break;
      case '\\':
        this->putc('\\');
        break;
      case '/':
        this->putc('/');
        break;
      case 'b':
        this->putc('\b');
        break;
      case 'f':
        this->putc('\f');
        break;
      case 'n':
        this->putc('\n');
        break;
      case 'r':
        this->putc('\r');
        break;
      case 't':
        this->putc('\t');
        break;
      case 'u':
        uint32_t u, u2;
        if (this->parse_hex4(&i, &u) == Parse::INVALID_UNICODE_HEX) {
          this->top = head;
          return Parse::INVALID_UNICODE_HEX;
        }
        if (u >= 0xD800 && u <= 0xDBFF) {
          if ((*this->json)[i++] != '\\' || (*this->json)[i++] != 'u') {
            this->top = head;
            return Parse::INVALID_UNICODE_SURROGATE;
          }
          if (this->parse_hex4(&i, &u2) == Parse::INVALID_UNICODE_HEX) {
            this->top = head;
            return Parse::INVALID_UNICODE_HEX;
          }
          if (u2 < 0xDC00 || u2 > 0xDFFF) {
            this->top = head;
            return Parse::INVALID_UNICODE_SURROGATE;
          }
          u = (((u - 0xD800) << 10) | (u2 - 0xDC00)) + 0x10000;
        }
        this->encode_utf8(u);
        break;
      default:
        this->top = head;
        return Parse::INVALID_STRING_ESCAPE;
      }
      break;
    case '\0':
      this->top = head;
      return Parse::MISS_QUOTATION_MARK;
    default:
      if ((unsigned char)ch < 0x20) {
        this->top = head;
        return Parse::INVALID_STRING_CHAR;
      }
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
  p = cstr + this->offset;
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
  v.n = std::strtod(cstr + this->offset, &end);
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
  if ((*this->json)[i++] != 'r' || (*this->json)[i++] != 'u' ||
      (*this->json)[i++] != 'e')
    return Parse::INVALID_VALUE;
  v.type = Type::TRUE;
  this->offset = i;
  return Parse::OK;
}

Parse Context::parse_false(Value &v) {
  size_t i = this->offset;
  EXPECT((*this->json)[i], &i, 'f');
  if ((*this->json)[i++] != 'a' || (*this->json)[i++] != 'l' ||
      (*this->json)[i++] != 's' || (*this->json)[i++] != 'e')
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
  case '[':
    return this->parse_array(v);
  default:
    return this->parse_number(v);
  case '\0':
    return Parse::EXPECT_VALUE;
  }
}

Parse Context::parse_hex4(int64_t *offset, uint32_t *u) {
  *u = 0;
  for (int i = 0; i < 4; i++) {
    *u <<= 4;
    char ch = (*this->json)[*offset + i];
    if (ch >= '0' && ch <= '9')
      *u |= ch - '0';
    else if (ch >= 'A' && ch <= 'F')
      *u |= ch - 'A' + 10;
    else if (ch >= 'a' && ch <= 'f')
      *u |= ch - 'a' + 10;
    else
      return Parse::INVALID_UNICODE_HEX;
  }
  *offset += 4;
  return Parse::OK;
}

Parse Context::parse_array(Value &v) {
  int64_t i = this->offset;
  size_t size = 0;
  Parse ret;
  EXPECT((*this->json)[i], &i, '[');
  this->offset = i;
  this->parse_whitespace();
  i = this->offset;
  // Handle empty array
  if ((*this->json)[i] == ']') {
    i++;
    this->offset = i;
    v.type = Type::ARRAY;
    v.array_len = 0;
    v.elems = nullptr;
    return Parse::OK;
  }
  while (1) {
    Value *e = new Value();
    if ((ret = this->parse_value(*e)) != Parse::OK) {
      return ret;
    }
    i = this->offset;
    this->stack_grow_size(sizeof(Value *));
    std::memcpy(this->stack + this->top, &e, sizeof(Value *));
    this->top += sizeof(Value *);
    size++;
    this->parse_whitespace();
    i = this->offset;
    if ((*this->json)[i] == ',') {
      i++;
      this->offset = i;
      this->parse_whitespace();
      i = this->offset;
    } else if ((*this->json)[i] == ']') {
      i++;
      v.type = Type::ARRAY;
      v.array_len = size;
      size *= sizeof(Value *);
      v.elems = (Value **)std::malloc(size);
      std::memcpy(v.elems, this->pop(size), size);
      this->offset = i;
      return Parse::OK;
    } else {
      return Parse::MISS_COMMA_OR_SQUARE_BRACKET;
    }
  }

  return Parse::OK;
}

void Context::encode_utf8(uint32_t u) {
  if (u <= 0x7F)
    this->putc(u & 0xFF);
  else if (u <= 0x7FF) {
    this->putc(0xC0 | (u >> 6 & 0xFF));
    this->putc(0x80 | (u & 0x3F));
  } else if (u <= 0xFFFF) {
    this->putc(0xE0 | (u >> 12 & 0xFF));
    this->putc(0x80 | (u >> 6 & 0x3F));
    this->putc(0x80 | (u & 0x3F));
  } else {
    assert(u <= 0x10FFFF);
    this->putc(0xF0 | (u >> 18 & 0xFF));
    this->putc(0x80 | (u >> 12 & 0x3F));
    this->putc(0x80 | (u >> 6 & 0x3F));
    this->putc(0x80 | (u & 0x3F));
  }
}

} // namespace tinyjson
