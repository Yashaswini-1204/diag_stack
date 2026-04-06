#ifndef DEM_CORE_H
#define DEM_CORE_H

#include "dem_types.h"
#include "dem_cfg.h"

void           Dem_Init(void);
Std_ReturnType Dem_ReportErrorStatus(Dem_EventIdType eventId,
                                     Dem_EventStatusType eventStatus);
Std_ReturnType Dem_GetEventUdsStatus(Dem_EventIdType eventId,
                                     uint8_t *statusByte);
void           Dem_OperationCycleStart(void);
void           Dem_OperationCycleEnd(void);
void           Dem_MainFunction(void);

#endif
