#ifndef SMOCIP_DIAG_H
#define SMOCIP_DIAG_H

#include "../dem/dem_types.h"

Std_ReturnType SMOCIP_Diag_Init(void);
void           SMOCIP_Diag_Task(void *pvParameters);

#endif
