/******************************************************************************
* | File      	:   GUI_Paint.c
* | Author      :   Waveshare electronics
* | Function    :	Achieve drawing: draw points, lines, boxes, circles
* | Info        :
*----------------
* |	This version:   V3.2
* | Date        :   2020-07-23
******************************************************************************/
#include "GUI_Paint.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

PAINT Paint;

/******************************************************************************
function: Create Image
******************************************************************************/
void Paint_NewImage(UBYTE *image, UWORD Width, UWORD Height, UWORD Rotate, UWORD Color)
{
    Paint.Image = NULL;
    Paint.Image = image;
    Paint.WidthMemory = Width;
    Paint.HeightMemory = Height;
    Paint.Color = Color;
    Paint.Scale = 2;
    Paint.WidthByte = (Width % 8 == 0)? (Width / 8 ): (Width / 8 + 1);
    Paint.HeightByte = Height;
    Paint.Rotate = Rotate;
    Paint.Mirror = MIRROR_NONE;
    if(Rotate == ROTATE_0 || Rotate == ROTATE_180) {
        Paint.Width = Width;
        Paint.Height = Height;
    } else {
        Paint.Width = Height;
        Paint.Height = Width;
    }
}

void Paint_SelectImage(UBYTE *image) { Paint.Image = image; }

void Paint_SetRotate(UWORD Rotate)
{
    if(Rotate == ROTATE_0 || Rotate == ROTATE_90 || Rotate == ROTATE_180 || Rotate == ROTATE_270)
        Paint.Rotate = Rotate;
    else
        Debug("rotate = 0, 90, 180, 270\r\n");
}

void Paint_SetMirroring(UBYTE mirror)
{
    if(mirror == MIRROR_NONE || mirror == MIRROR_HORIZONTAL ||
        mirror == MIRROR_VERTICAL || mirror == MIRROR_ORIGIN)
        Paint.Mirror = mirror;
    else
        Debug("mirror error\r\n");
}

void Paint_SetScale(UBYTE scale)
{
    if(scale == 2) {
        Paint.Scale = scale;
        Paint.WidthByte = (Paint.WidthMemory % 8 == 0)? (Paint.WidthMemory / 8 ): (Paint.WidthMemory / 8 + 1);
    } else if(scale == 4) {
        Paint.Scale = scale;
        Paint.WidthByte = (Paint.WidthMemory % 4 == 0)? (Paint.WidthMemory / 4 ): (Paint.WidthMemory / 4 + 1);
    } else if(scale == 6 || scale == 7) {
        Paint.Scale = 7;
        Paint.WidthByte = (Paint.WidthMemory % 2 == 0)? (Paint.WidthMemory / 2 ): (Paint.WidthMemory / 2 + 1);
    } else {
        Debug("Scale Only support: 2 4 7\r\n");
    }
}

