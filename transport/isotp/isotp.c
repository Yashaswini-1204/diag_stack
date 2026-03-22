#include "isotp.h"
#include <string.h>

static void isotp_reset_message(IsoTpMessage *msg)
{
    msg->state       = ISOTP_IDLE;
    msg->data_length = 0U;
    msg->data_offset = 0U;
    msg->sn          = 0U;
    msg->bs          = 0U;
    msg->st_min      = 0U;
}

static int isotp_send_flow_control(IsoTpLink *link, uint8_t fc_flag,
                                    uint8_t block_size, uint8_t st_min)
{
    uint8_t frame[ISOTP_CAN_FRAME_SIZE];
    memset(frame, 0xAAU, sizeof(frame));
    frame[0] = (uint8_t)((ISOTP_PCI_TYPE_FC << 4U) | (fc_flag & 0x0FU));
    frame[1] = block_size;
    frame[2] = st_min;
    return isotp_user_send_can(link->send_arbitration_id,
                                frame, ISOTP_CAN_FRAME_SIZE);
}

static int isotp_send_single_frame(IsoTpLink *link)
{
    uint8_t  frame[ISOTP_CAN_FRAME_SIZE];
    uint16_t len = link->send_message.data_length;
    if (len > 7U) { return ISOTP_RET_LENGTH; }
    memset(frame, 0xAAU, sizeof(frame));
    frame[0] = (uint8_t)((ISOTP_PCI_TYPE_SINGLE << 4U) | (len & 0x0FU));
    memcpy(&frame[1], link->send_message.data_buffer, (size_t)len);
    return isotp_user_send_can(link->send_arbitration_id,
                                frame, ISOTP_CAN_FRAME_SIZE);
}

static int isotp_send_first_frame(IsoTpLink *link)
{
    uint8_t  frame[ISOTP_CAN_FRAME_SIZE];
    uint16_t len = link->send_message.data_length;
    memset(frame, 0xAAU, sizeof(frame));
    frame[0] = (uint8_t)((ISOTP_PCI_TYPE_FIRST << 4U) |
                          ((len >> 8U) & 0x0FU));
    frame[1] = (uint8_t)(len & 0xFFU);
    memcpy(&frame[2], link->send_message.data_buffer, 6U);
    link->send_message.data_offset = 6U;
    link->send_message.sn          = 1U;
    link->send_message.state       = ISOTP_WAIT_FC;
    link->send_message.timer_bs    = isotp_user_get_ms();
    return isotp_user_send_can(link->send_arbitration_id,
                                frame, ISOTP_CAN_FRAME_SIZE);
}

static int isotp_send_consecutive_frame(IsoTpLink *link)
{
    uint8_t  frame[ISOTP_CAN_FRAME_SIZE];
    uint16_t remaining;
    uint8_t  copy_len;
    remaining = link->send_message.data_length
                - link->send_message.data_offset;
    copy_len  = (remaining > 7U) ? 7U : (uint8_t)remaining;
    memset(frame, 0xAAU, sizeof(frame));
    frame[0] = (uint8_t)((ISOTP_PCI_TYPE_CONSEC << 4U) |
                          (link->send_message.sn & 0x0FU));
    memcpy(&frame[1],
           link->send_message.data_buffer + link->send_message.data_offset,
           (size_t)copy_len);
    link->send_message.data_offset += copy_len;
    link->send_message.sn =
        (link->send_message.sn + 1U) & 0x0FU;
    if (link->send_message.data_offset >=
        link->send_message.data_length) {
        link->send_message.state = ISOTP_IDLE;
    }
    return isotp_user_send_can(link->send_arbitration_id,
                                frame, ISOTP_CAN_FRAME_SIZE);
}

void isotp_init_link(IsoTpLink *link,
                     uint32_t   send_id,
                     uint32_t   recv_id,
                     uint32_t   receive_timeout)
{
    if (link == NULL) { return; }
    memset(link, 0, sizeof(IsoTpLink));
    link->send_arbitration_id = send_id;
    link->recv_arbitration_id = recv_id;
    link->receive_timeout_ms  = (receive_timeout == 0U)
                                 ? ISOTP_DEFAULT_RESPONSE_TIMEOUT
                                 : receive_timeout;
    isotp_reset_message(&link->send_message);
    isotp_reset_message(&link->recv_message);
}

int isotp_send(IsoTpLink *link, const uint8_t *payload, uint16_t size)
{
    if ((link == NULL) || (payload == NULL) || (size == 0U)) {
        return ISOTP_RET_ERROR;
    }
    if (size > (uint16_t)sizeof(link->send_message.data_buffer)) {
        return ISOTP_RET_OVERFLOW;
    }
    if (link->send_message.state != ISOTP_IDLE) {
        return ISOTP_RET_INPROGRESS;
    }
    memcpy(link->send_message.data_buffer, payload, (size_t)size);
    link->send_message.data_length = size;
    link->send_message.data_offset = 0U;
    link->send_message.state       = ISOTP_SEND;
    if (size <= 7U) {
        link->send_message.state = ISOTP_IDLE;
        return isotp_send_single_frame(link);
    }
    return isotp_send_first_frame(link);
}

