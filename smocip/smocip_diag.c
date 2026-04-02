/* smocip_diag.c
 * FreeRTOS DIAG task — drives DEM + DCM every 10ms
 * Copy this file into your SM-OCIP STM32 project
 */
#include "smocip_diag.h"
#include "../dcm/dcm_main.h"
#include "../dem/dem_core.h"
#include "../dem/dem_dtc.h"
#include "../dem/dem_nvm.h"
#include "../dem/dem_debounce.h"
#include "../platform/platform_api.h"

#ifdef PLATFORM_FREERTOS
#include "cmsis_os2.h"
#define DIAG_DELAY_MS   (10U)
#define NVM_SAVE_MS     (60000U)   /* save to EEPROM every 60s */
#else
#include <unistd.h>
#define DIAG_DELAY_MS   (10U)
#define NVM_SAVE_MS     (60000U)
#endif

static uint32_t s_lastNvmSave = 0U;

/* Call once before starting scheduler */
Std_ReturnType SMOCIP_Diag_Init(void)
{
    Platform_Init();
    Dem_Init();
    Dem_Debounce_Init();
    Dem_Dtc_Init();
    Dem_Nvm_Load();
    return DCM_Init();
}

/* FreeRTOS task — priority 1 (lowest) */
void SMOCIP_Diag_Task(void *pvParameters)
{
    (void)pvParameters;
    uint32_t now;

    for (;;)
    {
        /* Drive DEM every 10ms */
        Dem_MainFunction();

        /* Drive DCM (UDS server poll) every 10ms */
        DCM_MainFunction();

        /* Kick watchdog */
        Platform_WdgTrigger();

        /* Periodic NvM save */
        now = Platform_GetTick_ms();
        if ((now - s_lastNvmSave) >= NVM_SAVE_MS)
        {
            Dem_Nvm_Save();
            s_lastNvmSave = now;
        }

#ifdef PLATFORM_FREERTOS
        osDelay(DIAG_DELAY_MS);
#else
        usleep(DIAG_DELAY_MS * 1000U);
#endif
    }
}
