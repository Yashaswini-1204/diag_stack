#ifndef TRANSPORT_TCP_H
#define TRANSPORT_TCP_H

#include "iso14229.h"

#define UDS_TCP_PORT  (13400U)

UDSTp_t *TcpTp_Init(void);
void      TcpTp_Deinit(void);

#endif
