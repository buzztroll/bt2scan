#include <CUnit/CUnit.h>
#include <stdio.h>
#include <stdlib.h>
#include "buzz_opts.h"

int main(int argc, char ** argv) {

    buzz_opts_handle_t * buzz_opts;
    char *args[] = {"arg1", "next", "three", NULL};
    static struct option * cli_options;

    buzz_opts_init(&buzz_opts, "test", "testing this program", args);
    buzz_opts_add_option(buzz_opts, "longname", 'l', 1, "this has an argument");
    buzz_opts_add_option(buzz_opts, "quiet", 'q', 0, "this has NO argument");

    buzz_opts_print_usage(buzz_opts, stdout);

    char * shortopts = buzz_opts_create_short_opts(buzz_opts);
    printf("%s\n", shortopts);
    cli_options = buzz_opts_create_long_opts(buzz_opts);

    int option_index = 0;
    int opt;

   while ((opt = getopt_long(argc, argv, shortopts, cli_options, &option_index)) != -1) {
        switch (opt) {
        case 'q':
            printf("be quiet!\n");
            break;

        case 'l':
            printf("logname %s\n", optarg);
            break;

        case 'h':
        default:
            printf("hit a random case!");
            return 0;
        }
    }


    buzz_opts_destroy(buzz_opts);

    return 0;
}
