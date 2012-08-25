// JSON Tokenizer. Copyright (c) 2012, Rasmus Andersson. All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <ctype.h> // isdigit
#include <errno.h>
#include <string.h>
#include <math.h>

// Error info
#ifndef JSONT_ERRINFO_CUSTOM
#define JSONT_ERRINFO_T const char*
#define DEF_EM(NAME, msg) static JSONT_ERRINFO_T JSONT_ERRINFO_##NAME = msg
DEF_EM(STACK_SIZE, "Stack size limit exceeded");
DEF_EM(UNEXPECTED_OBJECT_END,
  "Unexpected end of object while not in an object");
DEF_EM(UNEXPECTED_ARRAY_END, "Unexpected end of array while not in an array");
DEF_EM(UNEXPECTED_COMMA, "Unexpected \",\"");
DEF_EM(UNEXPECTED_COLON, "Unexpected \":\"");
DEF_EM(UNEXPECTED, "Unexpected input");
#undef DEF_EM
#endif

// Size of stack used for structures (in/out array and objects). This value
// is a balance between memory size of a ctx and how many levels deep the
// tokenizer can go.
#define _STRUCT_TYPE_STACK_SIZE 512

typedef uint8_t jsont_tok_t;

typedef struct jsont_ctx {
  void* user_data;
  const uint8_t* input_buf;
  const uint8_t* input_buf_ptr;
  size_t input_len;
  const uint8_t* input_buf_value_start;
  const uint8_t* input_buf_value_end;
  JSONT_ERRINFO_T error_info;
  jsont_tok_t curr_tok;
  size_t st_stack_size;
  size_t st_stack_len;
  jsont_tok_t st_stack[/**_STRUCT_TYPE_STACK_SIZE*/];
} jsont_ctx_t;  // 585 bytes on 64-bit arch

#define _JSONT_IN_SOURCE
#include <jsont.h>

jsont_ctx_t* jsont_create(void* user_data) {
  jsont_ctx_t* ctx = (jsont_ctx_t*)calloc(1,
    sizeof(jsont_ctx_t) + _STRUCT_TYPE_STACK_SIZE);
  ctx->user_data = user_data;
  ctx->st_stack_size = _STRUCT_TYPE_STACK_SIZE;
  return ctx;
}

void jsont_destroy(jsont_ctx_t* ctx) {
  free(ctx);
}

void jsont_reset(jsont_ctx_t* ctx, const uint8_t* bytes, size_t length) {
  ctx->input_buf_ptr = ctx->input_buf = bytes;
  ctx->input_len = length;
  ctx->st_stack_len = 0;
  ctx->curr_tok = JSONT_END;
  ctx->input_buf_value_start = 0;
  ctx->input_buf_value_end = 0;
  ctx->error_info = 0;
}

jsont_tok_t jsont_current(const jsont_ctx_t* ctx) {
  return ctx->curr_tok;
}

void* jsont_user_data(const jsont_ctx_t* ctx) {
  return ctx->user_data;
}

// Get the current/last byte read. Suitable for debugging JSONT_ERR
uint8_t jsont_current_byte(jsont_ctx_t* ctx) {
  return (ctx->input_buf_ptr == 0) ? 0 : *(ctx->input_buf_ptr-1);
}

size_t jsont_current_offset(jsont_ctx_t* ctx) {
  return ctx->input_buf_ptr - ctx->input_buf;
}

JSONT_ERRINFO_T jsont_error_info(jsont_ctx_t* ctx) {
  return ctx->error_info;
}

inline static bool _no_value(jsont_ctx_t* ctx) {
  return ctx->input_buf_value_start == 0
      || ctx->curr_tok < _JSONT_VALUES_START
      || ctx->curr_tok > _JSONT_VALUES_END;
}

inline static size_t _input_avail(jsont_ctx_t* ctx) {
  return ctx->input_len - (ctx->input_buf_ptr - ctx->input_buf);
}

inline static uint8_t _next_byte(jsont_ctx_t* ctx) {
  return (_input_avail(ctx) == 0) ? 0 : *(ctx->input_buf_ptr++);
}

inline static jsont_tok_t _st_stack_top(const jsont_ctx_t* ctx) {
  return (ctx->st_stack_len != 0) ? ctx->st_stack[ctx->st_stack_len-1]
                                  : JSONT_END;
}

size_t jsont_data_value(jsont_ctx_t* ctx, const uint8_t** bytes) {
  if (_no_value(ctx)) {
    return 0;
  } else {
    *bytes = ctx->input_buf_value_start;
    return ctx->input_buf_value_end - ctx->input_buf_value_start;
  }
}

