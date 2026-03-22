#include "unity.h"
#include "dem_core.h"
#include "dem_debounce.h"
#include "dem_types.h"
#include "dem_cfg.h"

void setUp(void)
{
    Dem_Init();
    Dem_Debounce_Init();
}

void tearDown(void) {}

static void confirmFault(Dem_EventIdType eventId)
{
    uint8_t i;
    for (i = 0U; i < (uint8_t)DEM_DEBOUNCE_FAIL_THRESHOLD; i++)
    {
        (void)Dem_ReportErrorStatus(eventId, DEM_EVENT_STATUS_FAILED);
    }
}

void test_Init_DefaultStatusByte(void)
{
    uint8_t status = 0x00U;
    TEST_ASSERT_EQUAL(E_OK, Dem_GetEventUdsStatus(1U, &status));
    TEST_ASSERT_EQUAL_HEX8((uint8_t)DEM_UDS_STATUS_DEFAULT, status);
}

void test_ReportFailed_BelowThreshold_NoCDTC(void)
{
    uint8_t status = 0x00U;
    uint8_t i;
    for (i = 0U; i < (uint8_t)(DEM_DEBOUNCE_FAIL_THRESHOLD - 1U); i++)
    {
        (void)Dem_ReportErrorStatus(1U, DEM_EVENT_STATUS_FAILED);
    }
    TEST_ASSERT_EQUAL(E_OK, Dem_GetEventUdsStatus(1U, &status));
    TEST_ASSERT_BITS_LOW((uint8_t)DEM_UDS_STATUS_CDTC, status);
}

void test_ReportFailed_AtThreshold_SetsCDTC(void)
{
    uint8_t status = 0x00U;
    confirmFault(1U);
    TEST_ASSERT_EQUAL(E_OK, Dem_GetEventUdsStatus(1U, &status));
    TEST_ASSERT_BITS_HIGH((uint8_t)DEM_UDS_STATUS_CDTC, status);
}

void test_ReportFailed_AtThreshold_SetsTF(void)
{
    uint8_t status = 0x00U;
    confirmFault(1U);
    TEST_ASSERT_EQUAL(E_OK, Dem_GetEventUdsStatus(1U, &status));
    TEST_ASSERT_BITS_HIGH((uint8_t)DEM_UDS_STATUS_TF, status);
}

void test_ReportFailed_AtThreshold_SetsPDTC(void)
{
    uint8_t status = 0x00U;
    confirmFault(1U);
    TEST_ASSERT_EQUAL(E_OK, Dem_GetEventUdsStatus(1U, &status));
    TEST_ASSERT_BITS_HIGH((uint8_t)DEM_UDS_STATUS_PDTC, status);
}

void test_ReportPassed_ClearsTF(void)
{
    uint8_t status = 0x00U;
    uint8_t i;
    confirmFault(1U);
    for (i = 0U; i < (uint8_t)DEM_DEBOUNCE_FAIL_THRESHOLD; i++)
    {
        (void)Dem_ReportErrorStatus(1U, DEM_EVENT_STATUS_PASSED);
    }
    TEST_ASSERT_EQUAL(E_OK, Dem_GetEventUdsStatus(1U, &status));
    TEST_ASSERT_BITS_LOW((uint8_t)DEM_UDS_STATUS_TF, status);
}

void test_PreFailed_100Times_NoCDTC(void)
{
    uint8_t  status = 0x00U;
    uint16_t i;
    for (i = 0U; i < 100U; i++)
    {
        (void)Dem_ReportErrorStatus(1U, DEM_EVENT_STATUS_PREFAILED);
    }
    TEST_ASSERT_EQUAL(E_OK, Dem_GetEventUdsStatus(1U, &status));
    TEST_ASSERT_BITS_LOW((uint8_t)DEM_UDS_STATUS_CDTC, status);
}

void test_InvalidEventId_ReturnsNotOk(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK,
        Dem_ReportErrorStatus(DEM_EVENT_ID_INVALID,
                              DEM_EVENT_STATUS_FAILED));
}

void test_OutOfRangeEventId_ReturnsNotOk(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK,
        Dem_ReportErrorStatus((Dem_EventIdType)200U,
                              DEM_EVENT_STATUS_FAILED));
}

void test_OperationCycleStart_SetsTNCTOC(void)
{
    uint8_t status = 0x00U;
    (void)Dem_ReportErrorStatus(1U, DEM_EVENT_STATUS_FAILED);
    Dem_OperationCycleStart();
    TEST_ASSERT_EQUAL(E_OK, Dem_GetEventUdsStatus(1U, &status));
    TEST_ASSERT_BITS_HIGH((uint8_t)DEM_UDS_STATUS_TNCTOC, status);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_Init_DefaultStatusByte);
    RUN_TEST(test_ReportFailed_BelowThreshold_NoCDTC);
    RUN_TEST(test_ReportFailed_AtThreshold_SetsCDTC);
    RUN_TEST(test_ReportFailed_AtThreshold_SetsTF);
    RUN_TEST(test_ReportFailed_AtThreshold_SetsPDTC);
    RUN_TEST(test_ReportPassed_ClearsTF);
    RUN_TEST(test_PreFailed_100Times_NoCDTC);
    RUN_TEST(test_InvalidEventId_ReturnsNotOk);
    RUN_TEST(test_OutOfRangeEventId_ReturnsNotOk);
    RUN_TEST(test_OperationCycleStart_SetsTNCTOC);
    return UNITY_END();
}
