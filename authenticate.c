#include <stdio.h>      /* printf() */
#include <stdlib.h>     /* getenv() */
#include <string.h>

#include <unistd.h>     /* STDIN_FILENO, read() */

/*
 * largest power of two greater than the sum of the lengths of "username" (8),
 * "=", the maximum length of an email address (254), "&", "password" (8), "="
 * and the maximum length of a password
 */
#define BUFSZ   512

#define strcmp4(s, c0, c1, c2, c3)                                            \
    s[0] == c0 && s[1] == c1 && s[2] == c2 && s[3] == c3

#define strcmp8(s, c0, c1, c2, c3, c4, c5, c6, c7)                            \
    s[0] == c0 && s[1] == c1 && s[2] == c2 && s[3] == c3 && s[4] == c4        \
    && s[5] == c5 && s[6] == c6 && s[7] == c7

typedef struct
{
    char *param_start;
    char *param_end;
    char *value_start;
    char *value_end;
} cgi_parse_t;

/* only for ascii, no utf-8 support */
static void uri_pct_decode(const char *uri, char *buf);

static int cgi_process_content(char *buf);
static int cgi_parse_param(char *buf, cgi_parse_t *param);

/*
 * form data
 *
 * these point to locations in the decoded buffer to avoid string copy
 * operatons and conserve memory
 */
const char *username;
const char *password;

static void
uri_pct_decode(const char *uri, char *buf)
{
    char ch;
    int ascii;
    static const int hex_sym_val[] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0,
        0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    while (ch = *uri++) {
        /* space (reserved character) */
        if (ch == '+') {
            *buf++ = ' ';
            continue;
        }

        /* reserved character */
        if (ch == '%') {
            ascii = (hex_sym_val[*uri++] * 16);
            ascii += (hex_sym_val[*uri++]);

            *buf++ = ascii;
            continue;
        }

        /* unreserved character */
        *buf++ = ch;
    }

    *buf = '\0';
}

static int
cgi_process_content(char *buf)
{
    int rv;
    size_t param_len, value_len;
    char *param;
    char *value;
    cgi_parse_t p;

    for (;;) {
        rv = cgi_parse_param(buf, &p);
        if (rv == -1)
            return -1;

        if (rv == 2)
            return 0;

        param_len = p.param_end - p.param_start;

        /* strlen("username") and strlen("password") are both 8 */
        if (param_len != 8)
            return -1;

        param = p.param_start;
        param[param_len] = '\0';

        value_len = p.value_end - p.value_start;
        value = p.value_start;
        value[value_len] = '\0';

        if (strcmp8(param, 'u', 's', 'e', 'r', 'n', 'a', 'm', 'e')) {
            username = (const char *)value;
            buf = p.value_end + 1;
            continue;
        }

        if (strcmp8(param, 'p', 'a', 's', 's', 'w', 'o', 'r', 'd'))
            password = (const char *)value;

        buf = p.value_end + 1;
    }
}

static int
cgi_parse_param(char *buf, cgi_parse_t *param)
{
    unsigned char ch;
    char *p;
    enum {
        sw_start,
        sw_param,
        sw_equal_sign,
        sw_value
    } state;

    state = sw_start;

    for (p = buf; p; p++) {
        ch = *p;

        switch (state) {
            case sw_start:
                if (ch == '\0')
                    return 2;

                param->param_start = p;
                state = sw_param;
                break;

            case sw_param:
                if (ch == '=') {
                    param->param_end = p;
                    state = sw_equal_sign;
                }
                break;

            case sw_equal_sign:
                if (ch == '\0')
                    return -1;

                param->value_start = p;
                state = sw_value;
                break;

            case sw_value:
                switch (ch) {
                    case '&':   /* FALLTHROUGH */
                    case '\0':
                        param->value_end = p;
                        goto done;
                }
                break;
        }
    }

done:

    return 0;
}

int
main(void)
{
    int rv;

    char encoded_buf[BUFSZ];
    char decoded_buf[BUFSZ];

    /* parsing */
    char *param;
    char *value;

    size_t content_length_n;

    /* environment variables */
    const char *method;
    const char *content_length;
    const char *http_cookie;

    method = getenv("REQUEST_METHOD");
    if (method == NULL)
        return 1;

    /*
     * IETF RFC 3875 6.1 says, "The script MUST check the REQUEST_METHOD
     * variable when processing the request and preparing its response."
     */

    /*
     * Environment variable strings are null-terminated so short-circuiting of
     * the logical AND (&&) will prevent out-of-bounds on the array.  However,
     * note that this a false positive if "POST" is a prefix of method.
     */
    if (!strcmp4(method, 'P', 'O', 'S', 'T')) {
        printf(
            "Content-Type:text/plain\r\n"
            "Status:501 Not Implemented\r\n"
            "\r\n"
            "%s\n"
            "unsupported HTTP method",
            password
        );

        return 1;
    }

    content_length = getenv("CONTENT_LENGTH");
    if (content_length == NULL)
        return 1;

    /*
     * IETF RFC 3875 4.3.2 says, "The script MUST check the value of the
     * CONTENT_LENGTH variable before reading the attached message-body, ..."
     */
    /* TODO: replace deprecated atoi() */
    content_length_n = atoi(content_length);
    if (content_length_n > (sizeof encoded_buf) - 1) {
        printf(
            "Content-Type:text/plain\r\n"
            "Status:413 Request Entity Too Large\r\n"
            "\r\n"
            "%s\n"
            "too much form data",
            password
        );

        return 1;
    }

    /*
     * IETF RFC 3875 says, "The server MUST make at least that many
     * [CONTENT_LENGTH] bytes available for the script to read."
     */
    (void) read(STDIN_FILENO, encoded_buf, content_length_n);

    encoded_buf[content_length_n] = '\0';

#if 0
    /*
     * IETF RFC 3875 4.3.2 says, "..., and SHOULD check the CONTENT_TYPE
     * value before processing it [the attached message-body]."
     */
     content_type = getenv("CONTENT_TYPE");
     if (strcmp("application/x-www-form-urlencoded", content_type != 0))
        /* TODO */;
#endif

    uri_pct_decode(encoded_buf, decoded_buf);

    rv = cgi_process_content(decoded_buf);
    if (rv == -1)
        return 1;

    if (strcmp("apple", password) == 0) {
        printf(
            "Set-Cookie:sessionId=; HttpOnly\r\n"
            "Location:../index.html\r\n"
            "\r\n"
        );
    } else {
        printf(
            "Content-Type: text/plain\r\n"
            "\r\n"
            "%s\n"
            "incorrect credentials",
            password
        );
    }

    return 0;
}
