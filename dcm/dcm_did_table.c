#include "dcm_did_table.h"
#include "dem_types.h"
#include <stddef.h>
#include <string.h>
#include <stdint.h>

static void WriteU32LE(uint8_t *dst, uint32_t value)
{
    dst[0U] = (uint8_t)( value         & 0xFFU);
    dst[1U] = (uint8_t)((value >>  8U) & 0xFFU);
    dst[2U] = (uint8_t)((value >> 16U) & 0xFFU);
    dst[3U] = (uint8_t)((value >> 24U) & 0xFFU);
}

static uint32_t ReadU32LE(const uint8_t *src)
{
    return (uint32_t)src[0U]
         | ((uint32_t)src[1U] <<  8U)
         | ((uint32_t)src[2U] << 16U)
         | ((uint32_t)src[3U] << 24U);
}

static uint32_t s_WheelSpeed = 0U;
static uint32_t s_Odometer   = 0U;

static Std_ReturnType Read_F186_ActiveSession(uint8_t *buf, uint16_t *len)
{ buf[0U] = 0x01U; *len = 1U; return E_OK; }

static Std_ReturnType Read_F187_SparePartNumber(uint8_t *buf, uint16_t *len)
{
    static const uint8_t d[10U] = {'0','0','0','0','0','0','0','0','0','1'};
    (void)memcpy(buf, d, sizeof(d)); *len = (uint16_t)sizeof(d); return E_OK;
}

static Std_ReturnType Read_F188_SoftwareVersion(uint8_t *buf, uint16_t *len)
{
    static const uint8_t d[4U] = {0x01U,0x00U,0x00U,0x00U};
    (void)memcpy(buf, d, sizeof(d)); *len = (uint16_t)sizeof(d); return E_OK;
}

static Std_ReturnType Read_F189_HardwareVersion(uint8_t *buf, uint16_t *len)
{
    static const uint8_t d[4U] = {0x01U,0x00U,0x00U,0x00U};
    (void)memcpy(buf, d, sizeof(d)); *len = (uint16_t)sizeof(d); return E_OK;
}

static Std_ReturnType Read_F18A_SystemSupplierID(uint8_t *buf, uint16_t *len)
{
    static const uint8_t d[3U] = {0xAAU,0xBBU,0xCCU};
    (void)memcpy(buf, d, sizeof(d)); *len = (uint16_t)sizeof(d); return E_OK;
}

static Std_ReturnType Read_F18B_ManufacturingDate(uint8_t *buf, uint16_t *len)
{
    static const uint8_t d[3U] = {0x24U,0x03U,0x01U};
    (void)memcpy(buf, d, sizeof(d)); *len = (uint16_t)sizeof(d); return E_OK;
}

static Std_ReturnType Read_F18C_ECUSerialNumber(uint8_t *buf, uint16_t *len)
{
    static const uint8_t d[8U] = {'E','C','U','0','0','0','0','1'};
    (void)memcpy(buf, d, sizeof(d)); *len = (uint16_t)sizeof(d); return E_OK;
}

static Std_ReturnType Read_F190_VIN(uint8_t *buf, uint16_t *len)
{
    static const uint8_t d[17U] = {
        'T','E','S','T','V','I','N',
        '0','0','0','0','0','0','0','0','0','1'};
    (void)memcpy(buf, d, sizeof(d)); *len = (uint16_t)sizeof(d); return E_OK;
}

static Std_ReturnType Read_F191_ECUHwVersion(uint8_t *buf, uint16_t *len)
{
    static const uint8_t d[4U] = {0x01U,0x00U,0x00U,0x00U};
    (void)memcpy(buf, d, sizeof(d)); *len = (uint16_t)sizeof(d); return E_OK;
}

static Std_ReturnType Read_0100_WheelSpeed(uint8_t *buf, uint16_t *len)
{ WriteU32LE(buf, s_WheelSpeed); *len = 4U; return E_OK; }

static Std_ReturnType Write_0100_WheelSpeed(const uint8_t *buf, uint16_t len)
{ if (len != 4U) { return E_NOT_OK; } s_WheelSpeed = ReadU32LE(buf); return E_OK; }

static Std_ReturnType Read_0101_BrakePressure(uint8_t *buf, uint16_t *len)
{ WriteU32LE(buf, 0U); *len = 4U; return E_OK; }

static Std_ReturnType Read_0200_Odometer(uint8_t *buf, uint16_t *len)
{ WriteU32LE(buf, s_Odometer); *len = 4U; return E_OK; }

static Std_ReturnType Write_0200_Odometer(const uint8_t *buf, uint16_t len)
{ if (len != 4U) { return E_NOT_OK; } s_Odometer = ReadU32LE(buf); return E_OK; }

static Std_ReturnType Read_F1A0_AuditLog(uint8_t *buf, uint16_t *len)
{ (void)memset(buf, 0x00U, 8U); *len = 8U; return E_OK; }

