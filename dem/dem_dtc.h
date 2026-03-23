#ifndef DEM_DTC_H
#define DEM_DTC_H

#include "dem_types.h"
#include "dem_cfg.h"

#define DEM_DTC_CLEAR_ALL  (0x00FFFFFFU)

void           Dem_Dtc_Init(void);
Std_ReturnType Dem_Dtc_Store(Dem_EventIdType eventId,
                              uint32_t        dtcNumber,
                              uint8_t         statusByte,
                              uint8_t         priority,
                              const uint8_t  *freezeFrame,
                              uint8_t         freezeFrameLen);
Std_ReturnType Dem_Dtc_GetStatus(uint32_t dtcNumber, uint8_t *statusByte);
Std_ReturnType Dem_Dtc_Clear(uint32_t dtcNumber);
uint8_t        Dem_Dtc_GetCount(void);
Std_ReturnType Dem_Dtc_GetByIndex(uint8_t index,
                                   uint32_t *dtcNumber,
                                   uint8_t  *statusByte);

#endif
