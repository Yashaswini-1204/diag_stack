#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include "dem_event_logger.h"
#include "dem_types.h"

int main(void)
{
    /* ── Init ─────────────────────────────────────────────── */
    DEM_EventLogger_Init();

    /* ── Simulate 10 events ───────────────────────────────── */
    DEM_EventLogger_Write(DEM_EVT_POWER_ON,        DEM_PRIO_MED,  DEM_SRC_SYSTEM, DEM_TYPE_PASS, 0x000000U, 0x50U);
    DEM_EventLogger_Write(DEM_EVT_RS485_UP,         DEM_PRIO_HIGH, DEM_SRC_COMMS,  DEM_TYPE_PASS, 0x000000U, 0x50U);
    DEM_EventLogger_Write(DEM_EVT_STN_SOS_GEN,      DEM_PRIO_CRIT, DEM_SRC_SMKEY,  DEM_TYPE_FAIL, 0xAB1234U, 0x08U);
    DEM_EventLogger_Write(DEM_EVT_HEALTH_FAIL,      DEM_PRIO_HIGH, DEM_SRC_SYSTEM, DEM_TYPE_FAIL, 0xCC5678U, 0x08U);
    DEM_EventLogger_Write(DEM_EVT_LOCO_SOS_RECV,    DEM_PRIO_CRIT, DEM_SRC_STCAS,  DEM_TYPE_FAIL, 0xDD9999U, 0x08U);
    DEM_EventLogger_Write(DEM_EVT_ETH_DOWN,         DEM_PRIO_HIGH, DEM_SRC_COMMS,  DEM_TYPE_FAIL, 0x000000U, 0x08U);
    DEM_EventLogger_Write(DEM_EVT_LOCO_SOS_ACK,     DEM_PRIO_CRIT, DEM_SRC_SMKEY,  DEM_TYPE_PASS, 0xDD9999U, 0x00U);
    DEM_EventLogger_Write(DEM_EVT_GPS1_FAULT,       DEM_PRIO_HIGH, DEM_SRC_STCAS,  DEM_TYPE_FAIL, 0xEE1111U, 0x08U);
    DEM_EventLogger_Write(DEM_EVT_CRC_ERROR,        DEM_PRIO_HIGH, DEM_SRC_COMMS,  DEM_TYPE_FAIL, 0x000000U, 0x08U);
    DEM_EventLogger_Write(DEM_EVT_HEALTH_OK,        DEM_PRIO_MED,  DEM_SRC_SYSTEM, DEM_TYPE_PASS, 0x000000U, 0x50U);
    /* Log same event twice to test occurrence count */
    DEM_EventLogger_Write(DEM_EVT_STN_SOS_GEN,      DEM_PRIO_CRIT, DEM_SRC_SMKEY,  DEM_TYPE_FAIL, 0xAB1234U, 0x08U);

    /* ══════════════════════════════════════════════════════
     * FORMAT A — Full event log table
     * ══════════════════════════════════════════════════════ */
    DEM_EventRecordA_t table[50];
    uint16_t count = DEM_EventLogger_ReadAll(table, 50U);

    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                         SM-OCIP DEM EVENT LOG — FORMAT A (ALL EVENTS)                  ║\n");
    printf("╠══════╦═════════════╦══════════╦═════════════════════╦══════════╦════════╦═══════╦═══════╣\n");
    printf("║ SlNo ║  Timestamp  ║   DTC    ║     Event Name      ║ Priority ║  Type  ║  UDS  ║ Count ║\n");
    printf("╠══════╬═════════════╬══════════╬═════════════════════╬══════════╬════════╬═══════╬═══════╣\n");

    const char *prioStr[] = {"LOW ","MED ","HIGH","CRIT"};
    uint16_t i;
    for (i = 0U; i < count; i++)
    {
        printf("║ %4u ║ %11lu ║ %08lX ║ %-19s ║   %-4s   ║ %-6s ║ 0x%02X  ║  %3u  ║\n",
            table[i].slno,
            (unsigned long)table[i].timestamp,
            (unsigned long)table[i].dtcNumber,
            table[i].eventName,
            prioStr[table[i].priority < 4U ? table[i].priority : 0U],
            table[i].type == DEM_TYPE_FAIL ? "FAIL" : "PASS",
            table[i].udsStatus,
            table[i].occurrence);
    }
    printf("╚══════╩═════════════╩══════════╩═════════════════════╩══════════╩════════╩═══════╩═══════╝\n");
    printf("  Total events logged: %u\n\n", count);

    /* ══════════════════════════════════════════════════════
     * FORMAT B — Query by Event ID (plug and play)
     * ══════════════════════════════════════════════════════ */
    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║       FORMAT B — Query by Event ID (Plug & Play)    ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n\n");

    uint16_t query_ids[] = {
        DEM_EVT_STN_SOS_GEN,
        DEM_EVT_HEALTH_FAIL,
        DEM_EVT_LOCO_SOS_RECV,
        DEM_EVT_GPS1_FAULT,
        DEM_EVT_POWER_ON,
        0x9999U   /* unknown ID */
    };
    uint8_t q;
    for (q = 0U; q < 6U; q++)
    {
        DEM_EventRecordB_t rec;
        memset(&rec, 0, sizeof(rec));
        printf("  Query EventID = 0x%04X  -->  ", query_ids[q]);
        if (DEM_EventLogger_QueryById(query_ids[q], &rec))
        {
            printf("FOUND\n");
            printf("    Name      : %s\n",  rec.eventName);
            printf("    SlNo      : %u\n",  rec.slno);
            printf("    Timestamp : %lu\n", (unsigned long)rec.timestamp);
            printf("    DTC       : 0x%06lX\n", (unsigned long)rec.dtcNumber);
            printf("    Priority  : %s\n",  prioStr[rec.priority < 4U ? rec.priority : 0U]);
            printf("    Type      : %s\n",  rec.type == DEM_TYPE_FAIL ? "FAIL" : "PASS");
            printf("    UDS Status: 0x%02X\n", rec.udsStatus);
            printf("    Occurrence: %u\n\n", rec.occurrence);
        }
        else
        {
            printf("NOT FOUND\n\n");
        }
    }
    return 0;
}