static const DCM_DidEntry_t DCM_DidTable[] = {
    {0x0100U, DCM_SESSION_ALL,      DCM_SEC_NONE,   Read_0100_WheelSpeed,     Write_0100_WheelSpeed},
    {0x0101U, DCM_SESSION_ALL,      DCM_SEC_NONE,   Read_0101_BrakePressure,  NULL                 },
    {0x0200U, DCM_SESSION_ALL,      DCM_SEC_NONE,   Read_0200_Odometer,       Write_0200_Odometer  },
    {0xF186U, DCM_SESSION_ALL,      DCM_SEC_NONE,   Read_F186_ActiveSession,  NULL                 },
    {0xF187U, DCM_SESSION_ALL,      DCM_SEC_NONE,   Read_F187_SparePartNumber,NULL                 },
    {0xF188U, DCM_SESSION_ALL,      DCM_SEC_NONE,   Read_F188_SoftwareVersion,NULL                 },
    {0xF189U, DCM_SESSION_ALL,      DCM_SEC_NONE,   Read_F189_HardwareVersion,NULL                 },
    {0xF18AU, DCM_SESSION_ALL,      DCM_SEC_NONE,   Read_F18A_SystemSupplierID,NULL                },
    {0xF18BU, DCM_SESSION_ALL,      DCM_SEC_NONE,   Read_F18B_ManufacturingDate,NULL               },
    {0xF18CU, DCM_SESSION_ALL,      DCM_SEC_NONE,   Read_F18C_ECUSerialNumber,NULL                 },
    {0xF190U, DCM_SESSION_ALL,      DCM_SEC_NONE,   Read_F190_VIN,            NULL                 },
    {0xF191U, DCM_SESSION_ALL,      DCM_SEC_NONE,   Read_F191_ECUHwVersion,   NULL                 },
    {0xF1A0U, DCM_SESSION_EXTENDED, DCM_SEC_LEVEL1, Read_F1A0_AuditLog,       NULL                 },
};

#define DCM_DID_TABLE_SIZE \
    ((uint16_t)(sizeof(DCM_DidTable)/sizeof(DCM_DidTable[0U])))

typedef struct {
    uint16_t did;
    uint8_t  sessionMask;
    uint8_t  securityLevel;
} DCM_DidWritePolicy_t;

static const DCM_DidWritePolicy_t DCM_WritePolicyTable[] = {
    {0x0100U, DCM_SESSION_EXTENDED,    DCM_SEC_LEVEL1},
    {0x0200U, DCM_SESSION_PROGRAMMING, DCM_SEC_LEVEL1},
};

#define DCM_WRITE_POLICY_SIZE \
    ((uint16_t)(sizeof(DCM_WritePolicyTable)/sizeof(DCM_WritePolicyTable[0U])))

static const DCM_DidWritePolicy_t *FindWritePolicy(uint16_t did)
{
    uint16_t i;
    for (i = 0U; i < DCM_WRITE_POLICY_SIZE; i++)
    {
        if (DCM_WritePolicyTable[i].did == did)
        {
            return &DCM_WritePolicyTable[i];
        }
    }
    return NULL;
}

const DCM_DidEntry_t *DCM_Did_Find(uint16_t did)
{
    uint16_t i;
    for (i = 0U; i < DCM_DID_TABLE_SIZE; i++)
    {
        if (DCM_DidTable[i].did == did) { return &DCM_DidTable[i]; }
    }
    return NULL;
}

Std_ReturnType DCM_Did_CheckAccess(const DCM_DidEntry_t *entry,
                                    uint8_t sessionType,
                                    uint8_t securityLevel,
                                    uint8_t isWrite)
{
    uint8_t                      reqSession;
    uint8_t                      reqSecurity;
    const DCM_DidWritePolicy_t  *wp;

    if (entry == NULL) { return E_NOT_OK; }

    if (isWrite != 0U)
    {
        wp = FindWritePolicy(entry->did);
        if (wp != NULL)
        {
            reqSession  = wp->sessionMask;
            reqSecurity = wp->securityLevel;
        }
        else
        {
            reqSession  = entry->sessionMask;
            reqSecurity = entry->securityLevel;
        }
        if (entry->writeFn == NULL) { return E_NOT_OK; }
    }
    else
    {
        reqSession  = entry->sessionMask;
        reqSecurity = entry->securityLevel;
        if (entry->readFn == NULL) { return E_NOT_OK; }
    }

    if ((uint8_t)(reqSession & sessionType) == 0U) { return E_NOT_OK; }
    if (securityLevel < reqSecurity)               { return E_NOT_OK; }

    return E_OK;
}

uint16_t DCM_Did_GetCount(void)
{
    return DCM_DID_TABLE_SIZE;
}