void isotp_on_can_message(IsoTpLink *link,
                           const uint8_t *data, uint8_t len)
{
    uint8_t  pci_type;
    uint16_t payload_len;
    if ((link == NULL) || (data == NULL) || (len == 0U)) { return; }
    pci_type = (data[0] >> 4U) & 0x0FU;
    switch (pci_type) {
    case ISOTP_PCI_TYPE_SINGLE:
        payload_len = (uint16_t)(data[0] & 0x0FU);
        if ((payload_len == 0U) || (payload_len > 7U)) { return; }
        memcpy(link->recv_message.data_buffer, &data[1],
               (size_t)payload_len);
        link->recv_message.data_length = payload_len;
        link->recv_message.data_offset = payload_len;
        link->recv_message.state       = ISOTP_IDLE;
        break;
    case ISOTP_PCI_TYPE_FIRST:
        payload_len = (uint16_t)(
            ((uint16_t)(data[0] & 0x0FU) << 8U) | (uint16_t)data[1]);
        if (payload_len >
            (uint16_t)sizeof(link->recv_message.data_buffer)) {
            (void)isotp_send_flow_control(link, ISOTP_FC_OVFLW, 0U, 0U);
            return;
        }
        isotp_reset_message(&link->recv_message);
        link->recv_message.data_length = payload_len;
        memcpy(link->recv_message.data_buffer, &data[2], 6U);
        link->recv_message.data_offset = 6U;
        link->recv_message.sn          = 1U;
        link->recv_message.state       = ISOTP_RECV_CF;
        link->recv_message.timer_bs    = isotp_user_get_ms();
        (void)isotp_send_flow_control(link, ISOTP_FC_CTS, 0U, 0U);
        break;
    case ISOTP_PCI_TYPE_CONSEC:
        if (link->recv_message.state != ISOTP_RECV_CF) { return; }
        if ((data[0] & 0x0FU) !=
            (link->recv_message.sn & 0x0FU)) {
            isotp_reset_message(&link->recv_message);
            return;
        }
        {
            uint16_t remaining = link->recv_message.data_length
                                 - link->recv_message.data_offset;
            uint8_t copy_len = (remaining > 7U) ? 7U : (uint8_t)remaining;
            memcpy(link->recv_message.data_buffer
                       + link->recv_message.data_offset,
                   &data[1], (size_t)copy_len);
            link->recv_message.data_offset += copy_len;
            link->recv_message.sn++;
            link->recv_message.timer_bs = isotp_user_get_ms();
            if (link->recv_message.data_offset >=
                link->recv_message.data_length) {
                link->recv_message.state = ISOTP_IDLE;
            }
        }
        break;
    case ISOTP_PCI_TYPE_FC:
        if (link->send_message.state != ISOTP_WAIT_FC) { return; }
        link->send_message.bs     = data[1];
        link->send_message.st_min = data[2];
        if ((data[0] & 0x0FU) == ISOTP_FC_CTS) {
            link->send_message.state    = ISOTP_SEND_CF;
            link->send_message.timer_st = isotp_user_get_ms();
        } else if ((data[0] & 0x0FU) == ISOTP_FC_OVFLW) {
            isotp_reset_message(&link->send_message);
        } else { /* WAIT — do nothing this cycle */ }
        break;
    default:
        break;
    }
}

void isotp_poll(IsoTpLink *link)
{
    uint32_t now;
    if (link == NULL) { return; }
    now = isotp_user_get_ms();
    if (link->send_message.state == ISOTP_SEND_CF) {
        if ((now - link->send_message.timer_st) >=
            (uint32_t)link->send_message.st_min) {
            (void)isotp_send_consecutive_frame(link);
            link->send_message.timer_st = now;
        }
    }
    if (link->send_message.state == ISOTP_WAIT_FC) {
        if ((now - link->send_message.timer_bs) >=
            link->receive_timeout_ms) {
            isotp_reset_message(&link->send_message);
        }
    }
    if (link->recv_message.state == ISOTP_RECV_CF) {
        if ((now - link->recv_message.timer_bs) >=
            link->receive_timeout_ms) {
            isotp_reset_message(&link->recv_message);
        }
    }
}

int isotp_receive(IsoTpLink *link, uint8_t *payload,
                  uint16_t buf_size, uint16_t *out_size)
{
    if ((link == NULL) || (payload == NULL) || (out_size == NULL)) {
        return ISOTP_RET_ERROR;
    }
    if ((link->recv_message.state == ISOTP_IDLE) &&
        (link->recv_message.data_offset > 0U) &&
        (link->recv_message.data_length > 0U)) {
        if (link->recv_message.data_length > buf_size) {
            return ISOTP_RET_OVERFLOW;
        }
        memcpy(payload, link->recv_message.data_buffer,
               (size_t)link->recv_message.data_length);
        *out_size = link->recv_message.data_length;
        isotp_reset_message(&link->recv_message);
        return ISOTP_RET_OK;
    }
    return ISOTP_RET_NO_DATA;
}
