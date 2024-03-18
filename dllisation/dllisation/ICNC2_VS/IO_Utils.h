#ifndef __IO_UTILS
#define __IO_UTILS

#ifndef MINI_ICNC2
#ifdef ICNC22
    #define USE_INPUT_INT 0
    #define USE_DFM_COUNT 0 // Comptage des front montant sur entrée IN1..16
#else
    #define USE_INPUT_INT 0
    #define USE_DFM_COUNT 0 // Comptage des front montant sur entrée IN1..16

#endif
#else
    #define USE_INPUT_INT 0
    #define USE_DFM_COUNT 0 // Comptage des front montant sur entrée IN1..16
#endif

#if USE_DFM_COUNT == 1
extern unsigned short int DFMCountOuverte[16];
extern unsigned short int DFMCountFermee[16];
extern unsigned short int CntTrappeOuverte;
extern unsigned short int CntTrappeFermee;
extern unsigned short int NbGelluleExpected;


extern int SysNbCntChanel;
#endif

extern unsigned int Forcage0;
extern unsigned int Forcage1;

extern void LEDStatus_initialisation(void);
extern void LEDStatus_SetStateAll(unsigned char State);

extern unsigned char GetDIPPosition(void);

extern void SetOutputAllState(unsigned int state);
//extern unsigned int GetOutputAllState(void);

extern unsigned int GetInputAllState(void);
// Function de test fonctionnement des leds d'entrées
extern void CheckInputLED(void);

extern void SetDAC_SPI(int chanel, unsigned int value);

extern void SetDAC_PWM(int chanel, unsigned int value);


void UpdateForcage0(unsigned int NewForcage0);

void UpdateForcage1(unsigned int NewForcage1);


#endif