/******************************************************************************
function: Draw Pixels
******************************************************************************/
void Paint_SetPixel(UWORD Xpoint, UWORD Ypoint, UWORD Color)
{
    if(Xpoint > Paint.Width || Ypoint > Paint.Height) {
        Debug("Exceeding display boundaries\r\n");
        return;
    }
    UWORD X, Y;
    switch(Paint.Rotate) {
    case 0:   X = Xpoint; Y = Ypoint; break;
    case 90:  X = Paint.WidthMemory - Ypoint - 1; Y = Xpoint; break;
    case 180: X = Paint.WidthMemory - Xpoint - 1; Y = Paint.HeightMemory - Ypoint - 1; break;
    case 270: X = Ypoint; Y = Paint.HeightMemory - Xpoint - 1; break;
    default:  return;
    }
    switch(Paint.Mirror) {
    case MIRROR_NONE:       break;
    case MIRROR_HORIZONTAL: X = Paint.WidthMemory - X - 1; break;
    case MIRROR_VERTICAL:   Y = Paint.HeightMemory - Y - 1; break;
    case MIRROR_ORIGIN:     X = Paint.WidthMemory - X - 1; Y = Paint.HeightMemory - Y - 1; break;
    default: return;
    }
    if(X > Paint.WidthMemory || Y > Paint.HeightMemory) {
        Debug("Exceeding display boundaries\r\n");
        return;
    }
    if(Paint.Scale == 2) {
        UDOUBLE Addr = X / 8 + Y * Paint.WidthByte;
        UBYTE Rdata = Paint.Image[Addr];
        if(Color == BLACK)
            Paint.Image[Addr] = Rdata & ~(0x80 >> (X % 8));
        else
            Paint.Image[Addr] = Rdata | (0x80 >> (X % 8));
    } else if(Paint.Scale == 4) {
        UDOUBLE Addr = X / 4 + Y * Paint.WidthByte;
        Color = Color % 4;
        UBYTE Rdata = Paint.Image[Addr];
        Rdata = Rdata & (~(0xC0 >> ((X % 4)*2)));
        Paint.Image[Addr] = Rdata | ((Color << 6) >> ((X % 4)*2));
    } else if(Paint.Scale == 6 || Paint.Scale == 7 || Paint.Scale == 16) {
        UDOUBLE Addr = X / 2  + Y * Paint.WidthByte;
        UBYTE Rdata = Paint.Image[Addr];
        Rdata = Rdata & (~(0xF0 >> ((X % 2)*4)));
        Paint.Image[Addr] = Rdata | ((Color << 4) >> ((X % 2)*4));
    }
}

/******************************************************************************
function: Clear the color of the picture
******************************************************************************/
void Paint_Clear(UWORD Color)
{
    if(Paint.Scale == 2) {
        for (UWORD Y = 0; Y < Paint.HeightByte; Y++)
            for (UWORD X = 0; X < Paint.WidthByte; X++)
                Paint.Image[X + Y*Paint.WidthByte] = Color;
    } else if(Paint.Scale == 4) {
        for (UWORD Y = 0; Y < Paint.HeightByte; Y++)
            for (UWORD X = 0; X < Paint.WidthByte; X++)
                Paint.Image[X + Y*Paint.WidthByte] = (Color<<6)|(Color<<4)|(Color<<2)|Color;
    } else if(Paint.Scale == 6 || Paint.Scale == 7 || Paint.Scale == 16) {
        for (UWORD Y = 0; Y < Paint.HeightByte; Y++)
            for (UWORD X = 0; X < Paint.WidthByte; X++)
                Paint.Image[X + Y*Paint.WidthByte] = (Color<<4)|Color;
    }
}

void Paint_ClearWindows(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend, UWORD Color)
{
    for (UWORD Y = Ystart; Y < Yend; Y++)
        for (UWORD X = Xstart; X < Xend; X++)
            Paint_SetPixel(X, Y, Color);
}

/******************************************************************************
function: Draw Point
******************************************************************************/
void Paint_DrawPoint(UWORD Xpoint, UWORD Ypoint, UWORD Color,
                     DOT_PIXEL Dot_Pixel, DOT_STYLE Dot_Style)
{
    if (Xpoint > Paint.Width || Ypoint > Paint.Height) {
        Debug("Paint_DrawPoint Input exceeds the normal display range\r\n");
        return;
    }
    int16_t XDir_Num, YDir_Num;
    if (Dot_Style == DOT_FILL_AROUND) {
        for (XDir_Num = 0; XDir_Num < 2 * Dot_Pixel - 1; XDir_Num++) {
            for (YDir_Num = 0; YDir_Num < 2 * Dot_Pixel - 1; YDir_Num++) {
                if(Xpoint + XDir_Num - Dot_Pixel < 0 || Ypoint + YDir_Num - Dot_Pixel < 0)
                    break;
                Paint_SetPixel(Xpoint + XDir_Num - Dot_Pixel, Ypoint + YDir_Num - Dot_Pixel, Color);
            }
        }
    } else {
        for (XDir_Num = 0; XDir_Num < Dot_Pixel; XDir_Num++)
            for (YDir_Num = 0; YDir_Num < Dot_Pixel; YDir_Num++)
                Paint_SetPixel(Xpoint + XDir_Num - 1, Ypoint + YDir_Num - 1, Color);
    }
}

