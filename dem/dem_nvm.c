#include "dem_nvm.h"
#include "dem_dtc.h"
#include "dem_cfg.h"
#include "dem_types.h"
#include "platform_api.h"
#include <string.h>
#include <stddef.h>

#define DEM_NVM_ENTRY_SIZE (4U + 1U + 1U + 1U + DEM_FREEZE_FRAME_SIZE)
#define DEM_NVM_DATA_SIZE  (DEM_MAX_DTC_ENTRIES * DEM_NVM_ENTRY_SIZE)

static const uint8_t Dem_Nvm_Magic[4U] = {'D', 'E', 'M', '1'};

typedef struct {
    uint8_t  magic[4U];
    uint8_t  count;
    uint8_t  data[DEM_NVM_DATA_SIZE];
    uint32_t crc32;
} Dem_NvmBlockType;

static Dem_NvmBlockType Dem_NvmBlock;

static uint32_t Dem_Nvm_Crc32(const uint8_t *buf, uint16_t len)
{
    uint32_t crc = 0xFFFFFFFFU;
    uint16_t i;
    uint8_t  bit;
    for (i = 0U; i < len; i++)
    {
        crc ^= (uint32_t)buf[i];
        for (bit = 0U; bit < 8U; bit++)
        {
            if ((crc & 1U) != 0U) { crc = (crc >> 1U) ^ 0xEDB88320U; }
            else                  { crc >>= 1U; }
        }
    }
    return crc ^ 0xFFFFFFFFU;
}

static void WriteU32LE(uint8_t *dst, uint32_t value)
{
    dst[0U] = (uint8_t)( value        & 0xFFU);
    dst[1U] = (uint8_t)((value >> 8U) & 0xFFU);
    dst[2U] = (uint8_t)((value >>16U) & 0xFFU);
    dst[3U] = (uint8_t)((value >>24U) & 0xFFU);
}

static uint32_t ReadU32LE(const uint8_t *src)
{
    return ((uint32_t)src[0U])
         | ((uint32_t)src[1U] <<  8U)
         | ((uint32_t)src[2U] << 16U)
         | ((uint32_t)src[3U] << 24U);
}

static void Dem_Nvm_Serialize(void)
{
    uint8_t  slotIdx;
    uint8_t  entryCount = 0U;
    uint32_t dtcNumber;
    uint8_t  statusByte;
    uint8_t *dst;

    for (slotIdx = 0U; slotIdx < (uint8_t)DEM_MAX_DTC_ENTRIES; slotIdx++)
    {
        if (Dem_Dtc_GetByIndex(slotIdx, &dtcNumber, &statusByte) == E_OK)
        {
            dst = &Dem_NvmBlock.data[entryCount *
                                     (uint8_t)DEM_NVM_ENTRY_SIZE];
            WriteU32LE(dst, dtcNumber);
            dst[4U] = statusByte;
            entryCount++;
        }
    }
    Dem_NvmBlock.count = entryCount;
}

static void Dem_Nvm_Deserialize(void)
{
    uint8_t         entryIdx;
    uint8_t         limit;
    const uint8_t  *src;
    uint32_t        dtcNumber;
    uint8_t         statusByte;
    uint8_t         priority;
    Dem_EventIdType syntheticId;

    limit = (Dem_NvmBlock.count < (uint8_t)DEM_MAX_DTC_ENTRIES)
            ? Dem_NvmBlock.count
            : (uint8_t)DEM_MAX_DTC_ENTRIES;

    for (entryIdx = 0U; entryIdx < limit; entryIdx++)
    {
        src        = &Dem_NvmBlock.data[entryIdx *
                                        (uint8_t)DEM_NVM_ENTRY_SIZE];
        dtcNumber  = ReadU32LE(src);
        statusByte = src[4U];
        priority   = src[6U];
        syntheticId = (Dem_EventIdType)(entryIdx + 1U);

        (void)Dem_Dtc_Store(syntheticId, dtcNumber, statusByte,
                            priority, &src[7U],
                            (uint8_t)DEM_FREEZE_FRAME_SIZE);
    }
}

