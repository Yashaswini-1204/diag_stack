/*
 * dcm_routine_table.c
 * SM-OCIP specific routine control table
 * Routines: SelfDiag, ClearEventLog, TestBuzzer, ResetCommsStats
 */
#include "dcm_routine_table.h"
#include "dem_types.h"
#include "dcm_did_table.h"
#include <stddef.h>

static DCM_RoutineEntry_t DCM_RoutineTable[];
static DCM_RoutineEntry_t *s_0100=NULL,*s_0101=NULL,
                           *s_0102=NULL,*s_0103=NULL;
static Dem_BooleanType     s_initialized=DEM_FALSE;

static Std_ReturnType Start_SelfDiag(const uint8_t*,uint16_t,uint8_t*o,uint16_t*l)
{ o[0]=0x01U;*l=1U;return E_OK; }
static Std_ReturnType Stop_SelfDiag(uint8_t*o,uint16_t*l)
{(void)o;*l=0U;return E_OK;}
static Std_ReturnType Result_SelfDiag(uint8_t*o,uint16_t*l)
{ o[0]=s_0100->status;*l=1U;return E_OK; }

static Std_ReturnType Start_ClearLog(const uint8_t*,uint16_t,uint8_t*o,uint16_t*l)
{ s_0101->status=DCM_ROUTINE_COMPLETED;(void)o;*l=0U;return E_OK; }
static Std_ReturnType Stop_ClearLog(uint8_t*o,uint16_t*l)
{(void)o;s_0101->status=DCM_ROUTINE_IDLE;*l=0U;return E_OK;}
static Std_ReturnType Result_ClearLog(uint8_t*o,uint16_t*l)
{ o[0]=s_0101->status;*l=1U;return E_OK; }

static Std_ReturnType Start_TestBuzzer(const uint8_t*,uint16_t,uint8_t*o,uint16_t*l)
{ s_0102->status=DCM_ROUTINE_RUNNING;(void)o;*l=0U;return E_OK; }
static Std_ReturnType Stop_TestBuzzer(uint8_t*o,uint16_t*l)
{(void)o;s_0102->status=DCM_ROUTINE_IDLE;*l=0U;return E_OK;}
static Std_ReturnType Result_TestBuzzer(uint8_t*o,uint16_t*l)
{ o[0]=s_0102->status;*l=1U;return E_OK; }

static Std_ReturnType Start_ResetComms(const uint8_t*,uint16_t,uint8_t*o,uint16_t*l)
{ s_0103->status=DCM_ROUTINE_COMPLETED;(void)o;*l=0U;return E_OK; }
static Std_ReturnType Stop_ResetComms(uint8_t*o,uint16_t*l)
{(void)o;s_0103->status=DCM_ROUTINE_IDLE;*l=0U;return E_OK;}
static Std_ReturnType Result_ResetComms(uint8_t*o,uint16_t*l)
{ o[0]=s_0103->status;*l=1U;return E_OK; }

static DCM_RoutineEntry_t DCM_RoutineTable[]={
    {0x0100U,DCM_SESSION_EXTENDED,   DCM_SEC_NONE,  DCM_ROUTINE_IDLE,
     Start_SelfDiag,  Stop_SelfDiag,  Result_SelfDiag},
    {0x0101U,DCM_SESSION_EXTENDED,   DCM_SEC_LEVEL1,DCM_ROUTINE_IDLE,
     Start_ClearLog,  Stop_ClearLog,  Result_ClearLog},
    {0x0102U,DCM_SESSION_EXTENDED,   DCM_SEC_NONE,  DCM_ROUTINE_IDLE,
     Start_TestBuzzer,Stop_TestBuzzer,Result_TestBuzzer},
    {0x0103U,DCM_SESSION_PROGRAMMING,DCM_SEC_LEVEL1,DCM_ROUTINE_IDLE,
     Start_ResetComms,Stop_ResetComms,Result_ResetComms},
};
#define ROUTINE_TABLE_SIZE \
    ((uint16_t)(sizeof(DCM_RoutineTable)/sizeof(DCM_RoutineTable[0U])))

static void InitPointers(void)
{
    s_0100=&DCM_RoutineTable[0U]; s_0101=&DCM_RoutineTable[1U];
    s_0102=&DCM_RoutineTable[2U]; s_0103=&DCM_RoutineTable[3U];
    s_initialized=DEM_TRUE;
}

DCM_RoutineEntry_t *DCM_Routine_Find(uint16_t id)
{
    uint16_t i;
    if(s_initialized==DEM_FALSE){InitPointers();}
    for(i=0U;i<ROUTINE_TABLE_SIZE;i++)
        if(DCM_RoutineTable[i].routineId==id) return &DCM_RoutineTable[i];
    return NULL;
}

Std_ReturnType DCM_Routine_CheckAccess(const DCM_RoutineEntry_t *e,
                                        uint8_t sess, uint8_t sec)
{
    if(e==NULL){return E_NOT_OK;}
    if((uint8_t)(e->sessionMask&sess)==0U){return E_NOT_OK;}
    if(sec<e->securityLevel){return E_NOT_OK;}
    return E_OK;
}

Std_ReturnType DCM_Routine_Execute(DCM_RoutineEntry_t *e,
                                    uint8_t ctrl,
                                    const uint8_t *in, uint16_t inLen,
                                    uint8_t *out, uint16_t *outLen)
{
    if((e==NULL)||(out==NULL)||(outLen==NULL)){return E_NOT_OK;}
    switch(ctrl){
        case DCM_ROUTINE_START:
            if(e->startFn==NULL){return E_NOT_OK;}
            return e->startFn(in,inLen,out,outLen);
        case DCM_ROUTINE_STOP:
            if(e->stopFn==NULL){return E_NOT_OK;}
            return e->stopFn(out,outLen);
        case DCM_ROUTINE_RESULT:
            if(e->resultFn==NULL){return E_NOT_OK;}
            return e->resultFn(out,outLen);
        default: return E_NOT_OK;
    }
}