/******************************************************************************
function: Draw a line
******************************************************************************/
void Paint_DrawLine(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend,
                    UWORD Color, DOT_PIXEL Line_width, LINE_STYLE Line_Style)
{
    if (Xstart > Paint.Width || Ystart > Paint.Height ||
        Xend > Paint.Width || Yend > Paint.Height) {
        Debug("Paint_DrawLine Input exceeds the normal display range\r\n");
        return;
    }
    UWORD Xpoint = Xstart, Ypoint = Ystart;
    int dx = (int)Xend - (int)Xstart >= 0 ? Xend - Xstart : Xstart - Xend;
    int dy = (int)Yend - (int)Ystart <= 0 ? Yend - Ystart : Ystart - Yend;
    int XAddway = Xstart < Xend ? 1 : -1;
    int YAddway = Ystart < Yend ? 1 : -1;
    int Esp = dx + dy;
    char Dotted_Len = 0;
    for (;;) {
        Dotted_Len++;
        if (Line_Style == LINE_STYLE_DOTTED && Dotted_Len % 3 == 0) {
            Paint_DrawPoint(Xpoint, Ypoint, IMAGE_BACKGROUND, Line_width, DOT_STYLE_DFT);
            Dotted_Len = 0;
        } else {
            Paint_DrawPoint(Xpoint, Ypoint, Color, Line_width, DOT_STYLE_DFT);
        }
        if (2 * Esp >= dy) {
            if (Xpoint == Xend) break;
            Esp += dy;
            Xpoint += XAddway;
        }
        if (2 * Esp <= dx) {
            if (Ypoint == Yend) break;
            Esp += dx;
            Ypoint += YAddway;
        }
    }
}

/******************************************************************************
function: Draw a rectangle
******************************************************************************/
void Paint_DrawRectangle(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend,
                         UWORD Color, DOT_PIXEL Line_width, DRAW_FILL Draw_Fill)
{
    if (Xstart > Paint.Width || Ystart > Paint.Height ||
        Xend > Paint.Width || Yend > Paint.Height) {
        Debug("Input exceeds the normal display range\r\n");
        return;
    }
    if (Draw_Fill) {
        for(UWORD Ypoint = Ystart; Ypoint < Yend; Ypoint++)
            Paint_DrawLine(Xstart, Ypoint, Xend, Ypoint, Color, Line_width, LINE_STYLE_SOLID);
    } else {
        Paint_DrawLine(Xstart, Ystart, Xend, Ystart, Color, Line_width, LINE_STYLE_SOLID);
        Paint_DrawLine(Xstart, Ystart, Xstart, Yend, Color, Line_width, LINE_STYLE_SOLID);
        Paint_DrawLine(Xend, Yend, Xend, Ystart, Color, Line_width, LINE_STYLE_SOLID);
        Paint_DrawLine(Xend, Yend, Xstart, Yend, Color, Line_width, LINE_STYLE_SOLID);
    }
}

