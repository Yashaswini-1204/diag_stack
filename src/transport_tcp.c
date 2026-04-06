/* transport_tcp.c
 * UDS transport over TCP port 13400 (ISO 13400 DoIP style)
 * Replaces ISOTPMock for real Ethernet communication
 * Works on Linux (PC test) and STM32H7 with LwIP
 */
#include "../inc/transport_tcp.h"
#include <string.h>

#ifdef PLATFORM_LINUX
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#define SOCK_CLOSE(s)   close(s)
#define SOCK_INVALID    (-1)
typedef int             sock_t;
#endif

#ifdef PLATFORM_FREERTOS
#include "lwip/sockets.h"
#include "lwip/netif.h"
#define SOCK_CLOSE(s)   lwip_close(s)
#define SOCK_INVALID    (-1)
typedef int             sock_t;
#endif

#define UDS_TCP_PORT     (13400U)
#define RX_BUF_SIZE      (256U)
#define TX_BUF_SIZE      (256U)

typedef struct {
    sock_t   server_fd;
    sock_t   client_fd;
    uint8_t  rx_buf[RX_BUF_SIZE];
    uint16_t rx_len;
    uint8_t  tx_buf[TX_BUF_SIZE];
    uint16_t tx_len;
    uint8_t  connected;
} TcpTp_t;

static TcpTp_t s_tp;

/* ── UDSTp interface functions ─────────────────────────────── */

static ssize_t TcpTp_Send(UDSTp_t *tp, uint8_t *buf,
                           size_t len, UDSSDU_t *info)
{
    (void)tp; (void)info;
    if (!s_tp.connected || s_tp.client_fd == SOCK_INVALID)
        return -1;
    return send(s_tp.client_fd, buf, len, 0);
}

static ssize_t TcpTp_Recv(UDSTp_t *tp, uint8_t *buf,
                           size_t bufsize, UDSSDU_t *info)
{
    (void)tp; (void)info;
    ssize_t n;
    if (!s_tp.connected || s_tp.client_fd == SOCK_INVALID)
        return 0;
    n = recv(s_tp.client_fd, buf, bufsize, MSG_DONTWAIT);
    if (n < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK) { return 0; }
        s_tp.connected = 0U;
        return -1;
    }
    if (n == 0) { s_tp.connected = 0U; return 0; }
    return n;
}

static UDSTpStatus_t TcpTp_Poll(UDSTp_t *tp)
{
    (void)tp;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    sock_t new_fd;

    /* Accept new connection if none active */
    if (!s_tp.connected)
    {
        new_fd = accept(s_tp.server_fd,
                        (struct sockaddr *)&client_addr,
                        &addr_len);
        if (new_fd != SOCK_INVALID)
        {
            if (s_tp.client_fd != SOCK_INVALID)
                SOCK_CLOSE(s_tp.client_fd);
            s_tp.client_fd = new_fd;
            s_tp.connected = 1U;
            /* Set non-blocking */
            fcntl(new_fd, F_SETFL, O_NONBLOCK);
        }
    }
    return 0U;
}

/* ── Public API ─────────────────────────────────────────────── */

static UDSTp_t s_udsTp = {
    .send = TcpTp_Send,
    .recv = TcpTp_Recv,
    .poll = TcpTp_Poll,
};

UDSTp_t *TcpTp_Init(void)
{
    struct sockaddr_in addr;
    int opt = 1;

    memset(&s_tp, 0, sizeof(s_tp));
    s_tp.client_fd = SOCK_INVALID;

    s_tp.server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (s_tp.server_fd == SOCK_INVALID) { return NULL; }

    setsockopt(s_tp.server_fd, SOL_SOCKET,
               SO_REUSEADDR, &opt, sizeof(opt));

    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(UDS_TCP_PORT);

    if (bind(s_tp.server_fd,
             (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        SOCK_CLOSE(s_tp.server_fd);
        return NULL;
    }

    listen(s_tp.server_fd, 1);

    /* Non-blocking accept */
    fcntl(s_tp.server_fd, F_SETFL, O_NONBLOCK);

    return &s_udsTp;
}

void TcpTp_Deinit(void)
{
    if (s_tp.client_fd != SOCK_INVALID) SOCK_CLOSE(s_tp.client_fd);
    if (s_tp.server_fd != SOCK_INVALID) SOCK_CLOSE(s_tp.server_fd);
    memset(&s_tp, 0, sizeof(s_tp));
}
