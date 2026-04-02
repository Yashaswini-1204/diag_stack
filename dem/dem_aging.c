#include "dem_aging.h"
#include "dem_aging.h"
#include "dem_aging.h"
#include "dem_dtc.h"
#include "dem_core.h"
#include "dem_cfg.h"
#include "dem_types.h"
#include <string.h>

typedef struct {
    uint8_t agingCounter;
    uint8_t healingCounter;
    uint8_t failedThisCycle;
    uint8_t passedThisCycle;
} Dem_AgingEntry_t;

static Dem_AgingEntry_t s_aging[DEM_MAX_EVENTS];
static uint8_t          s_initialized = 0U;

void Dem_Aging_Init(void)
{
    memset(s_aging, 0, sizeof(s_aging));
    s_initialized = 1U;
}

void Dem_Aging_ReportEvent(Dem_EventIdType eventId,
                            Dem_EventStatusType status)
{
    if (!s_initialized)                  { return; }
    if (eventId == DEM_EVENT_ID_INVALID) { return; }
    if (eventId >= DEM_MAX_EVENTS)       { return; }

    if (status == DEM_EVENT_STATUS_FAILED ||
        status == DEM_EVENT_STATUS_PREFAILED)
    {
        s_aging[eventId].failedThisCycle = 1U;
        s_aging[eventId].healingCounter  = 0U;
    }
    else if (status == DEM_EVENT_STATUS_PASSED ||
             status == DEM_EVENT_STATUS_PREPASSED)
    {
        s_aging[eventId].passedThisCycle = 1U;
    }
}

void Dem_Aging_OperationCycleEnd(void)
{
    uint16_t i;
    uint8_t  statusByte;

    if (!s_initialized) { return; }

    for (i = 0U; i < (uint16_t)DEM_MAX_EVENTS; i++)
    {
        if (Dem_GetEventUdsStatus((Dem_EventIdType)i, &statusByte) != E_OK)
            goto next;
        if ((statusByte & DEM_UDS_STATUS_CDTC) == 0U)
            goto next;

        /* AGING */
        if (s_aging[i].failedThisCycle == 0U)
        {
            if (s_aging[i].agingCounter < (uint8_t)DEM_AGING_CYCLE_COUNT)
                s_aging[i].agingCounter++;
            if (s_aging[i].agingCounter >= (uint8_t)DEM_AGING_CYCLE_COUNT)
            {
                Dem_Dtc_AgeOut((Dem_EventIdType)i);
                s_aging[i].agingCounter   = 0U;
                s_aging[i].healingCounter = 0U;
            }
        }
        else
        {
            s_aging[i].agingCounter = 0U;
        }

        /* HEALING */
        if (s_aging[i].passedThisCycle == 1U &&
            s_aging[i].failedThisCycle == 0U)
        {
            if (s_aging[i].healingCounter < (uint8_t)DEM_HEALING_CYCLE_COUNT)
                s_aging[i].healingCounter++;
            if (s_aging[i].healingCounter >= (uint8_t)DEM_HEALING_CYCLE_COUNT)
            {
                Dem_Aging_ClearHealingBits((Dem_EventIdType)i);
                s_aging[i].healingCounter = 0U;
            }
        }
        else if (s_aging[i].failedThisCycle == 1U)
        {
            s_aging[i].healingCounter = 0U;
        }

next:
        s_aging[i].failedThisCycle = 0U;
        s_aging[i].passedThisCycle = 0U;
    }
}

uint8_t Dem_Aging_GetCounter(Dem_EventIdType eventId)
{
    if (eventId >= (Dem_EventIdType)DEM_MAX_EVENTS) { return 0U; }
    return s_aging[eventId].agingCounter;
}

uint8_t Dem_Aging_GetHealingCounter(Dem_EventIdType eventId)
{
    if (eventId >= (Dem_EventIdType)DEM_MAX_EVENTS) { return 0U; }
    return s_aging[eventId].healingCounter;
}
