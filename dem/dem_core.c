#include "dem_aging.h"
#include "dem_core.h"
#include "dem_types.h"
#include "dem_cfg.h"
#include "platform_api.h"
#include <stddef.h>
#include <string.h>

typedef struct {
    Dem_EventIdType  eventId;
    uint8_t          udsStatusByte;
    int16_t          debounceCounter;
    uint32_t         debounceTimer_ms;
    uint8_t          agingCounter;
    Dem_BooleanType  isActive;
} Dem_EventEntryType;

static Dem_EventEntryType Dem_EventTable[DEM_MAX_EVENTS];
static Dem_BooleanType    Dem_Initialized = DEM_FALSE;

#define DEM_STATUS_SET(byte, mask) ((byte) |=  (uint8_t)(mask))
#define DEM_STATUS_CLR(byte, mask) ((byte) &= (uint8_t)(~(uint8_t)(mask)))

void Dem_Init(void)
{
    uint16_t idx;
    (void)memset(Dem_EventTable, 0x00, sizeof(Dem_EventTable));
    for (idx = 0U; idx < (uint16_t)DEM_MAX_EVENTS; idx++)
    {
        Dem_EventTable[idx].eventId       = (Dem_EventIdType)idx;
        Dem_EventTable[idx].udsStatusByte = (uint8_t)DEM_UDS_STATUS_DEFAULT;
        Dem_EventTable[idx].isActive      = DEM_FALSE;
    }
    Dem_Initialized = DEM_TRUE;
}

Std_ReturnType Dem_ReportErrorStatus(Dem_EventIdType eventId,
                                     Dem_EventStatusType eventStatus)
{
    Dem_EventEntryType *entry;
    int16_t             counter;
    uint8_t             newStatus;

    if (Dem_Initialized != DEM_TRUE)                    { return E_NOT_OK; }
    if (eventId == DEM_EVENT_ID_INVALID)                { return E_NOT_OK; }
    if (eventId >= (Dem_EventIdType)DEM_MAX_EVENTS)     { return E_NOT_OK; }

    entry = &Dem_EventTable[eventId];
    if (entry->isActive == DEM_FALSE) { entry->isActive = DEM_TRUE; }

    counter = entry->debounceCounter;

    switch (eventStatus)
    {
        case DEM_EVENT_STATUS_FAILED:
            if (counter < (int16_t)32767) { counter++; }
            break;
        case DEM_EVENT_STATUS_PASSED:
            if (counter > (int16_t)0)     { counter--; }
            break;
        case DEM_EVENT_STATUS_PREFAILED:
            if (counter < (int16_t)32767) { counter++; }
            break;
        case DEM_EVENT_STATUS_PREPASSED:
            if (counter > (int16_t)0)     { counter--; }
            break;
        default:
            return E_NOT_OK;
    }
    entry->debounceCounter = counter;

    Platform_EnterCritical();
    newStatus = entry->udsStatusByte;

    switch (eventStatus)
    {
        case DEM_EVENT_STATUS_FAILED:
            if (counter >= (int16_t)DEM_DEBOUNCE_FAIL_THRESHOLD)
            {
                DEM_STATUS_SET(newStatus, DEM_UDS_STATUS_TF);
                DEM_STATUS_SET(newStatus, DEM_UDS_STATUS_TFTOC);
                DEM_STATUS_SET(newStatus, DEM_UDS_STATUS_PDTC);
                DEM_STATUS_SET(newStatus, DEM_UDS_STATUS_CDTC);
                DEM_STATUS_SET(newStatus, DEM_UDS_STATUS_TFSLC);
                DEM_STATUS_CLR(newStatus, DEM_UDS_STATUS_TNCTOC);
            }
            break;
        case DEM_EVENT_STATUS_PASSED:
            if (counter == (int16_t)0)
            {
                DEM_STATUS_CLR(newStatus, DEM_UDS_STATUS_TF);
                DEM_STATUS_CLR(newStatus, DEM_UDS_STATUS_TFTOC);
                DEM_STATUS_CLR(newStatus, DEM_UDS_STATUS_PDTC);
            }
            break;
        case DEM_EVENT_STATUS_PREFAILED:
        case DEM_EVENT_STATUS_PREPASSED:
        default:
            break;
    }

    entry->udsStatusByte = newStatus;
    Platform_ExitCritical();
    return E_OK;
}

Std_ReturnType Dem_GetEventUdsStatus(Dem_EventIdType eventId,
                                     uint8_t *statusByte)
{
    if (Dem_Initialized != DEM_TRUE)                    { return E_NOT_OK; }
    if (eventId == DEM_EVENT_ID_INVALID)                { return E_NOT_OK; }
    if (eventId >= (Dem_EventIdType)DEM_MAX_EVENTS)     { return E_NOT_OK; }
    if (statusByte == NULL)                             { return E_NOT_OK; }

    Platform_EnterCritical();
    *statusByte = Dem_EventTable[eventId].udsStatusByte;
    Platform_ExitCritical();
    return E_OK;
}

void Dem_OperationCycleStart(void)
{
    uint16_t idx;
    for (idx = 0U; idx < (uint16_t)DEM_MAX_EVENTS; idx++)
    {
        if (Dem_EventTable[idx].isActive == DEM_TRUE)
        {
            Platform_EnterCritical();
            DEM_STATUS_SET(Dem_EventTable[idx].udsStatusByte,
                           DEM_UDS_STATUS_TNCTOC);
            DEM_STATUS_CLR(Dem_EventTable[idx].udsStatusByte,
                           DEM_UDS_STATUS_TFTOC);
            Platform_ExitCritical();
        }
    }
}

void Dem_OperationCycleEnd(void)
{
    uint16_t idx;
    for (idx = 0U; idx < (uint16_t)DEM_MAX_EVENTS; idx++)
    {
        if (Dem_EventTable[idx].isActive == DEM_TRUE)
        {
            Platform_EnterCritical();
            DEM_STATUS_CLR(Dem_EventTable[idx].udsStatusByte,
                           DEM_UDS_STATUS_TFTOC);
            Platform_ExitCritical();
        }
    }
    Dem_Aging_OperationCycleEnd();
}

void Dem_MainFunction(void)
{
    uint16_t idx;
    for (idx = 0U; idx < (uint16_t)DEM_MAX_EVENTS; idx++)
    {
        if (Dem_EventTable[idx].isActive == DEM_TRUE)
        {
            if (Dem_EventTable[idx].debounceTimer_ms > 0U)
            {
                if (Dem_EventTable[idx].debounceTimer_ms >=
                    (uint32_t)DEM_MAIN_FUNCTION_PERIOD_MS)
                {
                    Dem_EventTable[idx].debounceTimer_ms -=
                        (uint32_t)DEM_MAIN_FUNCTION_PERIOD_MS;
                }
                else
                {
                    Dem_EventTable[idx].debounceTimer_ms = 0U;
                }
            }
        }
    }
    Platform_WdgTrigger();
}

/* Called by dem_aging.c when healing threshold reached */
void Dem_Aging_ClearHealingBits(Dem_EventIdType eventId)
{
    if (eventId >= (Dem_EventIdType)DEM_MAX_EVENTS) { return; }
    Platform_EnterCritical();
    Dem_EventTable[eventId].udsStatusByte &=
        ~((uint8_t)(DEM_UDS_STATUS_TNCSLC | DEM_UDS_STATUS_WIR));
    Platform_ExitCritical();
}
