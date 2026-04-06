#ifndef DCM_DID_TABLE_H
#define DCM_DID_TABLE_H

#include "dem_types.h"
#include <stdint.h>

#define DCM_SESSION_DEFAULT      (0x01U)
#define DCM_SESSION_EXTENDED     (0x02U)
#define DCM_SESSION_PROGRAMMING  (0x04U)
#define DCM_SESSION_ALL          (0x07U)

#define DCM_SEC_NONE    (0x00U)
#define DCM_SEC_LEVEL1  (0x01U)

typedef struct {
    uint16_t       did;
    uint8_t        sessionMask;
    uint8_t        securityLevel;
    Std_ReturnType (*readFn)(uint8_t *buf, uint16_t *len);
    Std_ReturnType (*writeFn)(const uint8_t *buf, uint16_t len);
} DCM_DidEntry_t;

const DCM_DidEntry_t *DCM_Did_Find(uint16_t did);
Std_ReturnType        DCM_Did_CheckAccess(const DCM_DidEntry_t *entry,
                                           uint8_t sessionType,
                                           uint8_t securityLevel,
                                           uint8_t isWrite);
uint16_t              DCM_Did_GetCount(void);

#endif
