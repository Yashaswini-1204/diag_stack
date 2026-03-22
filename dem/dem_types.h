#ifndef DEM_TYPES_H
#define DEM_TYPES_H

#include <stdint.h>
#include <stddef.h>

typedef uint8_t Std_ReturnType;
#define E_OK        ((Std_ReturnType)0x00U)
#define E_NOT_OK    ((Std_ReturnType)0x01U)

typedef uint16_t Dem_EventIdType;
#define DEM_EVENT_ID_INVALID  ((Dem_EventIdType)0x0000U)

typedef uint8_t Dem_EventStatusType;
#define DEM_EVENT_STATUS_PASSED     ((Dem_EventStatusType)0x00U)
#define DEM_EVENT_STATUS_FAILED     ((Dem_EventStatusType)0x01U)
#define DEM_EVENT_STATUS_PREPASSED  ((Dem_EventStatusType)0x02U)
#define DEM_EVENT_STATUS_PREFAILED  ((Dem_EventStatusType)0x03U)

#define DEM_UDS_STATUS_TF      (0x01U)
#define DEM_UDS_STATUS_TFTOC   (0x02U)
#define DEM_UDS_STATUS_PDTC    (0x04U)
#define DEM_UDS_STATUS_CDTC    (0x08U)
#define DEM_UDS_STATUS_TNCSLC  (0x10U)
#define DEM_UDS_STATUS_TFSLC   (0x20U)
#define DEM_UDS_STATUS_TNCTOC  (0x40U)
#define DEM_UDS_STATUS_WIR     (0x80U)

#define DEM_UDS_STATUS_DEFAULT (DEM_UDS_STATUS_TNCSLC | DEM_UDS_STATUS_TNCTOC)

typedef uint8_t Dem_DTCFormatType;
#define DEM_DTC_FORMAT_UDS  ((Dem_DTCFormatType)0x01U)
#define DEM_DTC_FORMAT_OBD  ((Dem_DTCFormatType)0x00U)

typedef uint8_t Dem_DTCOriginType;
#define DEM_DTC_ORIGIN_PRIMARY_MEMORY ((Dem_DTCOriginType)0x01U)
#define DEM_DTC_ORIGIN_MIRROR_MEMORY  ((Dem_DTCOriginType)0x02U)

typedef uint8_t Dem_BooleanType;
#define DEM_FALSE ((Dem_BooleanType)0x00U)
#define DEM_TRUE  ((Dem_BooleanType)0x01U)

#endif
