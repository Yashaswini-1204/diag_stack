/* dcm_main.c — DCM init and poll entry point */
#include "dcm_main.h"
#include "dcm_callbacks.h"
#include "../transport/transport_tcp.h"
#include "../dem/dem_core.h"
#include "../dem/dem_dtc.h"
#include "../dem/dem_nvm.h"
#include "../dem/dem_debounce.h"
#include "../platform/platform_api.h"

static UDSServer_t s_server;
static UDSTp_t    *s_tp = NULL;

Std_ReturnType DCM_Init(void)
{
    s_tp = TcpTp_Init();
    if (s_tp == NULL) { return E_NOT_OK; }

    UDSServerInit(&s_server);
    s_server.tp         = s_tp;
    s_server.fn         = DCM_ServerCallback;
    s_server.p2_ms      = 50U;
    s_server.p2_star_ms = 2000U;
    s_server.s3_ms      = 5000U;
    s_server.p2_timer   = UDSMillis() + s_server.p2_ms;

    return E_OK;
}

void DCM_MainFunction(void)
{
    UDSServerPoll(&s_server);
}

void DCM_Deinit(void)
{
    TcpTp_Deinit();
}
