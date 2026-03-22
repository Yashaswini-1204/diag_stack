#ifndef ISOTP_DEFINES_H
#define ISOTP_DEFINES_H
#define ISOTP_RET_OK             0
#define ISOTP_RET_ERROR         -1
#define ISOTP_RET_INPROGRESS    -2
#define ISOTP_RET_OVERFLOW      -3
#define ISOTP_RET_WRONG_SN      -4
#define ISOTP_RET_NO_DATA       -5
#define ISOTP_RET_TIMEOUT       -6
#define ISOTP_RET_LENGTH        -7
#define ISOTP_CAN_FRAME_SIZE     (8U)
#define ISOTP_PCI_TYPE_SINGLE    (0x00U)
#define ISOTP_PCI_TYPE_FIRST     (0x01U)
#define ISOTP_PCI_TYPE_CONSEC    (0x02U)
#define ISOTP_PCI_TYPE_FC        (0x03U)
#define ISOTP_FC_CTS             (0x00U)
#define ISOTP_FC_WAIT            (0x01U)
#define ISOTP_FC_OVFLW           (0x02U)
#define ISOTP_DEFAULT_RESPONSE_TIMEOUT  (1000U)
#define ISOTP_DEFAULT_STM               (0U)
#endif
