#include "unity.h"
#include "dem_core.h"
#include "dem_dtc.h"
#include "dem_debounce.h"
#include "dem_aging.h"
#include "dem_cfg.h"
#include "dem_types.h"

#define TEST_EVENT (1U)

void setUp(void)
{
    Dem_Init(); Dem_Debounce_Init();
    Dem_Dtc_Init(); Dem_Aging_Init();
}
void tearDown(void) {}

static void ConfirmDTC(Dem_EventIdType id)
{
    uint8_t i;
    for (i = 0U; i < (uint8_t)DEM_DEBOUNCE_FAIL_THRESHOLD; i++)
        Dem_ReportErrorStatus(id, DEM_EVENT_STATUS_FAILED);
}

void test_Aging_CounterIncrements(void)
{
    ConfirmDTC(TEST_EVENT);
    Dem_Aging_OperationCycleEnd();
    TEST_ASSERT_EQUAL_UINT8(1U, Dem_Aging_GetCounter(TEST_EVENT));
}

void test_Aging_FailureResetsCounter(void)
{
    ConfirmDTC(TEST_EVENT);
    Dem_Aging_OperationCycleEnd();
    Dem_Aging_ReportEvent(TEST_EVENT, DEM_EVENT_STATUS_FAILED);
    Dem_Aging_OperationCycleEnd();
    TEST_ASSERT_EQUAL_UINT8(0U, Dem_Aging_GetCounter(TEST_EVENT));
}

void test_Aging_AgeOutAfterThreshold(void)
{
    uint8_t statusByte = 0U; uint8_t i;
    ConfirmDTC(TEST_EVENT);
    for (i = 0U; i < (uint8_t)DEM_AGING_CYCLE_COUNT; i++)
        Dem_Aging_OperationCycleEnd();
    Dem_GetEventUdsStatus(TEST_EVENT, &statusByte);
    TEST_ASSERT_BITS(DEM_UDS_STATUS_CDTC, 0U, statusByte);
}

void test_Healing_CounterIncrements(void)
{
    ConfirmDTC(TEST_EVENT);
    Dem_Aging_ReportEvent(TEST_EVENT, DEM_EVENT_STATUS_PASSED);
    Dem_Aging_OperationCycleEnd();
    TEST_ASSERT_EQUAL_UINT8(1U, Dem_Aging_GetHealingCounter(TEST_EVENT));
}

void test_Healing_FailureResetsCounter(void)
{
    ConfirmDTC(TEST_EVENT);
    Dem_Aging_ReportEvent(TEST_EVENT, DEM_EVENT_STATUS_PASSED);
    Dem_Aging_OperationCycleEnd();
    Dem_Aging_ReportEvent(TEST_EVENT, DEM_EVENT_STATUS_FAILED);
    Dem_Aging_OperationCycleEnd();
    TEST_ASSERT_EQUAL_UINT8(0U, Dem_Aging_GetHealingCounter(TEST_EVENT));
}

void test_Healing_ClearsTNCSLC(void)
{
    uint8_t statusByte = 0U; uint8_t i;
    ConfirmDTC(TEST_EVENT);
    for (i = 0U; i < (uint8_t)DEM_HEALING_CYCLE_COUNT; i++)
    {
        Dem_Aging_ReportEvent(TEST_EVENT, DEM_EVENT_STATUS_PASSED);
        Dem_Aging_OperationCycleEnd();
    }
    Dem_GetEventUdsStatus(TEST_EVENT, &statusByte);
    TEST_ASSERT_BITS(DEM_UDS_STATUS_TNCSLC, 0U, statusByte);
    TEST_ASSERT_BITS(DEM_UDS_STATUS_CDTC, DEM_UDS_STATUS_CDTC, statusByte);
}

void test_Aging_NoAgeIfNotConfirmed(void)
{
    Dem_ReportErrorStatus(TEST_EVENT, DEM_EVENT_STATUS_FAILED);
    Dem_Aging_OperationCycleEnd();
    TEST_ASSERT_EQUAL_UINT8(0U, Dem_Aging_GetCounter(TEST_EVENT));
}

void test_Aging_InvalidId_Safe(void)
{
    Dem_Aging_ReportEvent(DEM_EVENT_ID_INVALID, DEM_EVENT_STATUS_FAILED);
    Dem_Aging_OperationCycleEnd();
    TEST_PASS();
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_Aging_CounterIncrements);
    RUN_TEST(test_Aging_FailureResetsCounter);
    RUN_TEST(test_Aging_AgeOutAfterThreshold);
    RUN_TEST(test_Healing_CounterIncrements);
    RUN_TEST(test_Healing_FailureResetsCounter);
    RUN_TEST(test_Healing_ClearsTNCSLC);
    RUN_TEST(test_Aging_NoAgeIfNotConfirmed);
    RUN_TEST(test_Aging_InvalidId_Safe);
    return UNITY_END();
}
