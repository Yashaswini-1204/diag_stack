#include "unity.h"
#include "iso14229.h"
#include "../dcm/dcm_callbacks.h"
#include "../dem/dem_core.h"
#include "../dem/dem_dtc.h"
#include "../dem/dem_nvm.h"
#include "../dem/dem_debounce.h"
#include "../platform/platform_api.h"
#include <string.h>
#include <stdio.h>
#define _POSIX_C_SOURCE 200809L
#include <unistd.h>   /* usleep */

#define SERVER_ADDR  (0x7E8U)
#define CLIENT_ADDR  (0x7E0U)
#define FUNC_ADDR    (0x7DFU)
/* Poll for up to 2 seconds — 1ms sleep per iteration */
#define POLL_LIMIT   (2000U)

static UDSServer_t  s_server;
static UDSTp_t     *s_serverTp;
static UDSTp_t     *s_clientTp;
static UDSClient_t  s_client;

static int s_responseReceived;
static int s_idleReceived;

static int ClientCb(UDSClient_t *client, UDSEvent_t evt, void *ev_data)
{
    (void)client;
    (void)ev_data;
    if (evt == UDS_EVT_ResponseReceived) { s_responseReceived = 1; }
    if (evt == UDS_EVT_Idle)             { s_idleReceived     = 1; }
    return 0;
}

static void ResetTracking(void)
{
    s_responseReceived = 0;
    s_idleReceived     = 0;
}

/* Poll with real 1ms sleep so p2 timers actually expire */
static void PollUntilResponse(void)
{
    uint32_t i;
    for (i = 0U; i < POLL_LIMIT; i++)
    {
        UDSServerPoll(&s_server);
        UDSClientPoll(&s_client);
        if (s_responseReceived) { break; }
        usleep(1000U); /* 1ms — allows UDSMillis() to advance */
    }
}

static void SetupTransports(void)
{
    ISOTPMockArgs_t srvArgs;
    memset(&srvArgs, 0, sizeof(srvArgs));
    srvArgs.sa_phys = SERVER_ADDR;
    srvArgs.ta_phys = CLIENT_ADDR;
    srvArgs.sa_func = SERVER_ADDR;
    srvArgs.ta_func = FUNC_ADDR;
    s_serverTp = ISOTPMockNew("server", &srvArgs);

    ISOTPMockArgs_t cliArgs;
    memset(&cliArgs, 0, sizeof(cliArgs));
    cliArgs.sa_phys = CLIENT_ADDR;
    cliArgs.ta_phys = SERVER_ADDR;
    cliArgs.sa_func = CLIENT_ADDR;
    cliArgs.ta_func = FUNC_ADDR;
    s_clientTp = ISOTPMockNew("client", &cliArgs);
}

void setUp(void)
{
    ISOTPMockReset();
    Dem_Init();
    Dem_Debounce_Init();
    Dem_Dtc_Init();
    Dem_Nvm_Init();
    ResetTracking();

    SetupTransports();

    /* Init — both do memset internally */
    UDSServerInit(&s_server);
    UDSClientInit(&s_client);

    /* Set ALL fields AFTER init */
    s_server.tp         = s_serverTp;
    s_server.fn         = DCM_ServerCallback;
    s_server.p2_ms      = 10U;
    s_server.p2_star_ms = 500U;
    s_server.s3_ms      = 5000U;
    /* Reset p2_timer after setting p2_ms */
    s_server.p2_timer   = UDSMillis() + s_server.p2_ms;

    s_client.tp         = s_clientTp;
    s_client.p2_ms      = 1000U;
    s_client.p2_star_ms = 5000U;
    s_client.fn         = ClientCb;
}

void tearDown(void)
{
    ISOTPMockFree(s_serverTp);
    ISOTPMockFree(s_clientTp);
    ISOTPMockReset();
}

void test_UDS_0x10_SessionControl(void)
{
    ResetTracking();
    TEST_ASSERT_EQUAL(UDS_OK, UDSSendDiagSessCtrl(&s_client, 0x01U));
    PollUntilResponse();
    TEST_ASSERT_EQUAL_INT(1, s_responseReceived);
}

void test_UDS_0x14_ClearDTC_Empty(void)
{
    uint8_t req[4] = {0x14U, 0xFFU, 0xFFU, 0xFFU};
    ResetTracking();
    TEST_ASSERT_EQUAL(UDS_OK, UDSSendBytes(&s_client, req, 4U));
    PollUntilResponse();
    TEST_ASSERT_EQUAL_INT(1, s_responseReceived);
}

void test_UDS_0x14_ClearDTC_WithEntry(void)
{
    uint8_t req[4] = {0x14U, 0xFFU, 0xFFU, 0xFFU};
    TEST_ASSERT_EQUAL(E_OK,
        Dem_Dtc_Store(1U, 0xAB1234U, 0x08U, 1U, NULL, 0U));
    TEST_ASSERT_EQUAL_UINT8(1U, Dem_Dtc_GetCount());
    ResetTracking();
    TEST_ASSERT_EQUAL(UDS_OK, UDSSendBytes(&s_client, req, 4U));
    PollUntilResponse();
    TEST_ASSERT_EQUAL_INT(1, s_responseReceived);
    TEST_ASSERT_EQUAL_UINT8(0U, Dem_Dtc_GetCount());
}

void test_UDS_0x19_ReadDTC_WithEntry(void)
{
    uint8_t req[3] = {0x19U, 0x02U, 0xFFU};
    TEST_ASSERT_EQUAL(E_OK,
        Dem_Dtc_Store(1U, 0xAB1234U, 0x08U, 1U, NULL, 0U));
    ResetTracking();
    TEST_ASSERT_EQUAL(UDS_OK, UDSSendBytes(&s_client, req, 3U));
    PollUntilResponse();
    TEST_ASSERT_EQUAL_INT(1, s_responseReceived);
    TEST_ASSERT_GREATER_THAN(0U, s_client.recv_size);
}

void test_UDS_0x22_RDBI_VIN(void)
{
    uint16_t didList[1] = {0xF190U};
    ResetTracking();
    TEST_ASSERT_EQUAL(UDS_OK, UDSSendRDBI(&s_client, didList, 1U));
    PollUntilResponse();
    TEST_ASSERT_EQUAL_INT(1, s_responseReceived);
    TEST_ASSERT_GREATER_THAN(0U, s_client.recv_size);
}

void test_UDS_0x19_ReadDTC_Empty(void)
{
    uint8_t req[3] = {0x19U, 0x02U, 0xFFU};
    ResetTracking();
    TEST_ASSERT_EQUAL(UDS_OK, UDSSendBytes(&s_client, req, 3U));
    PollUntilResponse();
    TEST_ASSERT_EQUAL_INT(1, s_responseReceived);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_UDS_0x10_SessionControl);
    RUN_TEST(test_UDS_0x14_ClearDTC_Empty);
    RUN_TEST(test_UDS_0x14_ClearDTC_WithEntry);
    RUN_TEST(test_UDS_0x19_ReadDTC_WithEntry);
    RUN_TEST(test_UDS_0x22_RDBI_VIN);
    RUN_TEST(test_UDS_0x19_ReadDTC_Empty);
    return UNITY_END();
}