char* jsont_strcpy_value(jsont_ctx_t* ctx) {
  if (_no_value(ctx)) {
    return 0;
  } else {
    size_t len = ctx->input_buf_value_end - ctx->input_buf_value_start;
    char* buf = (char*)malloc(len+1);
    if (memcpy((void*)buf, (const void*)ctx->input_buf_value_start, len) != buf)
      return 0;
    buf[len] = 0;
    return buf;
  }
}

int64_t jsont_int_value(jsont_ctx_t* ctx) {
  if (_no_value(ctx))
    return INT64_MIN;

  const uint8_t* start = ctx->input_buf_value_start;
  const uint8_t* end = ctx->input_buf_value_end+1;

  bool negative;
  uint8_t b = *start++;
  const int base = 10;

  if (b == '-') {
    negative = true;
    b = *start++;
    if (start == end) {
      errno = EINVAL;
      return INT64_MIN;
    }
  } else {
    negative = false;
    if (b == '+') {
      b = *start++;
      if (start == end) {
        errno = EINVAL;
        return INT64_MIN;
      }
    }
  }

  uint64_t acc = 0;
  int any = 0;
  uint64_t cutoff = negative ? (uint64_t)-(LLONG_MIN + LLONG_MAX) + LLONG_MAX
                             : LLONG_MAX;
  int cutlim = cutoff % base;
  cutoff /= base;
  for ( ; start != end; b = *start++) {
    if (b >= '0' && b <= '9') b -= '0'; else break;
    if (any < 0 || acc > cutoff || (acc == cutoff && b > cutlim)) {
      any = -1;
    } else {
      any = 1;
      acc *= base;
      acc += b;
    }
  }

  if (any < 0) {
    acc = negative ? LLONG_MIN : LLONG_MAX;
    errno = ERANGE;
  } else if (!any) {
    errno = EINVAL;
    return INT64_MIN;
  } else if (negative) {
    acc = -acc;
  }

  return (int64_t)acc;
}

#ifdef NAN
  #define _JSONT_NAN NAN
#else
  #define _JSONT_NAN nan(0)
#endif

double jsont_float_value(jsont_ctx_t* ctx) {
  // Note: This might cause a segfault if the input is at the end, so we cause
  // an error if we try to read a float value while at the end of the input.
  if (_no_value(ctx) || _input_avail(ctx) == 0) {
    errno = EINVAL;
    return _JSONT_NAN;
  }
  return atof((const char*)ctx->input_buf_value_start);
}

inline static jsont_tok_t _set_tok(jsont_ctx_t* ctx, jsont_tok_t tok) {
  ctx->curr_tok = tok;

  if (tok != JSONT_END) {
    if (tok == JSONT_OBJECT_START) {
      if (ctx->st_stack_len == ctx->st_stack_size) {
        ctx->error_info = JSONT_ERRINFO_STACK_SIZE;
        return ctx->curr_tok = JSONT_ERR; // TODO: Grow st_stack
      }
       ctx->st_stack[ctx->st_stack_len++] = JSONT_OBJECT_START;

    } else if (tok == JSONT_OBJECT_END) {
      if (_st_stack_top(ctx) != JSONT_OBJECT_START) {
        ctx->error_info = JSONT_ERRINFO_UNEXPECTED_OBJECT_END;
        return ctx->curr_tok = JSONT_ERR;
      }
      --ctx->st_stack_len;

    } else if (tok == JSONT_ARRAY_START) {
      if (ctx->st_stack_len == ctx->st_stack_size) {
        ctx->error_info = JSONT_ERRINFO_STACK_SIZE;
        return ctx->curr_tok = JSONT_ERR;
      }
       ctx->st_stack[ctx->st_stack_len++] = JSONT_ARRAY_START;

    } else if (tok == JSONT_ARRAY_END) {
      if (_st_stack_top(ctx) != JSONT_ARRAY_START) {
        ctx->error_info = JSONT_ERRINFO_UNEXPECTED_ARRAY_END;
        return ctx->curr_tok = JSONT_ERR;
      }
      --ctx->st_stack_len;
    }
  }

  return tok;
}
inline static void _rewind_one_byte(jsont_ctx_t* ctx) {
  --ctx->input_buf_ptr;
}
inline static void _rewind_bytes(jsont_ctx_t* ctx, size_t n) {
  ctx->input_buf_ptr -= n;
}
inline static void _skip_bytes(jsont_ctx_t* ctx, size_t n) {
  ctx->input_buf_ptr += n;
}
inline static uint8_t _read_atom(jsont_ctx_t* ctx, size_t slacklen,
                                 jsont_tok_t tok) {
  if (_input_avail(ctx) < slacklen) {
    // rewind and wait for buffer fill
    _rewind_one_byte(ctx);
    return _set_tok(ctx, JSONT_END);
  } else {
    _skip_bytes(ctx, slacklen); // e.g. "ull" after "n" or "alse" after "f"
    return _set_tok(ctx, tok);
  }
}
inline static bool _expects_field_name(jsont_ctx_t* ctx) {
  return (   ctx->curr_tok == JSONT_OBJECT_START
          || (   ctx->curr_tok == _JSONT_COMMA
              && _st_stack_top(ctx) == JSONT_OBJECT_START) );
}

