#ifndef DEM_EVENT_LOGGER_H
#define DEM_EVENT_LOGGER_H

#include <stdint.h>

/* ── 25 Event IDs by priority ───────────────────────────────── */
/* CRITICAL */
#define DEM_EVT_STN_SOS_GEN        (0x0101U)
#define DEM_EVT_STN_SOS_ACK        (0x0102U)
#define DEM_EVT_STN_SOS_CANCEL_ACK (0x0103U)
#define DEM_EVT_LOCO_SOS_RECV      (0x0104U)
#define DEM_EVT_LOCO_SOS_ACK       (0x0105U)
#define DEM_EVT_LOCO_SOS_CANCEL    (0x0106U)
/* HIGH */
#define DEM_EVT_RS485_UP           (0x0201U)
#define DEM_EVT_RS485_DOWN         (0x0202U)
#define DEM_EVT_ETH_UP             (0x0203U)
#define DEM_EVT_ETH_DOWN           (0x0204U)
#define DEM_EVT_HARDWIRE_FAIL      (0x0205U)
#define DEM_EVT_CRC_ERROR          (0x0206U)
#define DEM_EVT_HEALTH_FAIL        (0x0207U)
#define DEM_EVT_GPS1_FAULT         (0x0208U)
#define DEM_EVT_GPS2_FAULT         (0x0209U)
#define DEM_EVT_RADIO_FAULT        (0x020AU)
#define DEM_EVT_GSM_FAULT          (0x020BU)
/* MEDIUM */
#define DEM_EVT_POWER_ON           (0x0301U)
#define DEM_EVT_COMM_FALLBACK      (0x0302U)
#define DEM_EVT_HEALTH_OK          (0x0303U)
#define DEM_EVT_TSR_ACK            (0x0304U)
/* LOW */
#define DEM_EVT_POWER_OFF_RESET    (0x0401U)
#define DEM_EVT_MSG_STALE          (0x0402U)
#define DEM_EVT_BUZZER_TIMEOUT     (0x0403U)
#define DEM_EVT_CHECKSUM_DISPLAY   (0x0404U)

/* Priority */
#define DEM_PRIO_LOW    (0U)
#define DEM_PRIO_MED    (1U)
#define DEM_PRIO_HIGH   (2U)
#define DEM_PRIO_CRIT   (3U)

/* Source */
#define DEM_SRC_SYSTEM  (0U)
#define DEM_SRC_COMMS   (1U)
#define DEM_SRC_HW      (2U)
#define DEM_SRC_STCAS   (3U)
#define DEM_SRC_SMKEY   (4U)

/* Type */
#define DEM_TYPE_FAIL   (0U)
#define DEM_TYPE_PASS   (1U)

/* FORMAT A — full log record (all events) */
typedef struct {
    uint16_t slno;           /* serial number     */
    uint32_t timestamp;      /* Unix epoch        */
    uint32_t dtcNumber;      /* 3-byte DTC        */
    uint16_t eventId;        /* DEM_EVT_xxx       */
    char     eventName[24];  /* human readable    */
    uint8_t  priority;       /* 0=LOW..3=CRIT     */
    uint8_t  type;           /* 0=FAIL 1=PASS     */
    uint8_t  udsStatus;      /* 8-bit status byte */
    uint8_t  occurrence;     /* occurrence count  */
} DEM_EventRecordA_t;

/* FORMAT B — plug and play: query by eventId */
typedef struct {
    uint16_t eventId;
    uint16_t slno;
    uint32_t timestamp;
    uint32_t dtcNumber;
    uint8_t  priority;
    uint8_t  type;
    uint8_t  udsStatus;
    uint8_t  occurrence;
    char     eventName[24];
} DEM_EventRecordB_t;

/* ── API ─────────────────────────────────────────────────────── */
void     DEM_EventLogger_Init(void);
void     DEM_EventLogger_Write(uint16_t eventId, uint8_t priority,
                                uint8_t source,   uint8_t type,
                                uint32_t dtcNumber, uint8_t udsStatus);

/* Format A — dump all events */
uint16_t DEM_EventLogger_ReadAll(DEM_EventRecordA_t *out, uint16_t max);

/* Format B — query single event by ID (plug and play) */
uint8_t  DEM_EventLogger_QueryById(uint16_t eventId,
                                    DEM_EventRecordB_t *out);

/* Macros */
#define LOG_CRIT(id,dtc,sts) \
    DEM_EventLogger_Write(id,DEM_PRIO_CRIT,DEM_SRC_SYSTEM,DEM_TYPE_FAIL,dtc,sts)
#define LOG_HIGH(id,dtc,sts) \
    DEM_EventLogger_Write(id,DEM_PRIO_HIGH,DEM_SRC_SYSTEM,DEM_TYPE_FAIL,dtc,sts)
#define LOG_MED(id,dtc,sts) \
    DEM_EventLogger_Write(id,DEM_PRIO_MED,DEM_SRC_SYSTEM,DEM_TYPE_FAIL,dtc,sts)
#define LOG_LOW(id,dtc,sts) \
    DEM_EventLogger_Write(id,DEM_PRIO_LOW,DEM_SRC_SYSTEM,DEM_TYPE_FAIL,dtc,sts)
#define LOG_PASS(id,dtc,sts) \
    DEM_EventLogger_Write(id,DEM_PRIO_LOW,DEM_SRC_SYSTEM,DEM_TYPE_PASS,dtc,sts)

#endif
