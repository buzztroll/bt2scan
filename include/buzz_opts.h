#ifndef BT2_OPTS_H
#define BT2_OPTS_H 1

#include <getopt.h>


typedef struct buzz_opts_entry_s {
    char * longname;
    char shortname;
    int has_arg;
    char * description;
} buzz_opts_entry_t;


typedef struct buzz_opts_handle_s {
    char * program_name;
    char * description;
    int options_count;
    int options_capacity;
    char ** args;
    int arg_count;
    buzz_opts_entry_t * opt_entry;

    char * short_opts;
    struct option* long_opts;
} buzz_opts_handle_t;

int buzz_opts_init(buzz_opts_handle_t ** buzz_opts, const char * program_name, const char * description, char ** args);

int buzz_opts_destroy(buzz_opts_handle_t * buzz_opts);

struct option* buzz_opts_create_long_opts(buzz_opts_handle_t * buzz_opts_handle);

char * buzz_opts_create_short_opts(buzz_opts_handle_t * buzz_opts_handle);

int buzz_opts_print_usage(buzz_opts_handle_t * buzz_opts_handle, FILE * output);

int buzz_opts_add_option(buzz_opts_handle_t * buzz_opts_handle, const char * long_name, char short_name, int has_arg, const char * description);

#define BUZZ_OPTS_SUCCESS 0
#define BUZZ_OPTS_ERROR -1

#endif
