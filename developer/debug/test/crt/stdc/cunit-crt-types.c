/*
    Copyright � 2021, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <assert.h>

#include <CUnit/Basic.h>

/* The suite initialization function.
  * Returns zero on success, non-zero otherwise.
 */
int init_suite(void)
{
    return 0;
}

/* The suite cleanup function.
  * Returns zero on success, non-zero otherwise.
 */
int clean_suite(void)
{
    return 0;
}

void testCHAR(void)
{
    CU_ASSERT(8 == sizeof(char));
    CU_ASSERT(8 == sizeof(signed char));
    CU_ASSERT(8 == sizeof(unsigned char));
}

void testSHORT(void)
{
    CU_ASSERT(16 == sizeof(short));
    CU_ASSERT(16 == sizeof(short int));
    CU_ASSERT(16 == sizeof(signed short));
    CU_ASSERT(16 == sizeof(signed short int));
    CU_ASSERT(16 == sizeof(unsigned short));
    CU_ASSERT(16 == sizeof(unsigned short int));
}

void testINT(void)
{
    CU_ASSERT(16 == sizeof(int));
    CU_ASSERT(16 == sizeof(signed));
    CU_ASSERT(16 == sizeof(signed int));
    CU_ASSERT(16 == sizeof(unsigned));
    CU_ASSERT(16 == sizeof(unsigned int));
}

void testLONG(void)
{
    CU_ASSERT(32 == sizeof(long));
    CU_ASSERT(32 == sizeof(long int));
    CU_ASSERT(32 == sizeof(signed long));
    CU_ASSERT(32 == sizeof(signed long int));
    CU_ASSERT(32 == sizeof(unsigned long));
    CU_ASSERT(32 == sizeof(unsigned long int));
}

void testLLONG(void)
{
    CU_ASSERT(64 == sizeof(long long));
    CU_ASSERT(64 == sizeof(long long int));
    CU_ASSERT(64 == sizeof(signed long long));
    CU_ASSERT(64 == sizeof(signed long long int));
    CU_ASSERT(64 == sizeof(unsigned long long));
    CU_ASSERT(64 == sizeof(unsigned long long int));
}

void testFLOAT(void)
{
    CU_ASSERT(32 == sizeof(float));
}

void testDOUBLE(void)
{
    CU_ASSERT(64 == sizeof(double));
}

void testLDOUBLE(void)
{
    CU_ASSERT(64 < sizeof(long double));
}

int main(void)
{
    CU_pSuite pSuite = NULL;

    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

   /* add a suite to the registry */
    pSuite = CU_add_suite("CRStandardTypes_Suite", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

   /* add the tests to the suite */
    if ((NULL == CU_add_test(pSuite, "test of char type", testCHAR)) ||
        (NULL == CU_add_test(pSuite, "test of short type", testSHORT)) ||
        (NULL == CU_add_test(pSuite, "test of int type", testINT)) ||
        (NULL == CU_add_test(pSuite, "test of long type", testLONG)) ||
        (NULL == CU_add_test(pSuite, "test of long long type", testLLONG)) ||
        (NULL == CU_add_test(pSuite, "test of float type", testFLOAT)) ||
        (NULL == CU_add_test(pSuite, "test of double type", testDOUBLE)) ||
        (NULL == CU_add_test(pSuite, "test of long double type", testLDOUBLE)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Run all tests using the CUnit Basic interface */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}