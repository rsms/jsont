# JSON Tokenizer (jsont)

A minimal and portable JSON tokenizer with syntax validation, suitable for parsing JSON directly into custom data structures or to be inspected rather than stored.

`example2.c` shows how this tokenizer can be used to build highly effective and strict parsers.

## Synopsis

    jsont_ctx_t* S = jsont_create(0);
    jsont_reset(S, uint8_t* inbuf, size_t inbuf_len);
    tok = jsont_next(S)
    // branch on `tok` ...
    V = jsont_*_value(S[, ...]);
    jsont_destroy(S);

## Further reading

- See `jsont.h` for a complete overview of the API, incuding documentation.
- See `example*.c` for working sample programs.
- See `LICENSE` for the MIT-style license under which this project is licensed.
