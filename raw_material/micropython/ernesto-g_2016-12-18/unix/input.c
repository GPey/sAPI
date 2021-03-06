/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "py/mpstate.h"
#include "py/mphal.h"
#include "input.h"

#if MICROPY_USE_READLINE == 1
#include "lib/mp-readline/readline.h"
#elif MICROPY_USE_READLINE == 2
#include <readline/readline.h>
#include <readline/history.h>
#include <readline/tilde.h>
#endif

char *prompt(char *p) {
#if MICROPY_USE_READLINE == 1
    // MicroPython supplied readline
    vstr_t vstr;
    vstr_init(&vstr, 16);
    mp_hal_stdio_mode_raw();
    int ret = readline(&vstr, p);
    mp_hal_stdio_mode_orig();
    if (ret != 0) {
        vstr_clear(&vstr);
        if (ret == CHAR_CTRL_D) {
            // EOF
            printf("\n");
            return NULL;
        } else {
            printf("\n");
            char *line = malloc(1);
            line[0] = '\0';
            return line;
        }
    }
    vstr_null_terminated_str(&vstr);
    char *line = malloc(vstr.len + 1);
    memcpy(line, vstr.buf, vstr.len + 1);
    vstr_clear(&vstr);
#elif MICROPY_USE_READLINE == 2
    // GNU readline
    char *line = readline(p);
    if (line) {
        add_history(line);
    }
#else
    // simple read string
    static char buf[256];
    fputs(p, stdout);
    char *s = fgets(buf, sizeof(buf), stdin);
    if (!s) {
        return NULL;
    }
    int l = strlen(buf);
    if (buf[l - 1] == '\n') {
        buf[l - 1] = 0;
    } else {
        l++;
    }
    char *line = malloc(l);
    memcpy(line, buf, l);
#endif
    return line;
}

void prompt_read_history(void) {
#if MICROPY_USE_READLINE_HISTORY
    #if MICROPY_USE_READLINE == 1
    readline_init0(); // will clear history pointers
    char *home = getenv("HOME");
    if (home != NULL) {
        vstr_t vstr;
        vstr_init(&vstr, 50);
        vstr_printf(&vstr, "%s/.micropython.history", home);
        int fd = open(vstr_null_terminated_str(&vstr), O_RDONLY);
        if (fd != -1) {
            vstr_reset(&vstr);
            for (;;) {
                char c;
                int sz = read(fd, &c, 1);
                if (sz < 0) {
                    break;
                }
                if (sz == 0 || c == '\n') {
                    readline_push_history(vstr_null_terminated_str(&vstr));
                    if (sz == 0) {
                        break;
                    }
                    vstr_reset(&vstr);
                } else {
                    vstr_add_byte(&vstr, c);
                }
            }
            close(fd);
        }
        vstr_clear(&vstr);
    }
    #elif MICROPY_USE_READLINE == 2
    read_history(tilde_expand("~/.micropython.history"));
    #endif
#endif
}

void prompt_write_history(void) {
#if MICROPY_USE_READLINE_HISTORY
    #if MICROPY_USE_READLINE == 1
    char *home = getenv("HOME");
    if (home != NULL) {
        vstr_t vstr;
        vstr_init(&vstr, 50);
        vstr_printf(&vstr, "%s/.micropython.history", home);
        int fd = open(vstr_null_terminated_str(&vstr), O_CREAT | O_TRUNC | O_WRONLY, 0644);
        if (fd != -1) {
            for (int i = MP_ARRAY_SIZE(MP_STATE_PORT(readline_hist)) - 1; i >= 0; i--) {
                const char *line = MP_STATE_PORT(readline_hist)[i];
                if (line != NULL) {
                    int res;
                    res = write(fd, line, strlen(line));
                    res = write(fd, "\n", 1);
                    (void)res;
                }
            }
            close(fd);
        }
    }
    #elif MICROPY_USE_READLINE == 2
    write_history(tilde_expand("~/.micropython.history"));
    #endif
#endif
}

STATIC mp_obj_t mp_builtin_input(mp_uint_t n_args, const mp_obj_t *args) {
    if (n_args == 1) {
        mp_obj_print(args[0], PRINT_STR);
    }

    char *line = prompt("");
    if (line == NULL) {
        nlr_raise(mp_obj_new_exception(&mp_type_EOFError));
    }
    mp_obj_t o = mp_obj_new_str(line, strlen(line), false);
    free(line);
    return o;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_builtin_input_obj, 0, 1, mp_builtin_input);
