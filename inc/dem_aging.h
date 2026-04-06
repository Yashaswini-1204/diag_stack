#ifndef DEM_AGING_H
#define DEM_AGING_H

#include "dem_types.h"

void    Dem_Aging_Init(void);
void    Dem_Aging_ReportEvent(Dem_EventIdType eventId,
                               Dem_EventStatusType status);
void    Dem_Aging_OperationCycleEnd(void);
uint8_t Dem_Aging_GetCounter(Dem_EventIdType eventId);
uint8_t Dem_Aging_GetHealingCounter(Dem_EventIdType eventId);
void    Dem_Dtc_AgeOut(Dem_EventIdType eventId);
void    Dem_Aging_ClearHealingBits(Dem_EventIdType eventId);

#endif
