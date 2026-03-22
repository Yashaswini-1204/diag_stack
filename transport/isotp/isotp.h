#ifndef ISOTP_H
#define ISOTP_H
#include <stdint.h>
#include "isotp_types.h"
#include "isotp_defines.h"
#ifdef __cplusplus
extern "C" {
#endif
int      isotp_user_send_can(const uint32_t arbitration_id,
                              const uint8_t *data,
                              const uint8_t  size);
uint32_t isotp_user_get_ms(void);
void     isotp_user_debug(const char *message, ...);
void isotp_init_link(IsoTpLink *link,
                     uint32_t   send_id,
                     uint32_t   recv_id,
                     uint32_t   receive_timeout);
void isotp_poll(IsoTpLink *link);
void isotp_on_can_message(IsoTpLink     *link,
                           const uint8_t *data,
                           uint8_t        len);
int  isotp_send(IsoTpLink     *link,
                const uint8_t *payload,
                uint16_t       size);
int  isotp_receive(IsoTpLink *link,
                   uint8_t   *payload,
                   uint16_t   buf_size,
                   uint16_t  *out_size);
#ifdef __cplusplus
}
#endif
#endif
