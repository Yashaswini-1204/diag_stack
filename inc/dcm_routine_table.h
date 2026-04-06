#ifndef DCM_ROUTINE_TABLE_H
#define DCM_ROUTINE_TABLE_H

#include "dem_types.h"
#include "dcm_did_table.h"
#include <stdint.h>

#define DCM_ROUTINE_START   (0x01U)
#define DCM_ROUTINE_STOP    (0x02U)
#define DCM_ROUTINE_RESULT  (0x03U)

#define DCM_ROUTINE_IDLE      (0x00U)
#define DCM_ROUTINE_RUNNING   (0x01U)
#define DCM_ROUTINE_COMPLETED (0x02U)
#define DCM_ROUTINE_FAILED    (0x03U)

typedef struct {
    uint16_t routineId;
    uint8_t  sessionMask;
    uint8_t  securityLevel;
    uint8_t  status;
    Std_ReturnType (*startFn)(const uint8_t *inBuf,  uint16_t  inLen,
                               uint8_t       *outBuf, uint16_t *outLen);
    Std_ReturnType (*stopFn)(uint8_t *outBuf, uint16_t *outLen);
    Std_ReturnType (*resultFn)(uint8_t *outBuf, uint16_t *outLen);
} DCM_RoutineEntry_t;

DCM_RoutineEntry_t *DCM_Routine_Find(uint16_t routineId);
Std_ReturnType      DCM_Routine_CheckAccess(const DCM_RoutineEntry_t *entry,
                                             uint8_t sessionType,
                                             uint8_t securityLevel);
Std_ReturnType      DCM_Routine_Execute(DCM_RoutineEntry_t *entry,
                                         uint8_t             controlType,
                                         const uint8_t      *inBuf,
                                         uint16_t            inLen,
                                         uint8_t            *outBuf,
                                         uint16_t           *outLen);
#endif
