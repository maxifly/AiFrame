#pragma once
#include "SPI.h"
#include <Adafruit_GFX.h>
#include <ILI9488.h>
#include "gen.h"

/* Подключение Wemos MINI R1 к TFT iLi9488
*
* ESP8266(Wemos MINI) | iLi9488
* ------------------- --------------------
* MISO D6 GPIO 12 -> SDO (MISO)
* +3,3 V -> BL (LED)
* SCK D5 GPIO 14 -> SCK
* MOSI D7 GPIO 13 -> SDI (MOSI)
* D4 GPIO 2 -> D/C
* D3 or +3,3 V -> RST (RESET)
* SS D8 GPIO15 -> CS
* GND -> GND
* +3,3 V -> VCC

* -------------------------------------------

*/

#define TFT_CS D8
#define TFT_DC D4
#define TFT_RST D3

ILI9488 tft = ILI9488(TFT_CS, TFT_DC, TFT_RST);

// SCL->D5
// SDA->D7
// BLK->3V3
// #define TFT_CS D3
// #define TFT_DC D8
// #define TFT_RST D6

// Adafruit_ST7796S_kbv tft(TFT_CS, TFT_DC, TFT_RST);

void tft_render(int x, int y, int w, int h, uint8_t* buf) {
    tft.drawRGBBitmap(x, y, (uint16_t*)buf, w, h);
}

void tft_init() {
    SPI.setFrequency(4000000ul);
    tft.begin();
    tft.setRotation(0);
    tft.fillScreen(0x0000);
    tft.setTextColor(0x07E0);
    tft.setTextSize(2, 2);
    gen.onRender(tft_render);
}