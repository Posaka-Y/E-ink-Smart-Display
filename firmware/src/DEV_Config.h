/*****************************************************************************
* | File      	:   DEV_Config.h
* | Author      :   Waveshare team (modified for ESP32-C3 Supermini)
* | Function    :   Hardware underlying interface
******************************************************************************/
#ifndef _DEV_CONFIG_H_
#define _DEV_CONFIG_H_

#include <Arduino.h>
#include <stdint.h>
#include <stdio.h>

/**
 * data
**/
#define UBYTE   uint8_t
#define UWORD   uint16_t
#define UDOUBLE uint32_t

/**
 * GPIO config — ESP32-C3 Supermini
 * Strapping pins (avoid as output at boot): GPIO0, GPIO2, GPIO8, GPIO9
 * RTC-capable (Deep Sleep wake): GPIO0-GPIO5
**/
#define EPD_SCK_PIN      6   // SPI clock (bit-bang)
#define EPD_MOSI_PIN     7   // SPI MOSI
#define EPD_CS_PIN      10   // Chip select
#define EPD_RST_PIN      3   // Reset
#define EPD_DC_PIN      21   // Data/Command
#define EPD_BUSY_PIN    20   // Busy signal
#define EPD_PWR_PIN      1   // E-ink power control (active HIGH)

#define I2C_SDA_PIN      8   // AHT20 + BMP280 shared SDA (strapping, Hi-Z at boot OK)
#define I2C_SCL_PIN      9   // AHT20 + BMP280 shared SCL (strapping, Hi-Z at boot OK)
#define PIR_PIN          4   // PIR motion sensor (RTC GPIO for Deep Sleep wake-up)

#define GPIO_PIN_SET   1
#define GPIO_PIN_RESET 0

/**
 * GPIO read and write
**/
#define DEV_Digital_Write(_pin, _value) digitalWrite(_pin, _value == 0? LOW:HIGH)
#define DEV_Digital_Read(_pin) digitalRead(_pin)

/**
 * delay x ms
**/
#define DEV_Delay_ms(__xms) delay(__xms)

/*------------------------------------------------------------------------------------------------------*/
UBYTE DEV_Module_Init(void);
void GPIO_Mode(UWORD GPIO_Pin, UWORD Mode);
void DEV_SPI_WriteByte(UBYTE data);
UBYTE DEV_SPI_ReadByte();
void DEV_SPI_Write_nByte(UBYTE *pData, UDOUBLE len);
void DEV_Module_Exit(void);

#endif
