/*****************************************************************************
* | File      	:	  EPD_5in79b.h
* | Author      :   Waveshare team
* | Function    :   Electronic paper driver
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2024-03-05
******************************************************************************/
#ifndef _EPD_5in79b_h_
#define _EPD_5in79b_h_

#include "DEV_Config.h"

// Display resolution
#define EPD_5in79b_WIDTH       792
#define EPD_5in79b_HEIGHT      272

UBYTE EPD_5in79b_Init(void);
void EPD_5in79b_Clear(void);
void EPD_5in79b_Display(const UBYTE *blackimage, const UBYTE *ryimage);
void EPD_5in79b_Sleep(void);

#endif
