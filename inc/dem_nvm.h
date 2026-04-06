#ifndef DEM_NVM_H
#define DEM_NVM_H

#include "dem_types.h"

void           Dem_Nvm_Init(void);
Std_ReturnType Dem_Nvm_Save(void);
Std_ReturnType Dem_Nvm_Load(void);

#endif
