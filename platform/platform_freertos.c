/* platform_freertos.c
 * Platform HAL for STM32H7ZG + FreeRTOS (CMSIS-RTOS2 wrapper)
 * Implements platform_api.h contract
 */
#include "platform_api.h"

#ifdef PLATFORM_FREERTOS

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os2.h"
#include "stm32h7xx_hal.h"

#define EEPROM_I2C_ADDR   (0xA0U)
#define EEPROM_PAGE_SIZE  (64U)
#define EEPROM_WRITE_MS   (10U)

extern I2C_HandleTypeDef  hi2c1;
extern IWDG_HandleTypeDef hiwdg;

static osMutexId_t s_demMutex = NULL;

void Platform_Init(void)
{
    s_demMutex = osMutexNew(NULL);
    configASSERT(s_demMutex != NULL);
}

void Platform_EnterCritical(void)
{
    if (osKernelGetState() == osKernelRunning)
        osMutexAcquire(s_demMutex, osWaitForever);
    else
        taskENTER_CRITICAL();
}

void Platform_ExitCritical(void)
{
    if (osKernelGetState() == osKernelRunning)
        osMutexRelease(s_demMutex);
    else
        taskEXIT_CRITICAL();
}

uint32_t Platform_GetTick_ms(void)
{
    return (uint32_t)(osKernelGetTickCount() * osKernelGetTickFreq() / 1000U);
}

Std_ReturnType Platform_NvmWrite(uint32_t addr, const uint8_t *buf, uint16_t len)
{
    uint16_t written = 0U;
    uint16_t chunk;
    while (written < len)
    {
        chunk = EEPROM_PAGE_SIZE
              - (uint16_t)((addr + written) % EEPROM_PAGE_SIZE);
        if (chunk > (len - written)) { chunk = len - written; }
        if (HAL_I2C_Mem_Write(&hi2c1,
                EEPROM_I2C_ADDR,
                (uint16_t)(addr + written),
                I2C_MEMADD_SIZE_16BIT,
                (uint8_t *)(buf + written),
                chunk,
                HAL_MAX_DELAY) != HAL_OK)
            return E_NOT_OK;
        osDelay(EEPROM_WRITE_MS);
        written += chunk;
    }
    return E_OK;
}

Std_ReturnType Platform_NvmRead(uint32_t addr, uint8_t *buf, uint16_t len)
{
    return (HAL_I2C_Mem_Read(&hi2c1,
                EEPROM_I2C_ADDR,
                (uint16_t)addr,
                I2C_MEMADD_SIZE_16BIT,
                buf, len,
                HAL_MAX_DELAY) == HAL_OK) ? E_OK : E_NOT_OK;
}

void Platform_WdgTrigger(void)
{
    HAL_IWDG_Refresh(&hiwdg);
}

#endif /* PLATFORM_FREERTOS */
