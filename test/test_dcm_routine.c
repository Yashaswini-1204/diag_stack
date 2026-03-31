#include "unity.h"
#include "dcm_routine_table.h"
#include "dcm_did_table.h"
#include "dem_types.h"
#include <stddef.h>

void setUp(void)   {}
void tearDown(void) {}

void test_Routine_Find_SelfTest(void)
{ TEST_ASSERT_NOT_NULL(DCM_Routine_Find(0x0200U)); }

void test_Routine_Find_Unknown(void)
{ TEST_ASSERT_NULL(DCM_Routine_Find(0xBEEFU)); }

void test_Routine_CheckAccess_SelfTest_Extended_OK(void)
{
    const DCM_RoutineEntry_t *e = DCM_Routine_Find(0x0200U);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL(E_OK,
        DCM_Routine_CheckAccess(e, DCM_SESSION_EXTENDED, DCM_SEC_NONE));
}

void test_Routine_CheckAccess_SelfTest_Default_Denied(void)
{
    const DCM_RoutineEntry_t *e = DCM_Routine_Find(0x0200U);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL(E_NOT_OK,
        DCM_Routine_CheckAccess(e, DCM_SESSION_DEFAULT, DCM_SEC_NONE));
}

void test_Routine_CheckAccess_EraseMemory_NeedsSecurity(void)
{
    const DCM_RoutineEntry_t *e = DCM_Routine_Find(0xFF01U);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL(E_NOT_OK,
        DCM_Routine_CheckAccess(e, DCM_SESSION_PROGRAMMING, DCM_SEC_NONE));
}

void test_Routine_CheckAccess_EraseMemory_WithSecurity_OK(void)
{
    const DCM_RoutineEntry_t *e = DCM_Routine_Find(0xFF01U);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL(E_OK,
        DCM_Routine_CheckAccess(e, DCM_SESSION_PROGRAMMING, DCM_SEC_LEVEL1));
}

void test_Routine_Execute_SelfTest_Start(void)
{
    uint8_t  outBuf[8]; uint16_t outLen = 0U;
    DCM_RoutineEntry_t *e = DCM_Routine_Find(0x0200U);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL(E_OK,
        DCM_Routine_Execute(e, DCM_ROUTINE_START, NULL, 0U, outBuf, &outLen));
    TEST_ASSERT_EQUAL_UINT16(1U, outLen);
    TEST_ASSERT_EQUAL_HEX8(0x01U, outBuf[0U]);
}

void test_Routine_Execute_SelfTest_Result(void)
{
    uint8_t  outBuf[8]; uint16_t outLen = 0U;
    DCM_RoutineEntry_t *e = DCM_Routine_Find(0x0200U);
    TEST_ASSERT_NOT_NULL(e);
    (void)DCM_Routine_Execute(e, DCM_ROUTINE_START, NULL, 0U, outBuf, &outLen);
    outLen = 0U;
    TEST_ASSERT_EQUAL(E_OK,
        DCM_Routine_Execute(e, DCM_ROUTINE_RESULT, NULL, 0U, outBuf, &outLen));
    TEST_ASSERT_EQUAL_UINT16(1U, outLen);
    TEST_ASSERT_EQUAL_HEX8((uint8_t)DCM_ROUTINE_COMPLETED, outBuf[0U]);
}

void test_Routine_Execute_SelfTest_Stop(void)
{
    uint8_t  outBuf[8]; uint16_t outLen = 0U;
    DCM_RoutineEntry_t *e = DCM_Routine_Find(0x0200U);
    TEST_ASSERT_NOT_NULL(e);
    (void)DCM_Routine_Execute(e, DCM_ROUTINE_START,  NULL, 0U, outBuf, &outLen);
    outLen = 0U;
    (void)DCM_Routine_Execute(e, DCM_ROUTINE_STOP,   NULL, 0U, outBuf, &outLen);
    outLen = 0U;
    TEST_ASSERT_EQUAL(E_OK,
        DCM_Routine_Execute(e, DCM_ROUTINE_RESULT, NULL, 0U, outBuf, &outLen));
    TEST_ASSERT_EQUAL_UINT16(1U, outLen);
    TEST_ASSERT_EQUAL_HEX8((uint8_t)DCM_ROUTINE_IDLE, outBuf[0U]);
}

void test_Routine_Execute_UnknownControlType(void)
{
    uint8_t  outBuf[8]; uint16_t outLen = 0U;
    DCM_RoutineEntry_t *e = DCM_Routine_Find(0x0200U);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL(E_NOT_OK,
        DCM_Routine_Execute(e, 0xFFU, NULL, 0U, outBuf, &outLen));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_Routine_Find_SelfTest);
    RUN_TEST(test_Routine_Find_Unknown);
    RUN_TEST(test_Routine_CheckAccess_SelfTest_Extended_OK);
    RUN_TEST(test_Routine_CheckAccess_SelfTest_Default_Denied);
    RUN_TEST(test_Routine_CheckAccess_EraseMemory_NeedsSecurity);
    RUN_TEST(test_Routine_CheckAccess_EraseMemory_WithSecurity_OK);
    RUN_TEST(test_Routine_Execute_SelfTest_Start);
    RUN_TEST(test_Routine_Execute_SelfTest_Result);
    RUN_TEST(test_Routine_Execute_SelfTest_Stop);
    RUN_TEST(test_Routine_Execute_UnknownControlType);
    return UNITY_END();
}
