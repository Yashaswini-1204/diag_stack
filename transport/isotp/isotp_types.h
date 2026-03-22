#ifndef ISOTP_TYPES_H
#define ISOTP_TYPES_H
#include <stdint.h>
#include <string.h>
#include "isotp_defines.h"
typedef struct {
    uint32_t  id;
    uint8_t   data[ISOTP_CAN_FRAME_SIZE];
    uint8_t   len;
} IsoTpCanMessage;
typedef enum {
    ISOTP_IDLE    = 0,
    ISOTP_SEND    = 1,
    ISOTP_SEND_CF = 2,
    ISOTP_WAIT_FC = 3,
    ISOTP_RECV_FF = 4,
    ISOTP_RECV_CF = 5
} IsoTpState;
typedef struct {
    uint16_t    buf_size;
    uint16_t    data_offset;
    uint16_t    data_length;
    uint8_t     data_buffer[4096];
    uint8_t     sn;
    uint8_t     bs;
    uint8_t     st_min;
    uint32_t    timer_st;
    uint32_t    timer_bs;
    uint32_t    last_ms;
    IsoTpState  state;
} IsoTpMessage;
typedef struct {
    uint32_t        send_arbitration_id;
    uint32_t        recv_arbitration_id;
    IsoTpMessage    send_message;
    IsoTpMessage    recv_message;
    uint32_t        receive_timeout_ms;
} IsoTpLink;
#endif
