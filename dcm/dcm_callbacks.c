#include "dcm_callbacks.h"
#include "../dem/dem_dtc.h"
#include "../dem/dem_nvm.h"
#include "../dem/dem_types.h"
#include "../dem/dem_cfg.h"
#include <string.h>

#define SECURITY_KEY_XOR  (0xDEADBEEFU)
#define SEED_LEN          (4U)
#define KEY_LEN           (4U)

static uint8_t s_seed[SEED_LEN] = {0x12U, 0x34U, 0x56U, 0x78U};

static UDSErr_t Handle_ClearDTC(UDSCDIArgs_t *args)
{
    Std_ReturnType result;
    uint32_t       groupOfDTC;

    if (args == NULL) { return UDS_ERR_INVALID_ARG; }

    groupOfDTC = args->groupOfDTC & 0x00FFFFFFU;

    if (groupOfDTC == 0x00FFFFFFU)
    {
        result = Dem_Dtc_Clear((uint32_t)DEM_DTC_CLEAR_ALL);
    }
    else
    {
        result = Dem_Dtc_Clear(groupOfDTC);
    }

    if (result == E_OK)
    {
        (void)Dem_Nvm_Save();
        return UDS_OK;
    }
    return (UDSErr_t)UDS_NRC_RequestOutOfRange;
}

static UDSErr_t Handle_ReadDTC(UDSServer_t *srv, UDSRDTCIArgs_t *args)
{
    uint8_t  idx;
    uint32_t dtcNumber  = 0U;
    uint8_t  statusByte = 0U;
    uint8_t  buf[4U];

    if ((srv == NULL) || (args == NULL)) { return UDS_ERR_INVALID_ARG; }

    switch (args->type)
    {
        case 0x02U:
            for (idx = 0U; idx < (uint8_t)DEM_MAX_DTC_ENTRIES; idx++)
            {
                if (Dem_Dtc_GetByIndex(idx, &dtcNumber, &statusByte) == E_OK)
                {
                    if ((statusByte &
                         args->subFuncArgs.dtcStatusByMaskArgs.mask) != 0U)
                    {
                        buf[0U] = (uint8_t)((dtcNumber >> 16U) & 0xFFU);
                        buf[1U] = (uint8_t)((dtcNumber >>  8U) & 0xFFU);
                        buf[2U] = (uint8_t)( dtcNumber         & 0xFFU);
                        buf[3U] = statusByte;
                        if (args->copy(srv, buf, 4U) != 4U)
                        {
                            return (UDSErr_t)UDS_NRC_ResponseTooLong;
                        }
                    }
                }
            }
            return UDS_OK;

        case 0x01U:
        {
            uint8_t count = 0U;
            for (idx = 0U; idx < (uint8_t)DEM_MAX_DTC_ENTRIES; idx++)
            {
                if (Dem_Dtc_GetByIndex(idx, &dtcNumber, &statusByte) == E_OK)
                {
                    if ((statusByte &
                         args->subFuncArgs.numOfDTCByStatusMaskArgs.mask) != 0U)
                    {
                        count++;
                    }
                }
            }
            buf[0U] = count;
            if (args->copy(srv, buf, 1U) != 1U)
            {
                return (UDSErr_t)UDS_NRC_ResponseTooLong;
            }
            return UDS_OK;
        }

        default:
            return (UDSErr_t)UDS_NRC_SubFunctionNotSupported;
    }
}

static UDSErr_t Handle_RDBI(UDSServer_t *srv, UDSRDBIArgs_t *args)
{
    uint8_t buf[8U];

    if ((srv == NULL) || (args == NULL)) { return UDS_ERR_INVALID_ARG; }

    switch (args->dataId)
    {
        case 0xF186U:
            buf[0U] = 0x01U;
            if (args->copy(srv, buf, 1U) != 1U)
            {
                return (UDSErr_t)UDS_NRC_ResponseTooLong;
            }
            return UDS_OK;

        case 0xF190U:
            (void)memset(buf, 0x00U, sizeof(buf));
            buf[0U] = 'T'; buf[1U] = 'E'; buf[2U] = 'S';
            buf[3U] = 'T'; buf[4U] = 'V'; buf[5U] = 'I';
            buf[6U] = 'N'; buf[7U] = '1';
            if (args->copy(srv, buf, 8U) != 8U)
            {
                return (UDSErr_t)UDS_NRC_ResponseTooLong;
            }
            return UDS_OK;

        default:
            return (UDSErr_t)UDS_NRC_RequestOutOfRange;
    }
}

static UDSErr_t Handle_SecAccessSeed(UDSServer_t *srv,
                                      UDSSecAccessRequestSeedArgs_t *args)
{
    if ((srv == NULL) || (args == NULL)) { return UDS_ERR_INVALID_ARG; }
    if (args->copySeed(srv, s_seed, (uint16_t)SEED_LEN) != SEED_LEN)
    {
        return (UDSErr_t)UDS_NRC_ResponseTooLong;
    }
    return UDS_OK;
}

static UDSErr_t Handle_SecAccessKey(UDSSecAccessValidateKeyArgs_t *args)
{
    uint32_t receivedKey;
    uint32_t expectedKey;

    if (args == NULL)                  { return UDS_ERR_INVALID_ARG; }
    if (args->len < (uint16_t)KEY_LEN) { return UDS_ERR_INVALID_ARG; }

    receivedKey = ((uint32_t)args->key[0U] << 24U)
                | ((uint32_t)args->key[1U] << 16U)
                | ((uint32_t)args->key[2U] <<  8U)
                | ((uint32_t)args->key[3U]);

    expectedKey = (((uint32_t)s_seed[0U] << 24U)
                 | ((uint32_t)s_seed[1U] << 16U)
                 | ((uint32_t)s_seed[2U] <<  8U)
                 | ((uint32_t)s_seed[3U]))
                ^ SECURITY_KEY_XOR;

    if (receivedKey == expectedKey) { return UDS_OK; }
    return (UDSErr_t)UDS_NRC_InvalidKey;
}

UDSErr_t DCM_ServerCallback(UDSServer_t *srv,
                             UDSEvent_t   event,
                             void        *arg)
{
    switch (event)
    {
        case UDS_EVT_ClearDiagnosticInfo:
            return Handle_ClearDTC((UDSCDIArgs_t *)arg);

        case UDS_EVT_ReadDTCInformation:
            return Handle_ReadDTC(srv, (UDSRDTCIArgs_t *)arg);

        case UDS_EVT_ReadDataByIdent:
            return Handle_RDBI(srv, (UDSRDBIArgs_t *)arg);

        case UDS_EVT_SecAccessRequestSeed:
            return Handle_SecAccessSeed(
                       srv, (UDSSecAccessRequestSeedArgs_t *)arg);

        case UDS_EVT_SecAccessValidateKey:
            return Handle_SecAccessKey(
                       (UDSSecAccessValidateKeyArgs_t *)arg);

        case UDS_EVT_SessionTimeout:
            (void)Dem_Nvm_Save();
            return UDS_OK;

        default:
            return UDS_OK;
    }
}
