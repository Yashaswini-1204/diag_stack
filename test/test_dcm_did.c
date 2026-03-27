#include "unity.h"
#include "dcm_did_table.h"
#include "dem_types.h"
#include <stddef.h>

void setUp(void)   {}
void tearDown(void) {}

void test_Did_TableNotEmpty(void)
{
    TEST_ASSERT_GREATER_THAN_UINT16(0U, DCM_Did_GetCount());
}

void test_Did_Find_VIN(void)
{
    TEST_ASSERT_NOT_NULL(DCM_Did_Find(0xF190U));
}

void test_Did_Find_Unknown(void)
{
    TEST_ASSERT_NULL(DCM_Did_Find(0x9999U));
}

void test_Did_Read_VIN(void)
{
    uint8_t               buf[32];
    uint16_t              len   = 0U;
    const DCM_DidEntry_t *entry = DCM_Did_Find(0xF190U);
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_NOT_NULL(entry->readFn);
    TEST_ASSERT_EQUAL(E_OK, entry->readFn(buf, &len));
    TEST_ASSERT_EQUAL_UINT16(17U, len);
    TEST_ASSERT_EQUAL_HEX8((uint8_t)'T', buf[0U]);
}

void test_Did_Read_SoftwareVersion(void)
{
    uint8_t               buf[8];
    uint16_t              len   = 0U;
    const DCM_DidEntry_t *entry = DCM_Did_Find(0xF188U);
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_NOT_NULL(entry->readFn);
    TEST_ASSERT_EQUAL(E_OK, entry->readFn(buf, &len));
    TEST_ASSERT_EQUAL_UINT16(4U, len);
    TEST_ASSERT_EQUAL_HEX8(0x01U, buf[0U]);
}

void test_Did_Read_WheelSpeed(void)
{
    uint8_t               buf[8];
    uint16_t              len   = 0U;
    const DCM_DidEntry_t *entry = DCM_Did_Find(0x0100U);
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_NOT_NULL(entry->readFn);
    TEST_ASSERT_EQUAL(E_OK, entry->readFn(buf, &len));
    TEST_ASSERT_EQUAL_UINT16(4U, len);
}

void test_Did_Write_WheelSpeed_DefaultSession_Denied(void)
{
    const DCM_DidEntry_t *entry = DCM_Did_Find(0x0100U);
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL(E_NOT_OK,
        DCM_Did_CheckAccess(entry, DCM_SESSION_DEFAULT,
                            DCM_SEC_NONE, 1U));
}

void test_Did_Write_WheelSpeed_ExtendedSession_NoSecurity_Denied(void)
{
    const DCM_DidEntry_t *entry = DCM_Did_Find(0x0100U);
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL(E_NOT_OK,
        DCM_Did_CheckAccess(entry, DCM_SESSION_EXTENDED,
                            DCM_SEC_NONE, 1U));
}

void test_Did_Write_WheelSpeed_ExtendedSession_WithSecurity_OK(void)
{
    const DCM_DidEntry_t *entry = DCM_Did_Find(0x0100U);
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL(E_OK,
        DCM_Did_CheckAccess(entry, DCM_SESSION_EXTENDED,
                            DCM_SEC_LEVEL1, 1U));
}

void test_Did_Read_AuditLog_DefaultSession_Denied(void)
{
    const DCM_DidEntry_t *entry = DCM_Did_Find(0xF1A0U);
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL(E_NOT_OK,
        DCM_Did_CheckAccess(entry, DCM_SESSION_DEFAULT,
                            DCM_SEC_NONE, 0U));
}

void test_Did_Read_AuditLog_ExtendedWithSecurity_OK(void)
{
    const DCM_DidEntry_t *entry = DCM_Did_Find(0xF1A0U);
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL(E_OK,
        DCM_Did_CheckAccess(entry, DCM_SESSION_EXTENDED,
                            DCM_SEC_LEVEL1, 0U));
}

void test_Did_Write_BrakePressure_NotWritable(void)
{
    const DCM_DidEntry_t *entry = DCM_Did_Find(0x0101U);
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_NULL(entry->writeFn);
    TEST_ASSERT_EQUAL(E_NOT_OK,
        DCM_Did_CheckAccess(entry, DCM_SESSION_ALL,
                            DCM_SEC_NONE, 1U));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_Did_TableNotEmpty);
    RUN_TEST(test_Did_Find_VIN);
    RUN_TEST(test_Did_Find_Unknown);
    RUN_TEST(test_Did_Read_VIN);
    RUN_TEST(test_Did_Read_SoftwareVersion);
    RUN_TEST(test_Did_Read_WheelSpeed);
    RUN_TEST(test_Did_Write_WheelSpeed_DefaultSession_Denied);
    RUN_TEST(test_Did_Write_WheelSpeed_ExtendedSession_NoSecurity_Denied);
    RUN_TEST(test_Did_Write_WheelSpeed_ExtendedSession_WithSecurity_OK);
    RUN_TEST(test_Did_Read_AuditLog_DefaultSession_Denied);
    RUN_TEST(test_Did_Read_AuditLog_ExtendedWithSecurity_OK);
    RUN_TEST(test_Did_Write_BrakePressure_NotWritable);
    return UNITY_END();
}