/******************************************************************************
function: Draw a circle
******************************************************************************/
void Paint_DrawCircle(UWORD X_Center, UWORD Y_Center, UWORD Radius,
                      UWORD Color, DOT_PIXEL Line_width, DRAW_FILL Draw_Fill)
{
    if (X_Center > Paint.Width || Y_Center >= Paint.Height) {
        Debug("Paint_DrawCircle Input exceeds the normal display range\r\n");
        return;
    }
    int16_t XCurrent = 0, YCurrent = Radius;
    int16_t Esp = 3 - (Radius << 1);
    int16_t sCountY;
    if (Draw_Fill == DRAW_FILL_FULL) {
        while (XCurrent <= YCurrent) {
            for (sCountY = XCurrent; sCountY <= YCurrent; sCountY++) {
                Paint_DrawPoint(X_Center + XCurrent, Y_Center + sCountY, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);
                Paint_DrawPoint(X_Center - XCurrent, Y_Center + sCountY, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);
                Paint_DrawPoint(X_Center - sCountY,  Y_Center + XCurrent, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);
                Paint_DrawPoint(X_Center - sCountY,  Y_Center - XCurrent, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);
                Paint_DrawPoint(X_Center - XCurrent, Y_Center - sCountY, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);
                Paint_DrawPoint(X_Center + XCurrent, Y_Center - sCountY, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);
                Paint_DrawPoint(X_Center + sCountY,  Y_Center - XCurrent, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);
                Paint_DrawPoint(X_Center + sCountY,  Y_Center + XCurrent, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);
            }
            if (Esp < 0) Esp += 4 * XCurrent + 6;
            else { Esp += 10 + 4 * (XCurrent - YCurrent); YCurrent--; }
            XCurrent++;
        }
    } else {
        while (XCurrent <= YCurrent) {
            Paint_DrawPoint(X_Center + XCurrent, Y_Center + YCurrent, Color, Line_width, DOT_STYLE_DFT);
            Paint_DrawPoint(X_Center - XCurrent, Y_Center + YCurrent, Color, Line_width, DOT_STYLE_DFT);
            Paint_DrawPoint(X_Center - YCurrent, Y_Center + XCurrent, Color, Line_width, DOT_STYLE_DFT);
            Paint_DrawPoint(X_Center - YCurrent, Y_Center - XCurrent, Color, Line_width, DOT_STYLE_DFT);
            Paint_DrawPoint(X_Center - XCurrent, Y_Center - YCurrent, Color, Line_width, DOT_STYLE_DFT);
            Paint_DrawPoint(X_Center + XCurrent, Y_Center - YCurrent, Color, Line_width, DOT_STYLE_DFT);
            Paint_DrawPoint(X_Center + YCurrent, Y_Center - XCurrent, Color, Line_width, DOT_STYLE_DFT);
            Paint_DrawPoint(X_Center + YCurrent, Y_Center + XCurrent, Color, Line_width, DOT_STYLE_DFT);
            if (Esp < 0) Esp += 4 * XCurrent + 6;
            else { Esp += 10 + 4 * (XCurrent - YCurrent); YCurrent--; }
            XCurrent++;
        }
    }
}

/******************************************************************************
function: Show English characters
******************************************************************************/
void Paint_DrawChar(UWORD Xpoint, UWORD Ypoint, const char Acsii_Char,
                    sFONT* Font, UWORD Color_Foreground, UWORD Color_Background)
{
    UWORD Page, Column;
    if (Xpoint > Paint.Width || Ypoint > Paint.Height) {
        Debug("Paint_DrawChar Input exceeds the normal display range\r\n");
        return;
    }
    uint32_t Char_Offset = (Acsii_Char - ' ') * Font->Height * (Font->Width / 8 + (Font->Width % 8 ? 1 : 0));
    const unsigned char *ptr = &Font->table[Char_Offset];
    for (Page = 0; Page < Font->Height; Page++) {
        for (Column = 0; Column < Font->Width; Column++) {
            if (FONT_BACKGROUND == Color_Background) {
                if (*ptr & (0x80 >> (Column % 8)))
                    Paint_SetPixel(Xpoint + Column, Ypoint + Page, Color_Foreground);
            } else {
                if (*ptr & (0x80 >> (Column % 8)))
                    Paint_SetPixel(Xpoint + Column, Ypoint + Page, Color_Foreground);
                else
                    Paint_SetPixel(Xpoint + Column, Ypoint + Page, Color_Background);
            }
            if (Column % 8 == 7) ptr++;
        }
        if (Font->Width % 8 != 0) ptr++;
    }
}

