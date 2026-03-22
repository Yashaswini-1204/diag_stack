#ifndef DEM_DEBOUNCE_H
#define DEM_DEBOUNCE_H

#include "dem_types.h"
#include "dem_cfg.h"

void           Dem_Debounce_Init(void);
Std_ReturnType Dem_Debounce_ProcessTimeBased(Dem_EventIdType eventId,
                                             Dem_EventStatusType status,
                                             uint32_t currentTick_ms);
void           Dem_Debounce_MainFunction(uint32_t currentTick_ms);

#endif