jsont_tok_t jsont_next(jsont_ctx_t* ctx) {
  //
  // { } [ ] n t f "
  //         | | | |
  //         | | | +- /[^"]*/ "
  //         | | +- a l s e
  //         | +- r u e
  //         +- u l l
  //
  while (1) {
    uint8_t b = _next_byte(ctx);
    switch (b) {
      case '{': return _set_tok(ctx, JSONT_OBJECT_START);
      case '}': return _set_tok(ctx, JSONT_OBJECT_END);
      case '[': return _set_tok(ctx, JSONT_ARRAY_START);
      case ']': return _set_tok(ctx, JSONT_ARRAY_END);
      case 'n': return _read_atom(ctx, 3, JSONT_NULL);
      case 't': return _read_atom(ctx, 3, JSONT_TRUE);
      case 'f': return _read_atom(ctx, 4, JSONT_FALSE);
      case '"': {
        ctx->input_buf_value_start = ctx->input_buf_ptr;
        uint8_t prev_b = 0;
        while (1) {
          b = _next_byte(ctx);
          if (b == '"' && prev_b != '\\') {
            ctx->input_buf_value_end = ctx->input_buf_ptr-1; // -1 b/c of the "
            return _set_tok(ctx, _expects_field_name(ctx)
              ? JSONT_FIELD_NAME : JSONT_STRING);
          } else if (b == 0) {
            // Input buffer ends in the middle of a string
            _rewind_bytes(ctx, ctx->input_buf_ptr
                               - (ctx->input_buf_value_start-1));
            return _set_tok(ctx, JSONT_END);
          }
          prev_b = b;
        }
      }
      case ',':
        if (   ctx->curr_tok == JSONT_OBJECT_START
            || ctx->curr_tok == JSONT_ARRAY_START
            || ctx->curr_tok == JSONT_END
            || ctx->curr_tok == JSONT_ERR) {
          if (ctx->curr_tok != JSONT_ERR)
            ctx->error_info = JSONT_ERRINFO_UNEXPECTED_COMMA;
          return _set_tok(ctx, JSONT_ERR);
        }
        _set_tok(ctx, _JSONT_COMMA);
        // read next by simply letting the outer "while" do its thing
        break;

      case ':':
        if (ctx->curr_tok != JSONT_FIELD_NAME) {
          ctx->error_info = JSONT_ERRINFO_UNEXPECTED_COLON;
          return _set_tok(ctx, JSONT_ERR);
        }
        // let the outer "while" do its thing
        break;

      case ' ': case '\r': case '\n': case '\t':
        // ignore whitespace and let the outer "while" do its thing
        break;

      case 0:
        //printf("** %d\n", __LINE__);
        return _set_tok(ctx, JSONT_END);

      default:
        if (isdigit((int)b) || b == '+' || b == '-') {
          // We are reading a number
          ctx->input_buf_value_start = ctx->input_buf_ptr-1;
          //uint8_t prev_b = 0;
          bool is_float = false;
          while (1) {
            b = _next_byte(ctx);
            if (b == '.') {
              is_float = true;
            } else if (!isdigit((int)b)) {
              _rewind_one_byte(ctx);
              ctx->input_buf_value_end = ctx->input_buf_ptr;
              return _set_tok(ctx, is_float ? JSONT_NUMBER_FLOAT
                                            : JSONT_NUMBER_INT);
            } else if (b == 0) {
              // Input buffer ends before we know that the number-value ended
              _rewind_bytes(ctx, ctx->input_buf_ptr
                                 - (ctx->input_buf_value_start-1));
              return _set_tok(ctx, JSONT_END);
            }
          }
        }

        ctx->error_info = JSONT_ERRINFO_UNEXPECTED;
        return _set_tok(ctx, JSONT_ERR);
    }
  } // while (1)
}

