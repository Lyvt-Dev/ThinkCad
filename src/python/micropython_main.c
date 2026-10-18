// Native, separate-process MicroPython host. User scripts execute only on Run.
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include "py/compile.h"
#include "py/builtin.h"
#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/stackctrl.h"
#include "port/micropython_embed.h"

void thinkcad_sleep_us(uint64_t us);
static unsigned char heap[4 * 1024 * 1024];

static char *read_source(const char *path, size_t *length) {
    FILE *file = fopen(path, "rb");
    if (!file) mp_raise_OSError(MP_ENOENT);
    if (fseek(file, 0, SEEK_END) != 0) { fclose(file); mp_raise_OSError(MP_EIO); }
    long size = ftell(file);
    if (size < 0 || size > 2 * 1024 * 1024) { fclose(file); mp_raise_ValueError(MP_ERROR_TEXT("Python file exceeds 2 MiB")); }
    rewind(file);
    char *text = malloc((size_t)size + 1);
    if (!text) { fclose(file); mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("Source allocation failed")); }
    *length = fread(text, 1, (size_t)size, file);
    int failed = ferror(file); fclose(file);
    if (failed || *length != (size_t)size) { free(text); mp_raise_OSError(MP_EIO); }
    text[*length] = 0;
    return text;
}

mp_import_stat_t mp_import_stat(const char *path) {
    struct stat info;
    if (stat(path, &info) != 0) return MP_IMPORT_STAT_NO_EXIST;
    return (info.st_mode & S_IFDIR) ? MP_IMPORT_STAT_DIR : MP_IMPORT_STAT_FILE;
}
mp_lexer_t *mp_lexer_new_from_file(qstr filename) {
    size_t length; char *source = read_source(qstr_str(filename), &length);
    // The lexer owns a GC allocation; the temporary libc buffer can be released.
    char *copy = m_new(char, length + 1); memcpy(copy, source, length + 1); free(source);
    return mp_lexer_new_from_str_len(filename, copy, length, length + 1);
}

// User print() is encoded as serial text, never interpreted as host commands.
void mp_hal_stdout_tx_strn_cooked(const char *text, size_t length) {
    static const char hex[] = "0123456789abcdef";
    while (length) {
        size_t count = length > 512 ? 512 : length;
        if (length > 512) while (count && (((unsigned char)text[count] & 0xc0) == 0x80)) --count;
        fputs("SERIAL ", stdout);
        for (size_t i = 0; i < count; ++i) { unsigned char byte = (unsigned char)text[i]; fputc(hex[byte >> 4], stdout); fputc(hex[byte & 15], stdout); }
        fputc('\n', stdout); text += count; length -= count;
    }
    fflush(stdout);
}
static void protocol_line(mp_obj_t command) {
    const char *line = mp_obj_str_get_str(command);
    if (strlen(line) > 8192 || strchr(line, '\n') || strchr(line, '\r')) mp_raise_ValueError(MP_ERROR_TEXT("Invalid host command"));
    puts(line); fflush(stdout);
}
static mp_obj_t host_emit(mp_obj_t command) { protocol_line(command); return mp_const_none; }
static MP_DEFINE_CONST_FUN_OBJ_1(host_emit_obj, host_emit);
static mp_obj_t host_read(void) {
    char line[64]; char *end;
    if (!fgets(line, sizeof(line), stdin)) mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Simulator disconnected"));
    long value = strtol(line, &end, 10);
    if (end == line || (*end != '\n' && *end != '\r' && *end != 0)) mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Invalid simulator response"));
    return mp_obj_new_int(value);
}
static MP_DEFINE_CONST_FUN_OBJ_0(host_read_obj, host_read);
static mp_obj_t host_request(mp_obj_t command) { protocol_line(command); return host_read(); }
static MP_DEFINE_CONST_FUN_OBJ_1(host_request_obj, host_request);
static mp_obj_t host_sleep(mp_obj_t value) {
    mp_int_t us = mp_obj_get_int(value);
    if (us < 0 || us > 1000000) mp_raise_ValueError(MP_ERROR_TEXT("Invalid host sleep"));
    thinkcad_sleep_us((uint64_t)us); return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(host_sleep_obj, host_sleep);
static void stderr_print(void *unused, const char *text, size_t length) { (void)unused; fwrite(text, 1, length, stderr); }
static int execute(const char *path) {
    nlr_buf_t exception;
    if (nlr_push(&exception) == 0) {
        mp_lexer_t *lexer = mp_lexer_new_from_file(qstr_from_str(path));
        qstr filename = lexer->source_name;
        mp_parse_tree_t tree = mp_parse(lexer, MP_PARSE_FILE_INPUT);
        mp_obj_t function = mp_compile(&tree, filename, true);
        mp_call_function_0(function); nlr_pop(); return 0;
    }
    const mp_print_t printer = {NULL, stderr_print};
    mp_obj_print_exception(&printer, (mp_obj_t)exception.ret_val); fflush(stderr); return 1;
}
int main(int argc, char **argv) {
    if (argc != 2) { fputs("Usage: thinkcad-micropython MAIN.py\n", stderr); return 2; }
    int stack_top; mp_embed_init(heap, sizeof(heap), &stack_top);mp_stack_set_limit(256 * 1024);
    mp_obj_t module = mp_obj_new_module(qstr_from_str("_thinkcad"));
    mp_obj_dict_t *globals = mp_obj_module_get_globals(module);
    mp_obj_dict_store(MP_OBJ_FROM_PTR(globals), MP_OBJ_NEW_QSTR(qstr_from_str("emit")), MP_OBJ_FROM_PTR(&host_emit_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(globals), MP_OBJ_NEW_QSTR(qstr_from_str("read")), MP_OBJ_FROM_PTR(&host_read_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(globals), MP_OBJ_NEW_QSTR(qstr_from_str("request")), MP_OBJ_FROM_PTR(&host_request_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(globals), MP_OBJ_NEW_QSTR(qstr_from_str("sleep_us")), MP_OBJ_FROM_PTR(&host_sleep_obj));
    mp_obj_list_append(mp_sys_path, mp_obj_new_str(".", 1));
    mp_obj_list_append(mp_sys_path, mp_obj_new_str("lib", 3));
    mp_obj_list_append(mp_sys_path, mp_obj_new_str("src", 3));
    mp_store_global(qstr_from_str("__file__"), mp_obj_new_str(argv[1], strlen(argv[1])));
    puts("READY"); fflush(stdout);
    int result = execute(argv[1]);
    mp_embed_deinit(); return result;
}
