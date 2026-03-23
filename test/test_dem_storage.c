#include "unity.h"
#include "dem_dtc.h"
#include "dem_nvm.h"
#include "dem_types.h"
#include "dem_cfg.h"
#include <stdio.h>

#define NVM_PRIMARY_PATH  "/tmp/dem_nvm_1.bin"
#define NVM_MIRROR_PATH   "/tmp/dem_nvm_2.bin"

void setUp(void)
{
    Dem_Dtc_Init();
    Dem_Nvm_Init();
    (void)remove(NVM_PRIMARY_PATH);
    (void)remove(NVM_MIRROR_PATH);
}

void tearDown(void) {}

void test_DtcStore_SingleEntry(void)
{
    uint8_t status = 0x00U;
    TEST_ASSERT_EQUAL(E_OK,
        Dem_Dtc_Store(1U, 0x010203U, 0x08U, 1U, NULL, 0U));
    TEST_ASSERT_EQUAL_UINT8(1U, Dem_Dtc_GetCount());
    TEST_ASSERT_EQUAL(E_OK, Dem_Dtc_GetStatus(0x010203U, &status));
    TEST_ASSERT_EQUAL_HEX8(0x08U, status);
}

void test_DtcStore_UpdateExisting(void)
{
    uint8_t status = 0x00U;
    TEST_ASSERT_EQUAL(E_OK,
        Dem_Dtc_Store(1U, 0x010203U, 0x08U, 1U, NULL, 0U));
    TEST_ASSERT_EQUAL(E_OK,
        Dem_Dtc_Store(1U, 0x010203U, 0x09U, 1U, NULL, 0U));
    TEST_ASSERT_EQUAL_UINT8(1U, Dem_Dtc_GetCount());
    TEST_ASSERT_EQUAL(E_OK, Dem_Dtc_GetStatus(0x010203U, &status));
    TEST_ASSERT_EQUAL_HEX8(0x09U, status);
}

void test_DtcStore_FillMemory(void)
{
    uint8_t i;
    for (i = 1U; i <= (uint8_t)DEM_MAX_DTC_ENTRIES; i++)
    {
        TEST_ASSERT_EQUAL(E_OK,
            Dem_Dtc_Store((Dem_EventIdType)i,
                          (uint32_t)i, 0x01U, 5U, NULL, 0U));
    }
    TEST_ASSERT_EQUAL_UINT8((uint8_t)DEM_MAX_DTC_ENTRIES,
                            Dem_Dtc_GetCount());
}

void test_DtcStore_Displacement(void)
{
    uint8_t  i;
    uint32_t foundDtc    = 0U;
    uint8_t  foundStatus = 0U;
    uint8_t  idx;
    uint8_t  newEntryFound = 0U;

    for (i = 1U; i <= (uint8_t)DEM_MAX_DTC_ENTRIES; i++)
    {
        TEST_ASSERT_EQUAL(E_OK,
            Dem_Dtc_Store((Dem_EventIdType)i,
                          (uint32_t)(0x100000U + i),
                          0x01U, 5U, NULL, 0U));
    }
    TEST_ASSERT_EQUAL(E_OK,
        Dem_Dtc_Store(21U, 0x200001U, 0x02U, 1U, NULL, 0U));
    TEST_ASSERT_EQUAL_UINT8((uint8_t)DEM_MAX_DTC_ENTRIES,
                            Dem_Dtc_GetCount());

    for (idx = 0U; idx < (uint8_t)DEM_MAX_DTC_ENTRIES; idx++)
    {
        if (Dem_Dtc_GetByIndex(idx, &foundDtc, &foundStatus) == E_OK)
        {
            if (foundDtc == 0x200001U)
            {
                newEntryFound = 1U;
                break;
            }
        }
    }
    TEST_ASSERT_EQUAL_UINT8(1U, newEntryFound);
}

void test_DtcClear_SingleDtc(void)
{
    uint8_t status = 0x00U;
    TEST_ASSERT_EQUAL(E_OK,
        Dem_Dtc_Store(1U, 0x000001U, 0x01U, 1U, NULL, 0U));
    TEST_ASSERT_EQUAL(E_OK,
        Dem_Dtc_Store(2U, 0x000002U, 0x01U, 1U, NULL, 0U));
    TEST_ASSERT_EQUAL(E_OK,
        Dem_Dtc_Store(3U, 0x000003U, 0x01U, 1U, NULL, 0U));
    TEST_ASSERT_EQUAL(E_OK, Dem_Dtc_Clear(0x000002U));
    TEST_ASSERT_EQUAL_UINT8(2U, Dem_Dtc_GetCount());
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_Dtc_GetStatus(0x000002U, &status));
}

void test_DtcClear_All(void)
{
    uint8_t i;
    for (i = 1U; i <= 5U; i++)
    {
        TEST_ASSERT_EQUAL(E_OK,
            Dem_Dtc_Store((Dem_EventIdType)i,
                          (uint32_t)(0x000010U + i),
                          0x01U, 1U, NULL, 0U));
    }
    TEST_ASSERT_EQUAL_UINT8(5U, Dem_Dtc_GetCount());
    TEST_ASSERT_EQUAL(E_OK,
        Dem_Dtc_Clear((uint32_t)DEM_DTC_CLEAR_ALL));
    TEST_ASSERT_EQUAL_UINT8(0U, Dem_Dtc_GetCount());
}

void test_NvmSaveAndLoad(void)
{
    uint8_t status = 0x00U;
    TEST_ASSERT_EQUAL(E_OK,
        Dem_Dtc_Store(1U, 0xAA0001U, 0x08U, 1U, NULL, 0U));
    TEST_ASSERT_EQUAL(E_OK,
        Dem_Dtc_Store(2U, 0xAA0002U, 0x08U, 1U, NULL, 0U));
    TEST_ASSERT_EQUAL(E_OK,
        Dem_Dtc_Store(3U, 0xAA0003U, 0x08U, 1U, NULL, 0U));
    TEST_ASSERT_EQUAL(E_OK, Dem_Nvm_Save());
    Dem_Dtc_Init();
    TEST_ASSERT_EQUAL_UINT8(0U, Dem_Dtc_GetCount());
    TEST_ASSERT_EQUAL(E_OK, Dem_Nvm_Load());
    TEST_ASSERT_EQUAL_UINT8(3U, Dem_Dtc_GetCount());
    TEST_ASSERT_EQUAL(E_OK, Dem_Dtc_GetStatus(0xAA0001U, &status));
    TEST_ASSERT_EQUAL_HEX8(0x08U, status);
}

void test_NvmLoad_NoFile_ReturnsNotOk(void)
{
    TEST_ASSERT_EQUAL(E_NOT_OK, Dem_Nvm_Load());
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_DtcStore_SingleEntry);
    RUN_TEST(test_DtcStore_UpdateExisting);
    RUN_TEST(test_DtcStore_FillMemory);
    RUN_TEST(test_DtcStore_Displacement);
    RUN_TEST(test_DtcClear_SingleDtc);
    RUN_TEST(test_DtcClear_All);
    RUN_TEST(test_NvmSaveAndLoad);
    RUN_TEST(test_NvmLoad_NoFile_ReturnsNotOk);
    return UNITY_END();
}