static Dem_BooleanType Dem_Nvm_IsBlockValid(void)
{
    uint32_t computed;
    size_t   coverLen;

    if ((Dem_NvmBlock.magic[0U] != Dem_Nvm_Magic[0U]) ||
        (Dem_NvmBlock.magic[1U] != Dem_Nvm_Magic[1U]) ||
        (Dem_NvmBlock.magic[2U] != Dem_Nvm_Magic[2U]) ||
        (Dem_NvmBlock.magic[3U] != Dem_Nvm_Magic[3U]))
    {
        return DEM_FALSE;
    }

    coverLen = offsetof(Dem_NvmBlockType, crc32);
    computed = Dem_Nvm_Crc32((const uint8_t *)&Dem_NvmBlock,
                              (uint16_t)coverLen);

    if (computed != Dem_NvmBlock.crc32) { return DEM_FALSE; }

    return DEM_TRUE;
}

void Dem_Nvm_Init(void)
{
    (void)memset(&Dem_NvmBlock, 0x00, sizeof(Dem_NvmBlockType));
}

Std_ReturnType Dem_Nvm_Save(void)
{
    size_t   coverLen;
    uint16_t blockLen;
    uint8_t  wr1;
    uint8_t  wr2;

    (void)memset(&Dem_NvmBlock, 0x00, sizeof(Dem_NvmBlockType));

    Dem_NvmBlock.magic[0U] = Dem_Nvm_Magic[0U];
    Dem_NvmBlock.magic[1U] = Dem_Nvm_Magic[1U];
    Dem_NvmBlock.magic[2U] = Dem_Nvm_Magic[2U];
    Dem_NvmBlock.magic[3U] = Dem_Nvm_Magic[3U];

    Dem_Nvm_Serialize();

    coverLen           = offsetof(Dem_NvmBlockType, crc32);
    Dem_NvmBlock.crc32 = Dem_Nvm_Crc32((const uint8_t *)&Dem_NvmBlock,
                                         (uint16_t)coverLen);

    blockLen = (uint16_t)sizeof(Dem_NvmBlockType);
    wr1 = Platform_NvmWrite((uint16_t)DEM_NVM_PRIMARY_BLOCK_ID,
                             &Dem_NvmBlock, blockLen);
    wr2 = Platform_NvmWrite((uint16_t)DEM_NVM_MIRROR_BLOCK_ID,
                             &Dem_NvmBlock, blockLen);

    return ((wr1 == PLATFORM_OK) && (wr2 == PLATFORM_OK))
           ? E_OK : E_NOT_OK;
}

Std_ReturnType Dem_Nvm_Load(void)
{
    uint16_t blockLen = (uint16_t)sizeof(Dem_NvmBlockType);
    uint8_t  rd;

    (void)memset(&Dem_NvmBlock, 0x00, sizeof(Dem_NvmBlockType));
    rd = Platform_NvmRead((uint16_t)DEM_NVM_PRIMARY_BLOCK_ID,
                           &Dem_NvmBlock, blockLen);
    if ((rd == PLATFORM_OK) && (Dem_Nvm_IsBlockValid() == DEM_TRUE))
    {
        Dem_Nvm_Deserialize();
        return E_OK;
    }

    (void)memset(&Dem_NvmBlock, 0x00, sizeof(Dem_NvmBlockType));
    rd = Platform_NvmRead((uint16_t)DEM_NVM_MIRROR_BLOCK_ID,
                           &Dem_NvmBlock, blockLen);
    if ((rd == PLATFORM_OK) && (Dem_Nvm_IsBlockValid() == DEM_TRUE))
    {
        Dem_Nvm_Deserialize();
        return E_OK;
    }

    return E_NOT_OK;
}
