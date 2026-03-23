#include "dem_dtc.h"
#include "dem_types.h"
#include "dem_cfg.h"
#include <string.h>
#include <stddef.h>

typedef struct {
    Dem_EventIdType  eventId;
    uint32_t         dtcNumber;
    uint8_t          udsStatusByte;
    uint8_t          occurrenceCounter;
    uint8_t          agingCounter;
    uint8_t          priority;
    Dem_BooleanType  isOccupied;
    uint8_t          freezeFrame[DEM_FREEZE_FRAME_SIZE];
    uint8_t          extData[DEM_EXT_DATA_SIZE];
} Dem_DtcEntryType;

static Dem_DtcEntryType Dem_DtcPrimaryMemory[DEM_MAX_DTC_ENTRIES];

static void CopyFreezeFrame(Dem_DtcEntryType *entry,
                             const uint8_t *src, uint8_t len)
{
    uint8_t copyLen;
    if ((src != NULL) && (len > 0U))
    {
        copyLen = (len < (uint8_t)DEM_FREEZE_FRAME_SIZE)
                  ? len : (uint8_t)DEM_FREEZE_FRAME_SIZE;
        (void)memcpy(entry->freezeFrame, src, (size_t)copyLen);
    }
}

static uint8_t FindByEventId(Dem_EventIdType eventId)
{
    uint8_t idx;
    for (idx = 0U; idx < (uint8_t)DEM_MAX_DTC_ENTRIES; idx++)
    {
        if ((Dem_DtcPrimaryMemory[idx].isOccupied == DEM_TRUE) &&
            (Dem_DtcPrimaryMemory[idx].eventId    == eventId))
        {
            return idx;
        }
    }
    return (uint8_t)DEM_MAX_DTC_ENTRIES;
}

static uint8_t FindByDtcNumber(uint32_t dtcNumber)
{
    uint8_t idx;
    for (idx = 0U; idx < (uint8_t)DEM_MAX_DTC_ENTRIES; idx++)
    {
        if ((Dem_DtcPrimaryMemory[idx].isOccupied == DEM_TRUE) &&
            (Dem_DtcPrimaryMemory[idx].dtcNumber  == dtcNumber))
        {
            return idx;
        }
    }
    return (uint8_t)DEM_MAX_DTC_ENTRIES;
}

static uint8_t FindFreeSlot(void)
{
    uint8_t idx;
    for (idx = 0U; idx < (uint8_t)DEM_MAX_DTC_ENTRIES; idx++)
    {
        if (Dem_DtcPrimaryMemory[idx].isOccupied == DEM_FALSE)
        {
            return idx;
        }
    }
    return (uint8_t)DEM_MAX_DTC_ENTRIES;
}

static uint8_t FindDisplacementCandidate(void)
{
    uint8_t worstIdx  = (uint8_t)DEM_MAX_DTC_ENTRIES;
    uint8_t worstPrio = 0U;
    uint8_t idx;
    for (idx = 0U; idx < (uint8_t)DEM_MAX_DTC_ENTRIES; idx++)
    {
        if ((Dem_DtcPrimaryMemory[idx].isOccupied == DEM_TRUE) &&
            (Dem_DtcPrimaryMemory[idx].priority > worstPrio))
        {
            worstPrio = Dem_DtcPrimaryMemory[idx].priority;
            worstIdx  = idx;
        }
    }
    return worstIdx;
}

static void WriteEntry(uint8_t idx, Dem_EventIdType eventId,
                       uint32_t dtcNumber, uint8_t statusByte,
                       uint8_t priority, const uint8_t *freezeFrame,
                       uint8_t freezeFrameLen)
{
    Dem_DtcEntryType *entry = &Dem_DtcPrimaryMemory[idx];
    (void)memset(entry, 0x00, sizeof(Dem_DtcEntryType));
    entry->eventId           = eventId;
    entry->dtcNumber         = dtcNumber;
    entry->udsStatusByte     = statusByte;
    entry->priority          = priority;
    entry->occurrenceCounter = 1U;
    entry->agingCounter      = (uint8_t)DEM_AGING_CYCLE_COUNT;
    entry->isOccupied        = DEM_TRUE;
    CopyFreezeFrame(entry, freezeFrame, freezeFrameLen);
}

