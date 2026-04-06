#include "../inc/dcm_callbacks.h"
#include "../inc/dcm_did_table.h"
#include "../inc/dcm_routine_table.h"
#include "../dem/dem_dtc.h"
#include "../dem/dem_nvm.h"
#include "../dem/dem_types.h"
#include "../dem/dem_cfg.h"
#include <string.h>

#define SECURITY_KEY_XOR  (0xDEADBEEFU)
#define SEED_LEN          (4U)
#define KEY_LEN           (4U)

static uint8_t s_seed[SEED_LEN]         = {0x12U,0x34U,0x56U,0x78U};
static uint8_t s_currentSession         = 0x01U; /* DefaultSession */
static uint8_t s_currentSecurityLevel   = 0x00U; /* no security */

/* ── 0x10 DiagnosticSessionControl ─────────────────────────────── */
static UDSErr_t Handle_SessionControl(UDSDiagSessCtrlArgs_t *args)
{
    if (args == NULL) { return UDS_NRC_GeneralReject; }
    switch (args->type)
    {
        case 0x01U: s_currentSession = DCM_SESSION_DEFAULT;     break;
        case 0x02U: s_currentSession = DCM_SESSION_EXTENDED;    break;
        case 0x03U: s_currentSession = DCM_SESSION_PROGRAMMING; break;
        default:    return UDS_NRC_SubFunctionNotSupported;
    }
    /* Reset security on session change */
    s_currentSecurityLevel = 0x00U;
    return UDS_PositiveResponse;
}

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
    if (result == E_OK) { (void)Dem_Nvm_Save(); return UDS_PositiveResponse; }
    return UDS_NRC_RequestOutOfRange;
}

