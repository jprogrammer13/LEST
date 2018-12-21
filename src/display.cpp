#include <Arduino.h>
#include <U8g2lib.h>

void display_mode_0(char *s_1, char *s_2, char *s_3, char *s_4, char *s_5, char *s_6, U8G2_SSD1306_128X64_NONAME_F_4W_SW_SPI oled_display);

void oled_display_update(char *s_1, char *s_2, char *s_3, char *s_4, char *s_5, char *s_6, int mode, U8G2_SSD1306_128X64_NONAME_F_4W_SW_SPI oled_display)
{

    switch (mode)
    {
    case 0:
        display_mode_0(s_1, s_2, s_3, s_4, s_5, s_6, oled_display);
        break;
    }
}

void display_mode_0(char *s_1, char *s_2, char *s_3, char *s_4, char *s_5, char *s_6, U8G2_SSD1306_128X64_NONAME_F_4W_SW_SPI oled_display)
{
    oled_display.clearBuffer();               //Pulisco il buffer
    oled_display.setFont(u8g2_font_t0_11_tr); // font che permette la disposizione di 4 righe nel display

    oled_display.drawUTF8(0, 10, s_1);
    oled_display.drawUTF8(0, 20, s_2);
    oled_display.drawUTF8(0, 30, s_3);
    oled_display.drawUTF8(0, 40, s_4);
    oled_display.drawUTF8(0, 50, s_5);
    oled_display.drawUTF8(0, 60, s_6);

    oled_display.sendBuffer(); // Mando le operazioni da eseguire sul display
    delay(100);                //mi fermo 100ms per il refresh
}