#ifndef _WATCH_DOG_H_
#define _WATCH_DOG_H_
#include "main.h"

extern void IWDG_Init(u8 prer,u16 rlr);
extern void IWDG_Feed(void);
#endif

