#include "dcm_main.h"
#include "dcm_callbacks.h"
#include "../dem/dem_core.h"
#include "../dem/dem_dtc.h"
#include "../dem/dem_nvm.h"
#include "../dem/dem_debounce.h"
#include "../platform/platform_api.h"

static UDSServer_t s_server;

void DCM_Init(void)
{
    /* Init DEM layers */
    Dem_Init();
    Dem_Debounce_Init();
    Dem_Dtc_Init();
    Dem_Nvm_Init();

    /* Restore DTCs from NvM — ignore error on first boot */
    (void)Dem_Nvm_Load();

    /* Init UDS server */
    s_server.fn = DCM_ServerCallback;
    (void)UDSServerInit(&s_server);
}

void DCM_MainFunction(void)
{
    /* Poll UDS server — handles session timers, responses */
    UDSServerPoll(&s_server);

    /* Poll DEM */
    Dem_MainFunction();
    Dem_Debounce_MainFunction(Platform_GetTick_ms());

    /* Service watchdog */
    Platform_WdgTrigger();
}
