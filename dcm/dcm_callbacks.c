/* dcm_callbacks.c
 * Wires iso14229 UDS server events to DEM.
 * Return UDS_PositiveResponse (0) on success, UDS_NRC_* on failure.
 */
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

/* ── 0x14 ClearDiagnosticInformation ───────────────────────────── */
static UDSErr_t Handle_ClearDTC(UDSCDIArgs_t *args)
{
    Std_ReturnType result;
    uint32_t       groupOfDTC;

    if (args == NULL) { return UDS_NRC_GeneralReject; }

    groupOfDTC = args->groupOfDTC & 0x00FFFFFFU;

    result = (groupOfDTC == 0x00FFFFFFU)
             ? Dem_Dtc_Clear((uint32_t)DEM_DTC_CLEAR_ALL)
             : Dem_Dtc_Clear(groupOfDTC);

    if (result == E_OK)
    {
        (void)Dem_Nvm_Save();
        return UDS_PositiveResponse;
    }
    return UDS_NRC_RequestOutOfRange;
}

/* ── 0x19 ReadDTCInformation ────────────────────────────────────── */
static UDSErr_t Handle_ReadDTC(UDSServer_t *srv, UDSRDTCIArgs_t *args)
{
    uint8_t  idx;
    uint32_t dtcNumber  = 0U;
    uint8_t  statusByte = 0U;
    uint8_t  buf[4U];
    uint8_t  mask;
    uint8_t  count      = 0U;

    if ((srv == NULL) || (args == NULL)) { return UDS_NRC_GeneralReject; }

    switch (args->type)
    {
        /* 0x01 reportNumberOfDTCByStatusMask
         * Response: base(2) + availMask(1) + dtcCountHighByte(1)
         *           + dtcCountLowByte(1) + dtcFormatId(1) = base+4
         * iso14229 checks: send_len == base + 4                    */
        case 0x01U:
        {
            uint8_t resp[4U];
            mask = args->subFuncArgs.numOfDTCByStatusMaskArgs.mask;
            for (idx = 0U; idx < (uint8_t)DEM_MAX_DTC_ENTRIES; idx++)
            {
                if (Dem_Dtc_GetByIndex(idx, &dtcNumber, &statusByte) == E_OK)
                {
                    if ((statusByte & mask) != 0U) { count++; }
                }
            }
            /* availableMask, formatId(UDS=0x01), countHigh, countLow */
            resp[0U] = mask;
            resp[1U] = 0x01U; /* DTCFormatIdentifier: ISO15031-6DTCFormat */
            resp[2U] = 0x00U; /* count high byte */
            resp[3U] = count; /* count low byte  */
            if (args->copy(srv, resp, 4U) != UDS_PositiveResponse)
            {
                return UDS_NRC_ResponseTooLong;
            }
            return UDS_PositiveResponse;
        }

        /* 0x02 reportDTCByStatusMask
         * Response: base(2) + availMask(1) [+ (dtc3 + status1)*N]
         * iso14229 checks: send_len >= base+1
         *   AND (send_len - (base+1)) % 4 == 0                     */
        case 0x02U:
        {
            uint8_t availMask[1U];
            mask = args->subFuncArgs.dtcStatusByMaskArgs.mask;

            /* Always write the availableDTCStatusMask byte first */
            availMask[0U] = 0xFFU; /* all status bits supported */
            if (args->copy(srv, availMask, 1U) != UDS_PositiveResponse)
            {
                return UDS_NRC_ResponseTooLong;
            }

            /* Then write each matching DTC (3 bytes) + status (1 byte) */
            for (idx = 0U; idx < (uint8_t)DEM_MAX_DTC_ENTRIES; idx++)
            {
                if (Dem_Dtc_GetByIndex(idx, &dtcNumber, &statusByte) == E_OK)
                {
                    if ((statusByte & mask) != 0U)
                    {
                        buf[0U] = (uint8_t)((dtcNumber >> 16U) & 0xFFU);
                        buf[1U] = (uint8_t)((dtcNumber >>  8U) & 0xFFU);
                        buf[2U] = (uint8_t)( dtcNumber         & 0xFFU);
                        buf[3U] = statusByte;
                        if (args->copy(srv, buf, 4U) != UDS_PositiveResponse)
                        {
                            return UDS_NRC_ResponseTooLong;
                        }
                    }
                }
            }
            return UDS_PositiveResponse;
        }

        default:
            return UDS_NRC_SubFunctionNotSupported;
    }
}

/* ── 0x22 ReadDataByIdentifier ──────────────────────────────────── */
static UDSErr_t Handle_RDBI(UDSServer_t *srv, UDSRDBIArgs_t *args)
{
    /* Static DID table */
    static const uint8_t vin[8U]  = {'T','E','S','T','V','I','N','1'};
    static const uint8_t sess[1U] = {0x01U}; /* DefaultSession */

    if ((srv == NULL) || (args == NULL)) { return UDS_NRC_GeneralReject; }

    switch (args->dataId)
    {
        case 0xF186U: /* ActiveDiagnosticSession */
            if (args->copy(srv, sess, 1U) != UDS_PositiveResponse)
            {
                return UDS_NRC_ResponseTooLong;
            }
            return UDS_PositiveResponse;

        case 0xF190U: /* VIN */
            if (args->copy(srv, vin, 8U) != UDS_PositiveResponse)
            {
                return UDS_NRC_ResponseTooLong;
            }
            return UDS_PositiveResponse;

        default:
            return UDS_NRC_RequestOutOfRange;
    }
}

/* ── 0x27 SecurityAccess — RequestSeed ─────────────────────────── */
static UDSErr_t Handle_SecAccessSeed(UDSServer_t *srv,
                                      UDSSecAccessRequestSeedArgs_t *args)
{
    if ((srv == NULL) || (args == NULL)) { return UDS_NRC_GeneralReject; }
    if (args->copySeed(srv, s_seed, (uint16_t)SEED_LEN) != UDS_PositiveResponse)
    {
        return UDS_NRC_ResponseTooLong;
    }
    return UDS_PositiveResponse;
}

/* ── 0x27 SecurityAccess — ValidateKey ─────────────────────────── */
static UDSErr_t Handle_SecAccessKey(UDSSecAccessValidateKeyArgs_t *args)
{
    uint32_t received;
    uint32_t expected;

    if (args == NULL)                  { return UDS_NRC_GeneralReject; }
    if (args->len < (uint16_t)KEY_LEN) { return UDS_NRC_GeneralReject; }

    received = ((uint32_t)args->key[0U] << 24U)
             | ((uint32_t)args->key[1U] << 16U)
             | ((uint32_t)args->key[2U] <<  8U)
             | ((uint32_t)args->key[3U]);

    expected = (((uint32_t)s_seed[0U] << 24U)
              | ((uint32_t)s_seed[1U] << 16U)
              | ((uint32_t)s_seed[2U] <<  8U)
              | ((uint32_t)s_seed[3U]))
             ^ SECURITY_KEY_XOR;

    return (received == expected)
           ? UDS_PositiveResponse
           : (UDSErr_t)UDS_NRC_InvalidKey;
}

/* ── Master callback ────────────────────────────────────────────── */
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
            return UDS_PositiveResponse;

        /* Unhandled events — return positive so server sends a response */
        default:
            return UDS_PositiveResponse;
    }
}
