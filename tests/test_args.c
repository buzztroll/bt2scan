#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include "CUnit/Basic.h"

#include "buzz_opts.h"


int setup(void)
{
    return 0;
}

int teardown(void)
{
    return 0;
}


void test_short_opts(void)
{
    buzz_opts_handle_t * buzz_opts;
    buzz_opts_init(&buzz_opts, "test", "testing this program", NULL);
    buzz_opts_add_option(buzz_opts, "longname", 'l', 1, "this has an argument");
    buzz_opts_add_option(buzz_opts, "quiet", 'q', 0, "this has NO argument");

    char * shortopts = buzz_opts_create_short_opts(buzz_opts);
    
    CU_ASSERT(NULL != strstr(shortopts, "l:"));

    char * ptr = strstr(shortopts, "q");

    CU_ASSERT_NOT_EQUAL(NULL, ptr);
    CU_ASSERT_NOT_EQUAL(':', ptr[1]);

    buzz_opts_destroy(buzz_opts);
}

void test_short_opts_arg(void)
{
    buzz_opts_handle_t * buzz_opts;
    char * arg = "debug";
    char * argv[] = {"-l", arg, "--quiet", NULL}; 

    buzz_opts_init(&buzz_opts, "test", "testing this program", NULL);
    buzz_opts_add_option(buzz_opts, "longname", 'l', 1, "this has an argument");
    buzz_opts_add_option(buzz_opts, "quiet", 'q', 0, "this has NO argument");

    int opt;
    char * shortopts = buzz_opts_create_short_opts(buzz_opts);
    int found_short = 0;
    int found_long = 0;
    int found_extra = 0;
    struct option * cli_options = buzz_opts_create_long_opts(buzz_opts);

    while ((opt = getopt_long(3, argv, shortopts, cli_options, NULL)) != -1) {
        switch (opt) {
        case 'q':
            found_short = 1;
            break;

        case 'l':
            CU_ASSERT_EQUAL(optarg, arg);
            found_long = 1;
            break;

        default:
            found_extra = 1;
            break;
        }
    }

    CU_ASSERT_EQUAL(1, found_short);
    CU_ASSERT_EQUAL(1, found_long);
    CU_ASSERT_EQUAL(0, found_extra);

    buzz_opts_destroy(buzz_opts);
}


int main(int argc, char ** argv)
{
   CU_pSuite pSuite = NULL;
   if (CUE_SUCCESS != CU_initialize_registry()) {
      return CU_get_error();
   }
   pSuite = CU_add_suite("Suite1", setup, teardown);
   if (NULL == pSuite) {
      CU_cleanup_registry();
      return CU_get_error();
   }

   CU_add_test(pSuite, "Test short arg string", test_short_opts);

   CU_basic_set_mode(CU_BRM_VERBOSE);
   CU_basic_run_tests();
   CU_cleanup_registry();
   return CU_get_error();
}
