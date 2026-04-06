/*
 * dcm_did_table.c
 * SM-OCIP specific DID table
 * All DIDs relevant to KAVACH SM-OCIP — no generic automotive DIDs
 */
#include "../inc/dcm_did_table.h"
#include "../inc/dem_types.h"
#include <stddef.h>
#include <string.h>
#include <stdint.h>

static void WriteU32BE(uint8_t *d, uint32_t v)
{
    d[0]=(uint8_t)(v>>24U); d[1]=(uint8_t)(v>>16U);
    d[2]=(uint8_t)(v>>8U);  d[3]=(uint8_t)(v);
}

/* ── Mutable SMOCIP data ──────────────────────────────────── */
static uint32_t s_SOSCounter    = 0U;
static uint8_t  s_HealthStatus  = 0x01U; /* 0x01=OK 0x02=FAIL */
static uint8_t  s_CommStatus    = 0x01U; /* 0x01=OK 0x02=FAIL */

/* ── Read handlers ────────────────────────────────────────── */
static Std_ReturnType Read_F186_ActiveSession(uint8_t *b, uint16_t *l)
{ b[0]=0x01U; *l=1U; return E_OK; }

static Std_ReturnType Read_F188_SWVersion(uint8_t *b, uint16_t *l)
{
    static const uint8_t d[4]={0x01U,0x00U,0x00U,0x00U};
    memcpy(b,d,4U); *l=4U; return E_OK;
}

static Std_ReturnType Read_F190_VIN(uint8_t *b, uint16_t *l)
{
    /* Station ID used as VIN equivalent for SM-OCIP */
    static const uint8_t d[17]={'S','M','O','C','I','P',
        '0','0','5','3','1','0','0','0','0','0','1'};
    memcpy(b,d,17U); *l=17U; return E_OK;
}

static Std_ReturnType Read_F18C_SerialNum(uint8_t *b, uint16_t *l)
{
    static const uint8_t d[8]={'S','M','O','C','0','0','0','1'};
    memcpy(b,d,8U); *l=8U; return E_OK;
}

/* SMOCIP-specific DIDs */
static Std_ReturnType Read_0300_SOSCounter(uint8_t *b, uint16_t *l)
{ WriteU32BE(b,s_SOSCounter); *l=4U; return E_OK; }

static Std_ReturnType Write_0300_SOSCounter(const uint8_t *b, uint16_t l)
{
    if (l!=4U) { return E_NOT_OK; }
    s_SOSCounter=((uint32_t)b[0]<<24U)|((uint32_t)b[1]<<16U)
                |((uint32_t)b[2]<<8U)|(uint32_t)b[3];
    return E_OK;
}

static Std_ReturnType Read_0301_HealthStatus(uint8_t *b, uint16_t *l)
{ b[0]=s_HealthStatus; *l=1U; return E_OK; }

static Std_ReturnType Write_0301_HealthStatus(const uint8_t *b, uint16_t l)
{ if(l!=1U){return E_NOT_OK;} s_HealthStatus=b[0]; return E_OK; }

static Std_ReturnType Read_0302_CommStatus(uint8_t *b, uint16_t *l)
{ b[0]=s_CommStatus; *l=1U; return E_OK; }

static Std_ReturnType Read_F1A0_StationID(uint8_t *b, uint16_t *l)
{
    /* RDSO Station ID 00531 */
    b[0]=0x00U; b[1]=0x05U; b[2]=0x31U; *l=3U; return E_OK;
}

/* ── DID table ────────────────────────────────────────────── */
static const DCM_DidEntry_t DCM_DidTable[] = {
    /* Standard ISO DIDs */
    {0xF186U,DCM_SESSION_ALL,    DCM_SEC_NONE,  Read_F186_ActiveSession, NULL              },
    {0xF188U,DCM_SESSION_ALL,    DCM_SEC_NONE,  Read_F188_SWVersion,     NULL              },
    {0xF18CU,DCM_SESSION_ALL,    DCM_SEC_NONE,  Read_F18C_SerialNum,     NULL              },
    {0xF190U,DCM_SESSION_ALL,    DCM_SEC_NONE,  Read_F190_VIN,           NULL              },
    /* SM-OCIP specific DIDs */
    {0x0300U,DCM_SESSION_ALL,    DCM_SEC_NONE,  Read_0300_SOSCounter,    Write_0300_SOSCounter  },
    {0x0301U,DCM_SESSION_ALL,    DCM_SEC_NONE,  Read_0301_HealthStatus,  Write_0301_HealthStatus},
    {0x0302U,DCM_SESSION_ALL,    DCM_SEC_NONE,  Read_0302_CommStatus,    NULL              },
    {0xF1A0U,DCM_SESSION_ALL,    DCM_SEC_NONE,  Read_F1A0_StationID,     NULL              },
};

#define DCM_DID_TABLE_SIZE \
    ((uint16_t)(sizeof(DCM_DidTable)/sizeof(DCM_DidTable[0U])))

typedef struct { uint16_t did; uint8_t sessionMask; uint8_t securityLevel; }
DCM_DidWritePolicy_t;

static const DCM_DidWritePolicy_t DCM_WritePolicyTable[] = {
    {0x0300U, DCM_SESSION_EXTENDED, DCM_SEC_LEVEL1},
    {0x0301U, DCM_SESSION_EXTENDED, DCM_SEC_LEVEL1},
};
#define DCM_WRITE_POLICY_SIZE \
    ((uint16_t)(sizeof(DCM_WritePolicyTable)/sizeof(DCM_WritePolicyTable[0U])))

static const DCM_DidWritePolicy_t *FindWritePolicy(uint16_t did)
{
    uint16_t i;
    for(i=0U;i<DCM_WRITE_POLICY_SIZE;i++)
        if(DCM_WritePolicyTable[i].did==did) return &DCM_WritePolicyTable[i];
    return NULL;
}

const DCM_DidEntry_t *DCM_Did_Find(uint16_t did)
{
    uint16_t i;
    for(i=0U;i<DCM_DID_TABLE_SIZE;i++)
        if(DCM_DidTable[i].did==did) return &DCM_DidTable[i];
    return NULL;
}

Std_ReturnType DCM_Did_CheckAccess(const DCM_DidEntry_t *entry,
                                    uint8_t sessionType,
                                    uint8_t securityLevel,
                                    uint8_t isWrite)
{
    uint8_t reqSession,reqSecurity;
    const DCM_DidWritePolicy_t *wp;
    if(entry==NULL){return E_NOT_OK;}
    if(isWrite!=0U){
        wp=FindWritePolicy(entry->did);
        if(wp!=NULL){reqSession=wp->sessionMask;reqSecurity=wp->securityLevel;}
        else{reqSession=entry->sessionMask;reqSecurity=entry->securityLevel;}
        if(entry->writeFn==NULL){return E_NOT_OK;}
    } else {
        reqSession=entry->sessionMask;reqSecurity=entry->securityLevel;
        if(entry->readFn==NULL){return E_NOT_OK;}
    }
    if((uint8_t)(reqSession&sessionType)==0U){return E_NOT_OK;}
    if(securityLevel<reqSecurity){return E_NOT_OK;}
    return E_OK;
}

uint16_t DCM_Did_GetCount(void){ return DCM_DID_TABLE_SIZE; }
