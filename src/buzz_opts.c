#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "buzz_opts.h"


int buzz_opts_init(buzz_opts_handle_t ** buzz_opts, const char * program_name, const char * description, char ** args) {
    buzz_opts_handle_t * new_handle;

    new_handle = (buzz_opts_handle_t *) calloc(1, sizeof(buzz_opts_handle_t));
    new_handle->program_name = strdup(program_name);
    new_handle->description = strdup(description);
    new_handle->options_capacity = 16;
    new_handle->args = args;
    new_handle->opt_entry = (buzz_opts_entry_t *) calloc(new_handle->options_capacity, sizeof(buzz_opts_entry_t));

    if (args != NULL) {
        int i;
        for (i = 0; args[i] != NULL; i++) {
            
        }
        new_handle->arg_count = i;
    }

    *buzz_opts = new_handle;

    return BUZZ_OPTS_SUCCESS;
}

int buzz_opts_destroy(buzz_opts_handle_t * buzz_opts) {

    if (buzz_opts->short_opts != NULL) {
        free(buzz_opts->short_opts);
    }
    if (buzz_opts->long_opts != NULL) {
        free(buzz_opts->long_opts);
    }

    free(buzz_opts->program_name);
    free(buzz_opts->description);
    for (int i = 0; i < buzz_opts->options_count; i++) {
        free(buzz_opts->opt_entry[i].longname);
        free(buzz_opts->opt_entry[i].description);
    }
    free(buzz_opts->opt_entry);
    free(buzz_opts);

    return BUZZ_OPTS_SUCCESS;
}


/* the caller needs to free the memory */
struct option* buzz_opts_create_long_opts(buzz_opts_handle_t * buzz_opts_handle) {
    struct option* long_opts;

    long_opts = (struct option *) calloc(buzz_opts_handle->options_count, sizeof(struct option));

    for(int i = 0; i < buzz_opts_handle->options_count; i++) {
        long_opts[i].name = buzz_opts_handle->opt_entry[i].longname;
        long_opts[i].has_arg = buzz_opts_handle->opt_entry[i].has_arg;
        long_opts[i].val = buzz_opts_handle->opt_entry[i].shortname;
    }
    buzz_opts_handle->long_opts = long_opts;

    return long_opts;
}

/* caller must free */
char * buzz_opts_create_short_opts(buzz_opts_handle_t * buzz_opts_handle) {

    /* create enough room for all options to have an argument plus '\0' */
    char * short_opts = calloc(buzz_opts_handle->options_count * 2 + 1, sizeof(char));
    int char_ndx = 0;

    for(int i = 0; i < buzz_opts_handle->options_count; i++) {
        short_opts[char_ndx] = buzz_opts_handle->opt_entry[i].shortname;
        char_ndx++;
        if (buzz_opts_handle->opt_entry[i].has_arg) {
            short_opts[char_ndx] = ':';
            char_ndx++;
        }
    }
    buzz_opts_handle->short_opts = short_opts;

    return short_opts;
}

int buzz_opts_print_usage(buzz_opts_handle_t * buzz_opts_handle, FILE * output) {
    int line_len;

    line_len = fprintf(output, "%s", buzz_opts_handle->program_name);
    if (buzz_opts_handle->options_count > 0) {
        line_len += fprintf(output, " [options]");
    }
    for (int i = 0; i < buzz_opts_handle->arg_count; i++) {
        line_len += fprintf(output, " <%s>", buzz_opts_handle->args[i]);
    }
    fprintf(output, "\n");
    for (int i = 0; i < line_len; i++) {
        fprintf(output, "=");
    }
    fprintf(output, "\n");
    fprintf(output, "%s\n", buzz_opts_handle->description);
    
    for(int i = 0; i < buzz_opts_handle->options_count; i++) {
        buzz_opts_entry_t * ent;
        ent = &buzz_opts_handle->opt_entry[i];
        fprintf(output, "    -%c|--%s : %s\n", ent->shortname, ent->longname, ent->description);
    }

    return BUZZ_OPTS_SUCCESS;
}

int buzz_opts_add_option(buzz_opts_handle_t * buzz_opts_handle, const char * long_name, char short_name, int has_arg, const char * description) {
    int option_count = buzz_opts_handle->options_count;
    if (option_count + 1 > buzz_opts_handle->options_capacity) {
        buzz_opts_handle->options_capacity = option_count * 2;
        buzz_opts_handle->opt_entry = (buzz_opts_entry_t *) realloc(buzz_opts_handle->opt_entry, buzz_opts_handle->options_capacity);
    }
    buzz_opts_handle->opt_entry[option_count].description = strdup(description);
    buzz_opts_handle->opt_entry[option_count].longname = strdup(long_name);
    buzz_opts_handle->opt_entry[option_count].has_arg = has_arg;
    buzz_opts_handle->opt_entry[option_count].shortname = short_name;

    buzz_opts_handle->options_count++;

    return BUZZ_OPTS_SUCCESS;
}