/******************************************************************************
function: Display the string
******************************************************************************/
void Paint_DrawString_EN(UWORD Xstart, UWORD Ystart, const char * pString,
                         sFONT* Font, UWORD Color_Foreground, UWORD Color_Background)
{
    UWORD Xpoint = Xstart, Ypoint = Ystart;
    if (Xstart > Paint.Width || Ystart > Paint.Height) {
        Debug("Paint_DrawString_EN Input exceeds the normal display range\r\n");
        return;
    }
    while (*pString != '\0') {
        if ((Xpoint + Font->Width) > Paint.Width) {
            Xpoint = Xstart;
            Ypoint += Font->Height;
        }
        if ((Ypoint + Font->Height) > Paint.Height) {
            Xpoint = Xstart;
            Ypoint = Ystart;
        }
        Paint_DrawChar(Xpoint, Ypoint, *pString, Font, Color_Background, Color_Foreground);
        pString++;
        Xpoint += Font->Width;
    }
}

void Paint_DrawString_CN(UWORD Xstart, UWORD Ystart, const char * pString, cFONT* font,
                        UWORD Color_Foreground, UWORD Color_Background)
{
    const char* p_text = pString;
    int x = Xstart, y = Ystart;
    int i, j, Num;
    while (*p_text != 0) {
        if((*p_text & 0xff) <= 0x7F) {
            for(Num = 0; Num < font->size; Num++) {
                if(*p_text == font->table[Num].index[0]) {
                    const unsigned char* ptr = &font->table[Num].matrix[0];
                    for (j = 0; j < font->Height; j++) {
                        for (i = 0; i < font->Width; i++) {
                            if (*ptr & (0x80 >> (i % 8)))
                                Paint_SetPixel(x + i, y + j, Color_Foreground);
                            else if (FONT_BACKGROUND != Color_Background)
                                Paint_SetPixel(x + i, y + j, Color_Background);
                            if (i % 8 == 7) ptr++;
                        }
                        if (font->Width % 8 != 0) ptr++;
                    }
                    break;
                }
            }
            p_text += 1;
            x += font->ASCII_Width;
        } else {
            for(Num = 0; Num < font->size; Num++) {
                if ((((*p_text)&0xFF) == font->table[Num].index[0]) &&
                    (((*(p_text+1))&0xFF) == font->table[Num].index[1]) &&
                    (((*(p_text+2))&0xFF) == font->table[Num].index[2])) {
                    const unsigned char* ptr = &font->table[Num].matrix[0];
                    for (j = 0; j < font->Height; j++) {
                        for (i = 0; i < font->Width; i++) {
                            if (*ptr & (0x80 >> (i % 8)))
                                Paint_SetPixel(x + i, y + j, Color_Foreground);
                            else if (FONT_BACKGROUND != Color_Background)
                                Paint_SetPixel(x + i, y + j, Color_Background);
                            if (i % 8 == 7) ptr++;
                        }
                        if (font->Width % 8 != 0) ptr++;
                    }
                    break;
                }
            }
            p_text += 3;
            x += font->Width;
        }
    }
}

/******************************************************************************
function: Display number
******************************************************************************/
#define ARRAY_LEN 255
void Paint_DrawNum(UWORD Xpoint, UWORD Ypoint, int32_t Nummber,
                   sFONT* Font, UWORD Color_Foreground, UWORD Color_Background)
{
    int16_t Num_Bit = 0, Str_Bit = 0;
    uint8_t Str_Array[ARRAY_LEN] = {0}, Num_Array[ARRAY_LEN] = {0};
    uint8_t *pStr = Str_Array;
    if (Xpoint > Paint.Width || Ypoint > Paint.Height) {
        Debug("Paint_DisNum Input exceeds the normal display range\r\n");
        return;
    }
    while (Nummber) {
        Num_Array[Num_Bit] = Nummber % 10 + '0';
        Num_Bit++;
        Nummber /= 10;
    }
    while (Num_Bit > 0) {
        Str_Array[Str_Bit] = Num_Array[Num_Bit - 1];
        Str_Bit++;
        Num_Bit--;
    }
    Paint_DrawString_EN(Xpoint, Ypoint, (const char*)pStr, Font, Color_Background, Color_Foreground);
}

