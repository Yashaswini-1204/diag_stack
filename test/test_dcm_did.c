#include "unity.h"
#include "dcm_did_table.h"
#include "dem_types.h"
#include <stddef.h>

void setUp(void)   {}
void tearDown(void) {}

void test_Did_TableNotEmpty(void)
{ TEST_ASSERT_GREATER_THAN_UINT16(0U, DCM_Did_GetCount()); }

void test_Did_Find_StationID(void)
{ TEST_ASSERT_NOT_NULL(DCM_Did_Find(0xF1A0U)); }

void test_Did_Find_Unknown(void)
{ TEST_ASSERT_NULL(DCM_Did_Find(0x9999U)); }

void test_Did_Read_SWVersion(void)
{
    uint8_t buf[8]; uint16_t len = 0U;
    const DCM_DidEntry_t *e = DCM_Did_Find(0xF188U);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL(E_OK, e->readFn(buf, &len));
    TEST_ASSERT_EQUAL_UINT16(4U, len);
    TEST_ASSERT_EQUAL_HEX8(0x01U, buf[0]);
}

void test_Did_Read_SOSCounter(void)
{
    uint8_t buf[8]; uint16_t len = 0U;
    const DCM_DidEntry_t *e = DCM_Did_Find(0x0300U);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL(E_OK, e->readFn(buf, &len));
    TEST_ASSERT_EQUAL_UINT16(4U, len);
}

void test_Did_Read_HealthStatus(void)
{
    uint8_t buf[4]; uint16_t len = 0U;
    const DCM_DidEntry_t *e = DCM_Did_Find(0x0301U);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL(E_OK, e->readFn(buf, &len));
    TEST_ASSERT_EQUAL_UINT16(1U, len);
}

void test_Did_Write_SOSCounter_DefaultSession_Denied(void)
{
    const DCM_DidEntry_t *e = DCM_Did_Find(0x0300U);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL(E_NOT_OK,
        DCM_Did_CheckAccess(e, DCM_SESSION_DEFAULT, DCM_SEC_NONE, 1U));
}

void test_Did_Write_SOSCounter_ExtendedNoSecurity_Denied(void)
{
    const DCM_DidEntry_t *e = DCM_Did_Find(0x0300U);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL(E_NOT_OK,
        DCM_Did_CheckAccess(e, DCM_SESSION_EXTENDED, DCM_SEC_NONE, 1U));
}

void test_Did_Write_SOSCounter_ExtendedWithSecurity_OK(void)
{
    const DCM_DidEntry_t *e = DCM_Did_Find(0x0300U);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL(E_OK,
        DCM_Did_CheckAccess(e, DCM_SESSION_EXTENDED, DCM_SEC_LEVEL1, 1U));
}

void test_Did_Read_CommStatus_AllSessions_OK(void)
{
    const DCM_DidEntry_t *e = DCM_Did_Find(0x0302U);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL(E_OK,
        DCM_Did_CheckAccess(e, DCM_SESSION_DEFAULT, DCM_SEC_NONE, 0U));
}

void test_Did_Write_CommStatus_NotWritable(void)
{
    const DCM_DidEntry_t *e = DCM_Did_Find(0x0302U);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_NULL(e->writeFn);
    TEST_ASSERT_EQUAL(E_NOT_OK,
        DCM_Did_CheckAccess(e, DCM_SESSION_ALL, DCM_SEC_NONE, 1U));
}

void test_Did_Read_StationID(void)
{
    uint8_t buf[8]; uint16_t len = 0U;
    const DCM_DidEntry_t *e = DCM_Did_Find(0xF1A0U);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL(E_OK, e->readFn(buf, &len));
    TEST_ASSERT_EQUAL_UINT16(3U, len);
}

void test_Did_Read_SerialNumber(void)
{
    uint8_t buf[16]; uint16_t len = 0U;
    const DCM_DidEntry_t *e = DCM_Did_Find(0xF18CU);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL(E_OK, e->readFn(buf, &len));
    TEST_ASSERT_EQUAL_UINT16(8U, len);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_Did_TableNotEmpty);
    RUN_TEST(test_Did_Find_StationID);
    RUN_TEST(test_Did_Find_Unknown);
    RUN_TEST(test_Did_Read_SWVersion);
    RUN_TEST(test_Did_Read_SOSCounter);
    RUN_TEST(test_Did_Read_HealthStatus);
    RUN_TEST(test_Did_Write_SOSCounter_DefaultSession_Denied);
    RUN_TEST(test_Did_Write_SOSCounter_ExtendedNoSecurity_Denied);
    RUN_TEST(test_Did_Write_SOSCounter_ExtendedWithSecurity_OK);
    RUN_TEST(test_Did_Read_CommStatus_AllSessions_OK);
    RUN_TEST(test_Did_Write_CommStatus_NotWritable);
    RUN_TEST(test_Did_Read_StationID);
    RUN_TEST(test_Did_Read_SerialNumber);
    return UNITY_END();
}
