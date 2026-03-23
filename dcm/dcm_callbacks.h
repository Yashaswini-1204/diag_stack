#ifndef DCM_CALLBACKS_H
#define DCM_CALLBACKS_H

#include "../dcm/iso14229/iso14229.h"

/* Main callback — wire this to UDSServer_t.fn */
UDSErr_t DCM_ServerCallback(UDSServer_t *srv,
                             UDSEvent_t   event,
                             void        *arg);

#endif
