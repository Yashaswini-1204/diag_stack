#include "dcm_routine_table.h"
#include "dem_types.h"
#include "dcm_did_table.h"
#include <stddef.h>

static DCM_RoutineEntry_t DCM_RoutineTable[];

static DCM_RoutineEntry_t *s_FF00 = NULL;
static DCM_RoutineEntry_t *s_FF01 = NULL;
static DCM_RoutineEntry_t *s_0200 = NULL;
static DCM_RoutineEntry_t *s_0201 = NULL;
static Dem_BooleanType     s_initialized = DEM_FALSE;

static Std_ReturnType Start_FF00(const uint8_t *inBuf, uint16_t inLen,
                                  uint8_t *outBuf, uint16_t *outLen);
static Std_ReturnType Stop_FF00(uint8_t *outBuf, uint16_t *outLen);
static Std_ReturnType Result_FF00(uint8_t *outBuf, uint16_t *outLen);
static Std_ReturnType Start_FF01(const uint8_t *inBuf, uint16_t inLen,
                                  uint8_t *outBuf, uint16_t *outLen);
static Std_ReturnType Stop_FF01(uint8_t *outBuf, uint16_t *outLen);
static Std_ReturnType Result_FF01(uint8_t *outBuf, uint16_t *outLen);
static Std_ReturnType Start_0200(const uint8_t *inBuf, uint16_t inLen,
                                  uint8_t *outBuf, uint16_t *outLen);
static Std_ReturnType Stop_0200(uint8_t *outBuf, uint16_t *outLen);
static Std_ReturnType Result_0200(uint8_t *outBuf, uint16_t *outLen);
static Std_ReturnType Start_0201(const uint8_t *inBuf, uint16_t inLen,
                                  uint8_t *outBuf, uint16_t *outLen);
static Std_ReturnType Stop_0201(uint8_t *outBuf, uint16_t *outLen);
static Std_ReturnType Result_0201(uint8_t *outBuf, uint16_t *outLen);

static DCM_RoutineEntry_t DCM_RoutineTable[] = {
    {0xFF00U, DCM_SESSION_EXTENDED,    DCM_SEC_NONE,   DCM_ROUTINE_IDLE,
     Start_FF00, Stop_FF00, Result_FF00},
    {0xFF01U, DCM_SESSION_PROGRAMMING, DCM_SEC_LEVEL1, DCM_ROUTINE_IDLE,
     Start_FF01, Stop_FF01, Result_FF01},
    {0x0200U, DCM_SESSION_EXTENDED,    DCM_SEC_NONE,   DCM_ROUTINE_IDLE,
     Start_0200, Stop_0200, Result_0200},
    {0x0201U, DCM_SESSION_PROGRAMMING, DCM_SEC_LEVEL1, DCM_ROUTINE_IDLE,
     Start_0201, Stop_0201, Result_0201},
};

#define DCM_ROUTINE_TABLE_SIZE \
    ((uint16_t)(sizeof(DCM_RoutineTable)/sizeof(DCM_RoutineTable[0U])))

static void DCM_Routine_InitPointers(void)
{
    s_FF00 = &DCM_RoutineTable[0U];
    s_FF01 = &DCM_RoutineTable[1U];
    s_0200 = &DCM_RoutineTable[2U];
    s_0201 = &DCM_RoutineTable[3U];
    s_initialized = DEM_TRUE;
}

static Std_ReturnType Start_FF00(const uint8_t *inBuf, uint16_t inLen,
                                  uint8_t *outBuf, uint16_t *outLen)
{ (void)inBuf; (void)inLen;
  s_FF00->status = DCM_ROUTINE_RUNNING; outBuf[0U] = 0x00U; *outLen = 1U;
  return E_OK; }

static Std_ReturnType Stop_FF00(uint8_t *outBuf, uint16_t *outLen)
{ (void)outBuf; s_FF00->status = DCM_ROUTINE_IDLE; *outLen = 0U; return E_OK; }

static Std_ReturnType Result_FF00(uint8_t *outBuf, uint16_t *outLen)
{ outBuf[0U] = (s_FF00->status == (uint8_t)DCM_ROUTINE_COMPLETED) ? 0x01U : 0x00U;
  *outLen = 1U; return E_OK; }

