# JSON Tokenizer (jsont)

A minimal and portable JSON tokenizer with syntax validation, suitable for parsing JSON directly into custom data structures or to be inspected rather than stored.

`example2.c` shows how this tokenizer can be used to build highly effective and strict parsers.

Build and run unit tests:

    make

## Synopsis

    jsont_ctx_t* S = jsont_create(0);
    jsont_reset(S, uint8_t* inbuf, size_t inbuf_len);
    tok = jsont_next(S)
    // branch on `tok` ...
    V = jsont_*_value(S[, ...]);
    jsont_destroy(S);

## API overview

See `jsont.h` for a complete overview of the API, incuding more detailed documentation. Here's an overview:

### Types

- `jsont_ctx_t` — A tokenizer context ("instance" in OOP lingo.)
- `jsont_tok_t` — A token type (see "Token types".)
- `jsont_err_t` — A user-configurable error type, which defaults to `const char*`.

### Managing a tokenizer context

- `jsont_ctx_t* jsont_create(void* user_data)` — Create a new JSON tokenizer context.
- `void jsont_destroy(jsont_ctx_t* ctx)` — Destroy a JSON tokenizer context.
- `void jsont_reset(jsont_ctx_t* ctx, const uint8_t* bytes, size_t length)` — Reset the tokenizer to parse the data pointed to by `bytes`.

### Dealing with tokens

- `jsont_tok_t jsont_next(jsont_ctx_t* ctx)` — Read and return the next token.
- `jsont_tok_t jsont_current(const jsont_ctx_t* ctx)` — Returns the current token (last token read by `jsont_next`).

### Accessing and comparing values

- `int64_t jsont_int_value(jsont_ctx_t* ctx)` — Returns the current integer value.
- `double jsont_float_value(jsont_ctx_t* ctx)` — Returns the current floating-point number value.
- `size_t jsont_data_value(jsont_ctx_t* ctx, const uint8_t** bytes)` — Returns a slice of the input which represents the current value.
- `char* jsont_strcpy_value(jsont_ctx_t* ctx)` — Retrieve a newly allocated c-string.
- `bool jsont_data_equals(jsont_ctx_t* ctx, const uint8_t* bytes, size_t length)` — Returns true if the current data value is equal to `bytes` of `length`
- `bool jsont_str_equals(jsont_ctx_t* ctx, const char* str)` — Returns true if the current data value is equal to c string `str`.

Note that the data is not parsed until you call one of these functions. This means that if you know that a value transferred as a string will fit in a 64-bit signed integer, it's completely valid to call `jsont_int_value` to parse the string as an integer.

### Miscellaneous

- `uint8_t jsont_current_byte(jsont_ctx_t* ctx)` — Get the last byte read.
- `size_t jsont_current_offset(jsont_ctx_t* ctx)` — Get the current offset of the last byte read.
- `jsont_err_t jsont_error_info(jsont_ctx_t* ctx)` — Get information on the last error.
- `void* jsont_user_data(const jsont_ctx_t* ctx)` — Returns the value passed to `jsont_create`

### Token types

- `JSONT_END` —            Input ended.
- `JSONT_ERR` —            Error. Retrieve details through `jsont_error_info`
- `JSONT_OBJECT_START` —   {
- `JSONT_OBJECT_END` —     }
- `JSONT_ARRAY_START` —    [
- `JSONT_ARRAY_END` —      ]
- `JSONT_TRUE` —           true
- `JSONT_FALSE` —          false
- `JSONT_NULL` —           null
- `JSONT_NUMBER_INT` —     number value without a fraction part (access through `jsont_int_value` or `jsont_float_value`)
- `JSONT_NUMBER_FLOAT` —   number value with a fraction part (access through `jsont_float_value`)
- `JSONT_STRING` —         string value (access through `jsont_data_value` or `jsont_strcpy_value`)
- `JSONT_FIELD_NAME` —     field name (access through `jsont_data_value` or `jsont_strcpy_value`)

## Further reading

- See `example*.c` for working sample programs.
- See `LICENSE` for the MIT-style license under which this project is licensed.
