#include "dem_debounce.h"
#include "dem_core.h"
#include "dem_types.h"
#include "dem_cfg.h"
#include <string.h>

typedef struct {
    Dem_EventIdType     eventId;
    Dem_EventStatusType lastStatus;
    uint32_t            timerStart_ms;
    uint32_t            timerDuration_ms;
    Dem_BooleanType     timerRunning;
    Dem_BooleanType     isActive;
} Dem_DebounceEntryType;

static Dem_DebounceEntryType Dem_DebounceTable[DEM_MAX_EVENTS];

void Dem_Debounce_Init(void)
{
    (void)memset(Dem_DebounceTable, 0x00, sizeof(Dem_DebounceTable));
}

Std_ReturnType Dem_Debounce_ProcessTimeBased(Dem_EventIdType     eventId,
                                             Dem_EventStatusType status,
                                             uint32_t            currentTick_ms)
{
    Dem_DebounceEntryType *entry;

    if (eventId == DEM_EVENT_ID_INVALID)                { return E_NOT_OK; }
    if (eventId >= (Dem_EventIdType)DEM_MAX_EVENTS)     { return E_NOT_OK; }

    entry = &Dem_DebounceTable[eventId];

    if (entry->isActive == DEM_FALSE)
    {
        entry->eventId  = eventId;
        entry->isActive = DEM_TRUE;
    }

    if (status == DEM_EVENT_STATUS_FAILED)
    {
        if ((entry->timerRunning == DEM_FALSE) ||
            (entry->lastStatus   != DEM_EVENT_STATUS_FAILED))
        {
            entry->timerStart_ms    = currentTick_ms;
            entry->timerDuration_ms = (uint32_t)DEM_DEBOUNCE_FAIL_TIME_MS;
            entry->timerRunning     = DEM_TRUE;
            entry->lastStatus       = DEM_EVENT_STATUS_FAILED;
        }
    }
    else if (status == DEM_EVENT_STATUS_PASSED)
    {
        if ((entry->timerRunning == DEM_FALSE) ||
            (entry->lastStatus   != DEM_EVENT_STATUS_PASSED))
        {
            entry->timerStart_ms    = currentTick_ms;
            entry->timerDuration_ms = (uint32_t)DEM_DEBOUNCE_PASS_TIME_MS;
            entry->timerRunning     = DEM_TRUE;
            entry->lastStatus       = DEM_EVENT_STATUS_PASSED;
        }
    }
    else
    {
        /* PREFAILED / PREPASSED — no time-based action */
    }

    return E_OK;
}

void Dem_Debounce_MainFunction(uint32_t currentTick_ms)
{
    uint16_t idx;

    for (idx = 0U; idx < (uint16_t)DEM_MAX_EVENTS; idx++)
    {
        Dem_DebounceEntryType *entry = &Dem_DebounceTable[idx];

        if ((entry->isActive     == DEM_TRUE) &&
            (entry->timerRunning == DEM_TRUE))
        {
            uint32_t elapsed = currentTick_ms - entry->timerStart_ms;

            if (elapsed >= entry->timerDuration_ms)
            {
                entry->timerRunning = DEM_FALSE;
                (void)Dem_ReportErrorStatus(entry->eventId,
                                            entry->lastStatus);
            }
        }
    }
}