void Dem_Dtc_Init(void)
{
    (void)memset(Dem_DtcPrimaryMemory, 0x00, sizeof(Dem_DtcPrimaryMemory));
}

Std_ReturnType Dem_Dtc_Store(Dem_EventIdType eventId,
                              uint32_t dtcNumber, uint8_t statusByte,
                              uint8_t priority, const uint8_t *freezeFrame,
                              uint8_t freezeFrameLen)
{
    uint8_t           existIdx;
    uint8_t           freeIdx;
    uint8_t           displaceIdx;
    Dem_DtcEntryType *entry;

    if (eventId == DEM_EVENT_ID_INVALID) { return E_NOT_OK; }

    existIdx = FindByEventId(eventId);
    if (existIdx < (uint8_t)DEM_MAX_DTC_ENTRIES)
    {
        entry = &Dem_DtcPrimaryMemory[existIdx];
        entry->udsStatusByte = statusByte;
        if (entry->occurrenceCounter < 0xFFU)
        {
            entry->occurrenceCounter++;
        }
        CopyFreezeFrame(entry, freezeFrame, freezeFrameLen);
        return E_OK;
    }

    freeIdx = FindFreeSlot();
    if (freeIdx < (uint8_t)DEM_MAX_DTC_ENTRIES)
    {
        WriteEntry(freeIdx, eventId, dtcNumber, statusByte,
                   priority, freezeFrame, freezeFrameLen);
        return E_OK;
    }

    displaceIdx = FindDisplacementCandidate();
    if (displaceIdx < (uint8_t)DEM_MAX_DTC_ENTRIES)
    {
        WriteEntry(displaceIdx, eventId, dtcNumber, statusByte,
                   priority, freezeFrame, freezeFrameLen);
    }
    return E_OK;
}

Std_ReturnType Dem_Dtc_GetStatus(uint32_t dtcNumber, uint8_t *statusByte)
{
    uint8_t idx;
    if (statusByte == NULL) { return E_NOT_OK; }
    idx = FindByDtcNumber(dtcNumber);
    if (idx >= (uint8_t)DEM_MAX_DTC_ENTRIES) { return E_NOT_OK; }
    *statusByte = Dem_DtcPrimaryMemory[idx].udsStatusByte;
    return E_OK;
}

Std_ReturnType Dem_Dtc_Clear(uint32_t dtcNumber)
{
    uint8_t idx;
    if (dtcNumber == (uint32_t)DEM_DTC_CLEAR_ALL)
    {
        (void)memset(Dem_DtcPrimaryMemory, 0x00,
                     sizeof(Dem_DtcPrimaryMemory));
        return E_OK;
    }
    idx = FindByDtcNumber(dtcNumber);
    if (idx >= (uint8_t)DEM_MAX_DTC_ENTRIES) { return E_NOT_OK; }
    (void)memset(&Dem_DtcPrimaryMemory[idx], 0x00,
                 sizeof(Dem_DtcEntryType));
    return E_OK;
}

uint8_t Dem_Dtc_GetCount(void)
{
    uint8_t count = 0U;
    uint8_t idx;
    for (idx = 0U; idx < (uint8_t)DEM_MAX_DTC_ENTRIES; idx++)
    {
        if (Dem_DtcPrimaryMemory[idx].isOccupied == DEM_TRUE)
        {
            count++;
        }
    }
    return count;
}

Std_ReturnType Dem_Dtc_GetByIndex(uint8_t index,
                                   uint32_t *dtcNumber,
                                   uint8_t  *statusByte)
{
    if ((dtcNumber == NULL) || (statusByte == NULL)) { return E_NOT_OK; }
    if (index >= (uint8_t)DEM_MAX_DTC_ENTRIES)       { return E_NOT_OK; }
    if (Dem_DtcPrimaryMemory[index].isOccupied == DEM_FALSE)
    {
        return E_NOT_OK;
    }
    *dtcNumber  = Dem_DtcPrimaryMemory[index].dtcNumber;
    *statusByte = Dem_DtcPrimaryMemory[index].udsStatusByte;
    return E_OK;
}