/* ── 0x19 ReadDTCInformation ────────────────────────────────────── */
static UDSErr_t Handle_ReadDTC(UDSServer_t *srv, UDSRDTCIArgs_t *args)
{
    uint8_t  idx;
    uint32_t dtcNumber  = 0U;
    uint8_t  statusByte = 0U;
    uint8_t  buf[DEM_FREEZE_FRAME_SIZE + 8U]; /* large enough for all subfuncs */
    uint8_t  mask;
    uint8_t  count      = 0U;

    if ((srv == NULL) || (args == NULL)) { return UDS_NRC_GeneralReject; }

    switch (args->type)
    {
        /* 0x01 reportNumberOfDTCByStatusMask */
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
            resp[0U] = mask;
            resp[1U] = 0x01U; /* UDS DTC format */
            resp[2U] = 0x00U;
            resp[3U] = count;
            if (args->copy(srv, resp, 4U) != UDS_PositiveResponse)
            {
                return UDS_NRC_ResponseTooLong;
            }
            return UDS_PositiveResponse;
        }

        /* 0x02 reportDTCByStatusMask */
        case 0x02U:
        {
            uint8_t availMask[1U];
            mask = args->subFuncArgs.dtcStatusByMaskArgs.mask;
            availMask[0U] = 0xFFU;
            if (args->copy(srv, availMask, 1U) != UDS_PositiveResponse)
            {
                return UDS_NRC_ResponseTooLong;
            }
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

        /* 0x03 reportDTCSnapshotIdentification
         * Response per DTC: 3 bytes DTC + 1 byte snapshotRecordNum
         * Length check: (send_len - base) % 4 == 0                */
        case 0x03U:
        {
            for (idx = 0U; idx < (uint8_t)DEM_MAX_DTC_ENTRIES; idx++)
            {
                if (Dem_Dtc_GetByIndex(idx, &dtcNumber, &statusByte) == E_OK)
                {
                    if ((statusByte & DEM_UDS_STATUS_CDTC) != 0U)
                    {
                        buf[0U] = (uint8_t)((dtcNumber >> 16U) & 0xFFU);
                        buf[1U] = (uint8_t)((dtcNumber >>  8U) & 0xFFU);
                        buf[2U] = (uint8_t)( dtcNumber         & 0xFFU);
                        buf[3U] = 0x01U; /* snapshotRecordNumber */
                        if (args->copy(srv, buf, 4U) != UDS_PositiveResponse)
                        {
                            return UDS_NRC_ResponseTooLong;
                        }
                    }
                }
            }
            return UDS_PositiveResponse;
        }

        /* 0x04 reportDTCSnapshotRecordByDTCNumber
         * Response: 3 DTC + 1 status + 1 recordNum + FF bytes      */
        case 0x04U:
        {
            uint32_t targetDtc;
            uint8_t  found     = 0U;
            uint8_t  ffBuf[DEM_FREEZE_FRAME_SIZE + 5U];

            targetDtc = args->subFuncArgs.dtcSnapshotRecordbyDTCNumArgs.dtc
                        & 0x00FFFFFFU;

            for (idx = 0U; idx < (uint8_t)DEM_MAX_DTC_ENTRIES; idx++)
            {
                if (Dem_Dtc_GetByIndex(idx, &dtcNumber, &statusByte) == E_OK)
                {
                    if ((dtcNumber & 0x00FFFFFFU) == targetDtc)
                    {
                        ffBuf[0U] = (uint8_t)((dtcNumber >> 16U) & 0xFFU);
                        ffBuf[1U] = (uint8_t)((dtcNumber >>  8U) & 0xFFU);
                        ffBuf[2U] = (uint8_t)( dtcNumber         & 0xFFU);
                        ffBuf[3U] = statusByte;
                        ffBuf[4U] = 0x01U; /* snapshotRecordNumber */
                        /* freeze frame placeholder — zeroed */
                        (void)memset(&ffBuf[5U], 0x00U,
                                     (size_t)DEM_FREEZE_FRAME_SIZE);
                        if (args->copy(srv, ffBuf,
                                       (uint16_t)(5U + DEM_FREEZE_FRAME_SIZE))
                            != UDS_PositiveResponse)
                        {
                            return UDS_NRC_ResponseTooLong;
                        }
                        found = 1U;
                        break;
                    }
                }
            }
            if (found == 0U) { return UDS_NRC_RequestOutOfRange; }
            return UDS_PositiveResponse;
        }

        /* 0x06 reportDTCExtDataRecordByDTCNumber
         * Response: 3 DTC + 1 status + 1 extDataRecNum + 4 bytes   */
        case 0x06U:
        {
            uint32_t targetDtc;
            uint8_t  found    = 0U;
            uint8_t  extBuf[9U]; /* 3+1+1+4 */

            targetDtc = args->subFuncArgs.dtcExtDtaRecordByDTCNumArgs.dtc
                        & 0x00FFFFFFU;

            for (idx = 0U; idx < (uint8_t)DEM_MAX_DTC_ENTRIES; idx++)
            {
                if (Dem_Dtc_GetByIndex(idx, &dtcNumber, &statusByte) == E_OK)
                {
                    if ((dtcNumber & 0x00FFFFFFU) == targetDtc)
                    {
                        extBuf[0U] = (uint8_t)((dtcNumber >> 16U) & 0xFFU);
                        extBuf[1U] = (uint8_t)((dtcNumber >>  8U) & 0xFFU);
                        extBuf[2U] = (uint8_t)( dtcNumber         & 0xFFU);
                        extBuf[3U] = statusByte;
                        extBuf[4U] = 0x01U; /* extDataRecordNumber */
                        /* occurrence counter = 1 (big-endian uint32) */
                        extBuf[5U] = 0x00U;
                        extBuf[6U] = 0x00U;
                        extBuf[7U] = 0x00U;
                        extBuf[8U] = 0x01U;
                        if (args->copy(srv, extBuf, 9U) != UDS_PositiveResponse)
                        {
                            return UDS_NRC_ResponseTooLong;
                        }
                        found = 1U;
                        break;
                    }
                }
            }
            if (found == 0U) { return UDS_NRC_RequestOutOfRange; }
            return UDS_PositiveResponse;
        }

        /* 0x09 / 0x0A reportSupportedDTC — all occupied DTCs */
        case 0x09U:
        case 0x0AU:
        {
            uint8_t availMask[1U];
            availMask[0U] = 0xFFU;
            if (args->copy(srv, availMask, 1U) != UDS_PositiveResponse)
            {
                return UDS_NRC_ResponseTooLong;
            }
            for (idx = 0U; idx < (uint8_t)DEM_MAX_DTC_ENTRIES; idx++)
            {
                if (Dem_Dtc_GetByIndex(idx, &dtcNumber, &statusByte) == E_OK)
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
            return UDS_PositiveResponse;
        }

        default:
            return UDS_NRC_SubFunctionNotSupported;
    }
}
static UDSErr_t Handle_RDBI(UDSServer_t *srv, UDSRDBIArgs_t *args)
{
    const DCM_DidEntry_t *entry;
    uint8_t               buf[64U];
    uint16_t              len = 0U;

    if ((srv == NULL) || (args == NULL)) { return UDS_NRC_GeneralReject; }

    entry = DCM_Did_Find(args->dataId);
    if (entry == NULL) { return UDS_NRC_RequestOutOfRange; }

    if (DCM_Did_CheckAccess(entry, s_currentSession,
                             s_currentSecurityLevel, 0U) != E_OK)
    {
        return UDS_NRC_SecurityAccessDenied;
    }

    if (entry->readFn(buf, &len) != E_OK)
    {
        return UDS_NRC_GeneralReject;
    }

    if (args->copy(srv, buf, len) != UDS_PositiveResponse)
    {
        return UDS_NRC_ResponseTooLong;
    }
    return UDS_PositiveResponse;
}

/* ── 0x2E WriteDataByIdentifier — uses DID table ────────────────── */
static UDSErr_t Handle_WDBI(UDSWDBIArgs_t *args)
{
    const DCM_DidEntry_t *entry;

    if (args == NULL) { return UDS_NRC_GeneralReject; }

    entry = DCM_Did_Find(args->dataId);
    if (entry == NULL) { return UDS_NRC_RequestOutOfRange; }

    if (DCM_Did_CheckAccess(entry, s_currentSession,
                             s_currentSecurityLevel, 1U) != E_OK)
    {
        return UDS_NRC_SecurityAccessDenied;
    }

    if (entry->writeFn(args->data, args->len) != E_OK)
    {
        return UDS_NRC_GeneralReject;
    }
    return UDS_PositiveResponse;
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

    if (received == expected)
    {
        s_currentSecurityLevel = DCM_SEC_LEVEL1;
        return UDS_PositiveResponse;
    }
    return (UDSErr_t)UDS_NRC_InvalidKey;
}
static UDSErr_t Handle_RoutineControl(UDSServer_t *srv, UDSRoutineCtrlArgs_t *args)
{
    DCM_RoutineEntry_t *entry;
    uint8_t             outBuf[32U];
    uint16_t            outLen = 0U;
    Std_ReturnType      result;

    if (args == NULL) { return UDS_NRC_GeneralReject; }

    entry = DCM_Routine_Find(args->id);
    if (entry == NULL) { return UDS_NRC_RequestOutOfRange; }

    if (DCM_Routine_CheckAccess(entry, s_currentSession,
                                 s_currentSecurityLevel) != E_OK)
    {
        return UDS_NRC_SecurityAccessDenied;
    }

    result = DCM_Routine_Execute(entry, args->ctrlType,
                                  args->optionRecord,
                                  args->len,
                                  outBuf, &outLen);
    if (result != E_OK) { return UDS_NRC_GeneralReject; }

    if ((outLen > 0U) && (args->copyStatusRecord != NULL))
    {
        if (args->copyStatusRecord(srv, outBuf, outLen) != UDS_PositiveResponse)
        {
            return UDS_NRC_ResponseTooLong;
        }
    }
    return UDS_PositiveResponse;
}


/* ── Master callback ────────────────────────────────────────────── */
UDSErr_t DCM_ServerCallback(UDSServer_t *srv,
                             UDSEvent_t   event,
                             void        *arg)
{
    switch (event)
    {
        case UDS_EVT_DiagSessCtrl:
            return Handle_SessionControl((UDSDiagSessCtrlArgs_t *)arg);
        case UDS_EVT_ClearDiagnosticInfo:
            return Handle_ClearDTC((UDSCDIArgs_t *)arg);
        case UDS_EVT_ReadDTCInformation:
            return Handle_ReadDTC(srv, (UDSRDTCIArgs_t *)arg);
        case UDS_EVT_ReadDataByIdent:
            return Handle_RDBI(srv, (UDSRDBIArgs_t *)arg);
        case UDS_EVT_RoutineCtrl:
            return Handle_RoutineControl(srv, (UDSRoutineCtrlArgs_t *)arg);
        case UDS_EVT_WriteDataByIdent:
            return Handle_WDBI((UDSWDBIArgs_t *)arg);
        case UDS_EVT_SecAccessRequestSeed:
            return Handle_SecAccessSeed(
                       srv, (UDSSecAccessRequestSeedArgs_t *)arg);
        case UDS_EVT_SecAccessValidateKey:
            return Handle_SecAccessKey(
                       (UDSSecAccessValidateKeyArgs_t *)arg);
        case UDS_EVT_SessionTimeout:
            (void)Dem_Nvm_Save();
            s_currentSession      = DCM_SESSION_DEFAULT;
            s_currentSecurityLevel = 0x00U;
            return UDS_PositiveResponse;
        default:
            return UDS_PositiveResponse;
    }
}
