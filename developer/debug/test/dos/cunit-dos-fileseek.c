/*
    Copyright (C) 2021, The AROS Development Team. All rights reserved.
*/

#include <stdio.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <stdlib.h>
#include <assert.h>

#include <CUnit/Basic.h>

/* handles for the respective tests */
static BPTR file = BNULL;

/* storage used during testing */
static char buffer[32];
static int i;

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
    if (file)
        if (0 ==Close (file))
            return -1;
    return 0;
}

/* Simple test of Open().
 */
void testOPENW(void)
{
    CU_ASSERT(NULL != (file = Open( "T:cunit-dos-fileseek.txt", MODE_NEWFILE )));
}

/* Simple test of Write().
 */
void testWRITE(void)
{
    if (file)
    {
        CU_ASSERT(0 != Write(file,"() does not work!\n",18));
    }
}

/* Simple test of Close().
 */
void testCLOSE(void)
{
    if (file)
    {
        CU_ASSERT(0 != Close(file));
        file = BNULL;
    }
}

/* Simple test of Open().
 */
void testOPENR(void)
{
    CU_ASSERT(NULL != (file = Open( "T:cunit-dos-fileseek.txt", MODE_OLDFILE)));
}

/* Simple test of Read().
 */
void testREAD(void)
{
    if (file)
    {
        CU_ASSERT(7 == (i = Read( file, buffer, 7 )));
    }
}

/* Simple test of Seek().
 */
void testSEEK(void)
{
    if (file)
    {
        /* Seek() */
        CU_ASSERT(-1 == Seek( file, 4, OFFSET_CURRENT ));
        CU_ASSERT(18 == (i += Read( file, &buffer[7], 11 )));
    }
}

int main(void)
{
    CU_pSuite pSuite = NULL;

    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

   /* add a suite to the registry */
    pSuite = CU_add_suite("DOSFileSeek_Suite", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

   /* add the tests to the suite */
    if ((NULL == CU_add_test(pSuite, "test of Open(\"T:cunit-dos-fileseek.txt\",MODE_NEWFILE)", testOPENW)) ||
        (NULL == CU_add_test(pSuite, "test of Write()", testWRITE)) ||
        (NULL == CU_add_test(pSuite, "test of Close()", testCLOSE)) ||
        (NULL == CU_add_test(pSuite, "test of Open(\"T:cunit-dos-fileseek.txt\",MODE_OLDFILE)", testOPENR)) ||
        (NULL == CU_add_test(pSuite, "test of Read()", testREAD)) ||
        (NULL == CU_add_test(pSuite, "test of Seek()", testSEEK)))
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