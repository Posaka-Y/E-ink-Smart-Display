/*****************************************************************************
* | File      	:	EPD_5in79b.c
* | Author      :   Waveshare team
* | Function    :   Electronic paper driver
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2024-03-05
******************************************************************************/
#include "EPD_5in79b.h"
#include "Debug.h"

/******************************************************************************
function :	Software reset
parameter:
******************************************************************************/
static void EPD_5in79b_Reset(void)
{
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(200);
    DEV_Digital_Write(EPD_RST_PIN, 0);
    DEV_Delay_ms(1);
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(200);
}

/******************************************************************************
function :	send command
parameter:
     Reg : Command register
******************************************************************************/
static void EPD_5in79b_SendCommand(UBYTE Reg)
{
    DEV_Digital_Write(EPD_DC_PIN, 0);
    DEV_Digital_Write(EPD_CS_PIN, 0);
    DEV_SPI_WriteByte(Reg);
    DEV_Digital_Write(EPD_CS_PIN, 1);
}

/******************************************************************************
function :	send data
parameter:
    Data : Write data
******************************************************************************/
static void EPD_5in79b_SendData(UBYTE Data)
{
    DEV_Digital_Write(EPD_CS_PIN, 0);
    DEV_Digital_Write(EPD_DC_PIN, 1);
    DEV_SPI_WriteByte(Data);
    DEV_Digital_Write(EPD_CS_PIN, 1);
}

/******************************************************************************
function :	Wait until the busy_pin goes LOW
parameter:
******************************************************************************/
void EPD_5in79b_WaitUntilIdle(void)
{
    Debug("e-Paper busy\r\n");
	UBYTE busy;
	do
	{
		busy = DEV_Digital_Read(EPD_BUSY_PIN);
        DEV_Delay_ms(10);
	}
	while(busy);
	DEV_Delay_ms(200);
    Debug("e-Paper busy release\r\n");
}

/******************************************************************************
function :	Turn On Display
parameter:
******************************************************************************/
static void EPD_5in79b_TurnOnDisplay(void)
{
    EPD_5in79b_SendCommand(0x22);
    EPD_5in79b_SendData(0xF7);
	EPD_5in79b_SendCommand(0x20);
	DEV_Delay_ms(100);
	EPD_5in79b_WaitUntilIdle();
}

/******************************************************************************
function :	Initialize the e-Paper register
parameter:
******************************************************************************/
UBYTE EPD_5in79b_Init(void)
{
    EPD_5in79b_Reset();

    EPD_5in79b_WaitUntilIdle();
	EPD_5in79b_SendCommand(0x12); //POWER ON
	EPD_5in79b_WaitUntilIdle();

    EPD_5in79b_SendCommand(0x11);
    EPD_5in79b_SendData(0x01);
    EPD_5in79b_SendCommand(0x44);
	EPD_5in79b_SendData(0x00);
	EPD_5in79b_SendData(0x31); //400/8-1
	EPD_5in79b_SendCommand(0x45);
	EPD_5in79b_SendData(0x0f);
	EPD_5in79b_SendData(0x01);  //300-1
	EPD_5in79b_SendData(0x00);
	EPD_5in79b_SendData(0x00);

    EPD_5in79b_SendCommand(0x4e);
	EPD_5in79b_SendData(0x00);
	EPD_5in79b_SendCommand(0x4f);
	EPD_5in79b_SendData(0x0f);
	EPD_5in79b_SendData(0x01);

    EPD_5in79b_SendCommand(0x91);
    EPD_5in79b_SendData(0x00);

    EPD_5in79b_SendCommand(0xC4);
	EPD_5in79b_SendData(0x31);
	EPD_5in79b_SendData(0x00);
	EPD_5in79b_SendCommand(0xC5);
	EPD_5in79b_SendData(0x0f);
	EPD_5in79b_SendData(0x01);  //300-1
	EPD_5in79b_SendData(0x00);
	EPD_5in79b_SendData(0x00);

    EPD_5in79b_SendCommand(0xCe);
	EPD_5in79b_SendData(0x31);
	EPD_5in79b_SendCommand(0xCf);
	EPD_5in79b_SendData(0x0f);
	EPD_5in79b_SendData(0x01);
    return 0;
}

/******************************************************************************
function :	Clear screen
parameter:
******************************************************************************/
void EPD_5in79b_Clear(void)
{
    UDOUBLE i;
    // M part 396*272
    EPD_5in79b_SendCommand(0x24);
    for(i=0; i<13600; i++)
    {
        EPD_5in79b_SendData(0xff);
    }
    EPD_5in79b_SendCommand(0X26);
    for(i=0; i<13600; i++)
    {
        EPD_5in79b_SendData(0x00);
    }

    // S part 396*272
    EPD_5in79b_SendCommand(0xA4);
    for(i=0; i<13600; i++)
    {
        EPD_5in79b_SendData(0xff);
    }
    EPD_5in79b_SendCommand(0xA6);
    for(i=0; i<13600; i++)
    {
        EPD_5in79b_SendData(0x00);
    }

    EPD_5in79b_TurnOnDisplay();
}

/******************************************************************************
function :	Sends the image buffer in RAM to e-Paper and displays
parameter:
******************************************************************************/
void EPD_5in79b_Display(const UBYTE *blackimage, const UBYTE *ryimage)
{
    UDOUBLE Width, Width1, Height;
    Width =(EPD_5in79b_WIDTH % 16 == 0)?(EPD_5in79b_WIDTH / 16 ):(EPD_5in79b_WIDTH / 16 + 1);
    Width1 =(EPD_5in79b_WIDTH % 8 == 0)?(EPD_5in79b_WIDTH / 8 ):(EPD_5in79b_WIDTH / 8 + 1);
    Height = EPD_5in79b_HEIGHT;

    // M part 396*272
    EPD_5in79b_SendCommand(0x24);
    for (UDOUBLE j = 0; j < Height; j++) {
        for (UDOUBLE i = 0; i < Width; i++) {
            EPD_5in79b_SendData(blackimage[j * Width1 + i]);
        }
    }
    EPD_5in79b_SendCommand(0X26);
    for (UDOUBLE j = 0; j < Height; j++) {
        for (UDOUBLE i = 0; i < Width; i++) {
            EPD_5in79b_SendData(~ryimage[j * Width1 + i]);
        }
    }

    // S part 396*272
    EPD_5in79b_SendCommand(0xA4);
    for (UDOUBLE j = 0; j < Height; j++) {
        for (UDOUBLE i = 0; i < Width; i++) {
            EPD_5in79b_SendData(blackimage[j * Width1 + i + Width - 1]);
        }
    }
    EPD_5in79b_SendCommand(0XA6);
    for (UDOUBLE j = 0; j < Height; j++) {
        for (UDOUBLE i = 0; i < Width; i++) {
            EPD_5in79b_SendData(~ryimage[j * Width1 + i + Width - 1]);
        }
    }

    EPD_5in79b_TurnOnDisplay();
}

/******************************************************************************
function :	Enter sleep mode
parameter:
******************************************************************************/
void EPD_5in79b_Sleep(void)
{
    EPD_5in79b_SendCommand(0X10);   //deep sleep
    EPD_5in79b_SendData(0x03);
}