/******************************************************************************
function: Display time
******************************************************************************/
void Paint_DrawTime(UWORD Xstart, UWORD Ystart, PAINT_TIME *pTime, sFONT* Font,
                    UWORD Color_Foreground, UWORD Color_Background)
{
    uint8_t value[10] = {'0','1','2','3','4','5','6','7','8','9'};
    UWORD Dx = Font->Width;
    Paint_DrawChar(Xstart,                            Ystart, value[pTime->Hour / 10], Font, Color_Background, Color_Foreground);
    Paint_DrawChar(Xstart + Dx,                       Ystart, value[pTime->Hour % 10], Font, Color_Background, Color_Foreground);
    Paint_DrawChar(Xstart + Dx + Dx/4 + Dx/2,         Ystart, ':',                     Font, Color_Background, Color_Foreground);
    Paint_DrawChar(Xstart + Dx*2 + Dx/2,              Ystart, value[pTime->Min / 10],  Font, Color_Background, Color_Foreground);
    Paint_DrawChar(Xstart + Dx*3 + Dx/2,              Ystart, value[pTime->Min % 10],  Font, Color_Background, Color_Foreground);
    Paint_DrawChar(Xstart + Dx*4 + Dx/2 - Dx/4,      Ystart, ':',                     Font, Color_Background, Color_Foreground);
    Paint_DrawChar(Xstart + Dx*5,                     Ystart, value[pTime->Sec / 10],  Font, Color_Background, Color_Foreground);
    Paint_DrawChar(Xstart + Dx*6,                     Ystart, value[pTime->Sec % 10],  Font, Color_Background, Color_Foreground);
}

void Paint_DrawBitMap(const unsigned char* image_buffer)
{
    UWORD x, y;
    UDOUBLE Addr = 0;
    for (y = 0; y < Paint.HeightByte; y++)
        for (x = 0; x < Paint.WidthByte; x++) {
            Addr = x + y * Paint.WidthByte;
            Paint.Image[Addr] = (unsigned char)image_buffer[Addr];
        }
}

void Paint_DrawBitMap_Paste(const unsigned char* image_buffer, UWORD xStart, UWORD yStart,
                            UWORD imageWidth, UWORD imageHeight, UBYTE flipColor)
{
    UBYTE color, srcImage;
    UWORD x, y;
    UWORD width = (imageWidth % 8 == 0 ? imageWidth/8 : imageWidth/8+1);
    for (y = 0; y < imageHeight; y++) {
        for (x = 0; x < imageWidth; x++) {
            srcImage = image_buffer[y*width + x/8];
            if(flipColor)
                color = (((srcImage<<(x%8) & 0x80) == 0) ? 1 : 0);
            else
                color = (((srcImage<<(x%8) & 0x80) == 0) ? 0 : 1);
            Paint_SetPixel(x+xStart, y+yStart, color);
        }
    }
}

void Paint_DrawImage(const unsigned char *image_buffer, UWORD xStart, UWORD yStart,
                     UWORD W_Image, UWORD H_Image)
{
    UWORD x, y;
    UWORD w_byte = (W_Image%8) ? (W_Image/8)+1 : W_Image/8;
    for (y = 0; y < H_Image; y++)
        for (x = 0; x < w_byte; x++)
            Paint.Image[x + (xStart/8) + ((y+yStart)*Paint.WidthByte)] =
                (unsigned char)image_buffer[x + y * w_byte];
}
