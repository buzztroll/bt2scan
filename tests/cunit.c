#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include "CUnit/Basic.h"

int setup(void)
{
    printf("enter setup\n");
    return 0;
}

int teardown(void)
{
    printf("enter teardown\n");
    return 0;
}


void test_one(void)
{
    printf("enter test_one\n");
    CU_ASSERT(1 == 1);
}

void test_fail(void)
{
    CU_ASSERT(1);
}

int main()
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

   CU_add_test(pSuite, "Good test", test_one);
   CU_add_test(pSuite, "Bad test", test_one);

   CU_basic_set_mode(CU_BRM_VERBOSE);
   CU_basic_run_tests();
   CU_cleanup_registry();
   return CU_get_error();
}
