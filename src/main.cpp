#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <Wire.h>

#define G D1
#define R D2
#define B D3
#define BT D4

U8G2_SSD1306_128X64_NONAME_F_4W_SW_SPI u8g2(U8G2_R0, D5, D6, D7, D8, D0);

extern void oled_display_update(char *s_1, char *s_2, char *s_3, char *s_4, char *s_5, char *s_6, int mode, U8G2_SSD1306_128X64_NONAME_F_4W_SW_SPI oled_display);

void setup()
{
  pinMode(R, OUTPUT); //Red
  pinMode(G, OUTPUT); //Green
  pinMode(B, OUTPUT); //Blue
  u8g2.begin();
}

void loop()
{
  oled_display_update("Cucio scripta", "Salvo nuova", "configurazione...", "Bau", "Mao", "Montagna", 0, u8g2);
}