#ifndef DCM_MAIN_H
#define DCM_MAIN_H

/* Call once at startup */
void DCM_Init(void);

/* Call every 10ms from your scheduler */
void DCM_MainFunction(void);

#endif
