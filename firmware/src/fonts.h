/**
  ******************************************************************************
  * @file    fonts.h
  * @author  MCD Application Team / modified for E-ink project
  *******************************************************************************/
#ifndef __FONTS_H
#define __FONTS_H

#define MAX_HEIGHT_FONT         41
#define MAX_WIDTH_FONT          32
#define OFFSET_BITMAP           54

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdint.h>

// ASCII font struct
typedef struct _tFont {
    const uint8_t *table;
    uint16_t Width;
    uint16_t Height;
} sFONT;

// GB2312 Chinese font struct
typedef struct {
    unsigned char index[4];
    const unsigned char matrix[MAX_HEIGHT_FONT*MAX_WIDTH_FONT/8 + 1];
} CH_CN;

typedef struct {
    const CH_CN *table;
    uint16_t size;
    uint16_t ASCII_Width;
    uint16_t Width;
    uint16_t Height;
} cFONT;

// Standard Waveshare ASCII fonts
extern sFONT Font20;
extern sFONT Font16;
extern sFONT Font12;

// Custom 64px clock font (chars: ' 0123456789:.-')
extern sFONT Font64;

#ifdef __cplusplus
}
#endif

#endif /* __FONTS_H */
