#define _XOPEN_SOURCE 600
#include "../inc/platform_api.h"
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stddef.h>

#define NVM_PATH_MAX_LEN (64U)
#define NVM_DIR_PREFIX   "/tmp/dem_nvm_"
#define NVM_FILE_SUFFIX  ".bin"

static pthread_mutex_t s_criticalMutex;
static int             s_mutexReady = 0;

static void Mutex_InitOnce(void)
{
    static pthread_mutex_t s_initGuard = PTHREAD_MUTEX_INITIALIZER;
    (void)pthread_mutex_lock(&s_initGuard);
    if (s_mutexReady == 0)
    {
        pthread_mutexattr_t attr;
        (void)pthread_mutexattr_init(&attr);
        (void)pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        (void)pthread_mutex_init(&s_criticalMutex, &attr);
        (void)pthread_mutexattr_destroy(&attr);
        s_mutexReady = 1;
    }
    (void)pthread_mutex_unlock(&s_initGuard);
}

static void NvmBuildPath(char *buf, size_t bufLen, uint16_t blockId)
{
    (void)snprintf(buf, bufLen,
                   NVM_DIR_PREFIX "%u" NVM_FILE_SUFFIX,
                   (unsigned int)blockId);
}

void Platform_EnterCritical(void)
{
    if (s_mutexReady == 0) { Mutex_InitOnce(); }
    (void)pthread_mutex_lock(&s_criticalMutex);
}

void Platform_ExitCritical(void)
{
    (void)pthread_mutex_unlock(&s_criticalMutex);
}

uint32_t Platform_GetTick_ms(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) { return 0U; }
    return (uint32_t)((uint64_t)ts.tv_sec * 1000ULL +
                      (uint64_t)ts.tv_nsec / 1000000ULL);
}

uint8_t Platform_NvmWrite(uint16_t blockId, const void *data, uint16_t len)
{
    char   path[NVM_PATH_MAX_LEN];
    FILE  *fp;
    size_t written;
    NvmBuildPath(path, sizeof(path), blockId);
    fp = fopen(path, "wb");
    if (fp == NULL) { return PLATFORM_NOT_OK; }
    written = fwrite(data, 1U, (size_t)len, fp);
    (void)fclose(fp);
    return (written == (size_t)len) ? PLATFORM_OK : PLATFORM_NOT_OK;
}

uint8_t Platform_NvmRead(uint16_t blockId, void *data, uint16_t len)
{
    char   path[NVM_PATH_MAX_LEN];
    FILE  *fp;
    size_t bytesRead;
    NvmBuildPath(path, sizeof(path), blockId);
    fp = fopen(path, "rb");
    if (fp == NULL) { return PLATFORM_NOT_OK; }
    bytesRead = fread(data, 1U, (size_t)len, fp);
    (void)fclose(fp);
    return (bytesRead == (size_t)len) ? PLATFORM_OK : PLATFORM_NOT_OK;
}

void Platform_WdgTrigger(void)
{
    /* No-op on Linux */
}