static Std_ReturnType Start_FF01(const uint8_t *inBuf, uint16_t inLen,
                                  uint8_t *outBuf, uint16_t *outLen)
{ (void)inBuf; (void)inLen; (void)outBuf;
  s_FF01->status = DCM_ROUTINE_RUNNING; *outLen = 0U; return E_OK; }

static Std_ReturnType Stop_FF01(uint8_t *outBuf, uint16_t *outLen)
{ (void)outBuf; s_FF01->status = DCM_ROUTINE_IDLE; *outLen = 0U; return E_OK; }

static Std_ReturnType Result_FF01(uint8_t *outBuf, uint16_t *outLen)
{ outBuf[0U] = s_FF01->status; *outLen = 1U; return E_OK; }

static Std_ReturnType Start_0200(const uint8_t *inBuf, uint16_t inLen,
                                  uint8_t *outBuf, uint16_t *outLen)
{ (void)inBuf; (void)inLen;
  s_0200->status = DCM_ROUTINE_COMPLETED; outBuf[0U] = 0x01U; *outLen = 1U;
  return E_OK; }

static Std_ReturnType Stop_0200(uint8_t *outBuf, uint16_t *outLen)
{ (void)outBuf; s_0200->status = DCM_ROUTINE_IDLE; *outLen = 0U; return E_OK; }

static Std_ReturnType Result_0200(uint8_t *outBuf, uint16_t *outLen)
{ outBuf[0U] = s_0200->status; *outLen = 1U; return E_OK; }

static Std_ReturnType Start_0201(const uint8_t *inBuf, uint16_t inLen,
                                  uint8_t *outBuf, uint16_t *outLen)
{ (void)inBuf; (void)inLen; (void)outBuf;
  s_0201->status = DCM_ROUTINE_COMPLETED; *outLen = 0U; return E_OK; }

static Std_ReturnType Stop_0201(uint8_t *outBuf, uint16_t *outLen)
{ (void)outBuf; s_0201->status = DCM_ROUTINE_IDLE; *outLen = 0U; return E_OK; }

static Std_ReturnType Result_0201(uint8_t *outBuf, uint16_t *outLen)
{ outBuf[0U] = s_0201->status; *outLen = 1U; return E_OK; }

DCM_RoutineEntry_t *DCM_Routine_Find(uint16_t routineId)
{
    uint16_t i;
    if (s_initialized == DEM_FALSE) { DCM_Routine_InitPointers(); }
    for (i = 0U; i < DCM_ROUTINE_TABLE_SIZE; i++)
    {
        if (DCM_RoutineTable[i].routineId == routineId)
        {
            return &DCM_RoutineTable[i];
        }
    }
    return NULL;
}

Std_ReturnType DCM_Routine_CheckAccess(const DCM_RoutineEntry_t *entry,
                                        uint8_t sessionType,
                                        uint8_t securityLevel)
{
    if (entry == NULL) { return E_NOT_OK; }
    if ((uint8_t)(entry->sessionMask & sessionType) == 0U) { return E_NOT_OK; }
    if (securityLevel < entry->securityLevel) { return E_NOT_OK; }
    return E_OK;
}

Std_ReturnType DCM_Routine_Execute(DCM_RoutineEntry_t *entry,
                                    uint8_t             controlType,
                                    const uint8_t      *inBuf,
                                    uint16_t            inLen,
                                    uint8_t            *outBuf,
                                    uint16_t           *outLen)
{
    if ((entry == NULL) || (outBuf == NULL) || (outLen == NULL))
    {
        return E_NOT_OK;
    }
    switch (controlType)
    {
        case DCM_ROUTINE_START:
            if (entry->startFn == NULL) { return E_NOT_OK; }
            return entry->startFn(inBuf, inLen, outBuf, outLen);
        case DCM_ROUTINE_STOP:
            if (entry->stopFn == NULL) { return E_NOT_OK; }
            return entry->stopFn(outBuf, outLen);
        case DCM_ROUTINE_RESULT:
            if (entry->resultFn == NULL) { return E_NOT_OK; }
            return entry->resultFn(outBuf, outLen);
        default:
            return E_NOT_OK;
    }
}
