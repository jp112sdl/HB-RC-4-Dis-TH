/*
 * ePaperDisplay.h
 *
 *  Created on: 9 Aug 2020
 *      Author: jp112sdl
 */

#ifndef EPAPERDISPLAY_H_
#define EPAPERDISPLAY_H_

#include <Adafruit_GFX.h>
#include <GxEPD2_BW.h>
#include "U8G2_FONTS_GFX.h"
#include <AskSinPP.h>

#define GxRST_PIN      9 // PC6
#define GxBUSY_PIN     3 // PC7
#define GxDC_PIN      30 // PC5
#define GxCS_PIN      31 // PC4

#define MAX_DISPLAY_BUFFER_SIZE 5000
#define MAX_HEIGHT(EPD) (EPD::HEIGHT <= MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8) ? EPD::HEIGHT : MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8))
GxEPD2_BW<GxEPD2_154_M09, MAX_HEIGHT(GxEPD2_154_M09)> display(GxEPD2_154_M09(/*CS=10*/ GxCS_PIN, /*DC=*/ GxDC_PIN, /*RST=*/ GxRST_PIN, /*BUSY=*/ GxBUSY_PIN)); // GDEW0154M09 200x200
U8G2_FONTS_GFX u8g2Fonts(display);
typedef enum screens { SCREEN_KEYLABELS, SCREEN_TEMPERATURE } Screen;

#define BLACK       GxEPD_BLACK
#define WHITE       GxEPD_WHITE

#define FONT_KEYLABEL_TEXT        u8g2_font_helvB14_tf
#define FONT_KEYLABEL_HEADER      u8g2_font_helvB08_tf

#define FONT_TEMPERATURE          u8g2_font_logisoso58_tn
#define FONT_TEMPERATURE_MAX      u8g2_font_logisoso92_tn
#define FONT_TEMPERATURE_UNIT     u8g2_font_logisoso18_tf
#define FONT_TEMPERATURE_MAX_UNIT u8g2_font_logisoso18_tf
#define FONT_HUMIDITY             u8g2_font_logisoso34_tn
#define FONT_HUMIDITY_UNIT        u8g2_font_logisoso16_tr
#define FONT_INITSCREEN_BOLD      u8g2_font_helvB18_tr
#define FONT_INITSCREEN_REG       u8g2_font_helvR18_tr

using namespace as;

typedef struct {
  String HeaderText  = "";
  String MainText1   = "";
  String MainText2   = "";
  bool   showHeader = true;
} DisplayRCConfig;
DisplayRCConfig DisplayFields[DEVICE_CHANNEL_COUNT];

class DisplayType : public Alarm {
  enum DefaultDisplayModes { DDM_TH = 0, DDM_T };
private:
  uint8_t screen;
  uint8_t current_screen;
  uint8_t timeout;
  int16_t temperature;
  uint8_t humidity;
  uint8_t defaultdisplaymode;
  uint16_t battery;
  uint16_t battery_low;
private:
  uint16_t centerPosition(const char * text) { return centerPosition(display.width(), text); }
  uint16_t centerPosition(uint8_t width, const char * text) { return (width / 2) - (u8g2Fonts.getUTF8Width(text) / 2); }
public:
  DisplayType () :  Alarm(seconds2ticks(1)), screen(SCREEN_KEYLABELS), current_screen(SCREEN_KEYLABELS), timeout(10), temperature(0), humidity(0), defaultdisplaymode(DDM_TH), battery(0), battery_low(0) {}
  virtual ~DisplayType () {}
  void cancel (AlarmClock& clock) {
    clock.cancel(*this);
  }
  void set (uint32_t t,AlarmClock& clock) {
    clock.cancel(*this);
    Alarm::set(t);
    clock.add(*this);
  }
  virtual void trigger (__attribute__((unused)) AlarmClock& clock) {
    switch (screen) {
    case SCREEN_KEYLABELS:
      showKeyLabels();
      set(seconds2ticks(timeout),sysclock);
      break;
    case Screen::SCREEN_TEMPERATURE:
      showTemp();
      break;
    }
   // Menu Button pressed...
   // updateDisplay(mustRefreshDisplay);
  }
  void setNextScreen(Screen scr) {
    screen = scr;
    set(millis2ticks(timeout),sysclock);
  }

  void setScreenKeysTimeout(uint8_t t) {
    timeout = t;
  }

  void defaultDisplayMode(uint8_t m) {
    defaultdisplaymode = m;
  }

  uint8_t defaultDisplayMode() {
    return defaultdisplaymode;
  }

  void setWeatherValues(int16_t t, uint8_t h, uint16_t b, uint16_t bl) {
    temperature = t;
    humidity = h;
    battery = b;
    battery_low = bl;
  }

  uint8_t currentScreen() {
    return current_screen;
  }

  void showTemp() {
      const char * t_unit = "°C";
      const char * h_unit = "%rH";

      static int16_t last_temperature = 0;
      static uint8_t last_humidity = 0;
      static uint8_t last_battpct = 0;

      uint8_t max = 30 - battery_low;
      uint8_t diff = battery - battery_low;
      uint8_t battpct = (100 * diff) / max;

      //draw main frame on first call
      if (current_screen != Screen::SCREEN_TEMPERATURE) {
        display.fillScreen(GxEPD_WHITE);
        if (defaultdisplaymode == DDM_TH) {
          for (uint8_t i = 0; i < 3; i ++)
            display.drawLine(0, (display.height() / 2) + i, display.width(), (display.height() / 2) + i, BLACK);
        }

        display.drawLine(0, display.height() -17, display.width(), display.height() -17, BLACK);
        display.drawLine(0, display.height() -18, display.width(), display.height() -18, BLACK);

        display.drawRect(display.width()/2-14, display.height()-14, 20, 12, BLACK);
        display.fillRect(display.width()/2-14+20, display.height()-12, 4, 8, BLACK);
      }

      if (this->temperature != last_temperature || current_screen != Screen::SCREEN_TEMPERATURE) {

        int8_t  t_int = this->temperature / 10;
        uint8_t t_dec = this->temperature % 10;

        if (defaultdisplaymode == DDM_TH) {
          display.fillRect(0,0,display.width(),(display.height() / 2), WHITE);

          u8g2Fonts.setFont(FONT_TEMPERATURE);
          uint8_t t_width = u8g2Fonts.getUTF8Width(String(t_int).c_str());
          u8g2Fonts.setFont(FONT_TEMPERATURE_UNIT);
          uint8_t t_unit_width = u8g2Fonts.getUTF8Width(String(t_unit).c_str());

          u8g2Fonts.setFont(FONT_TEMPERATURE);
          u8g2Fonts.setCursor((display.width() / 2) - ((t_unit_width + t_width) / 2) - 6, display.height() - 67);
          u8g2Fonts.print(t_int);

          u8g2Fonts.setFont(FONT_TEMPERATURE_UNIT);
          u8g2Fonts.setCursor((display.width() / 2) + ((t_width / 2) / 2) + 6, display.height() - 105);
          u8g2Fonts.print(t_unit);
          u8g2Fonts.setCursor((display.width() / 2) + ((t_width / 2) / 2) + 2, display.height() - 67);
          u8g2Fonts.print(".");
          u8g2Fonts.print(t_dec);
        } else {
          display.fillRect(0,0,display.width(),display.height() - 20, WHITE);

          u8g2Fonts.setFont(FONT_TEMPERATURE_MAX);
          uint8_t t_width = u8g2Fonts.getUTF8Width(String(t_int).c_str());
          u8g2Fonts.setFont(FONT_TEMPERATURE_MAX_UNIT);
          uint8_t t_unit_width = u8g2Fonts.getUTF8Width(String(t_unit).c_str());

          u8g2Fonts.setFont(FONT_TEMPERATURE_MAX);
          u8g2Fonts.setCursor((display.width() / 2) - ((t_unit_width + t_width) / 2) - 4, display.height() - 76);
          u8g2Fonts.print(t_int);

          u8g2Fonts.setFont(FONT_TEMPERATURE_MAX_UNIT);
          u8g2Fonts.setCursor((display.width() / 2) + ((t_width / 2) / 2) + 15, display.height() - 148);
          u8g2Fonts.print(t_unit);
          u8g2Fonts.setCursor((display.width() / 2) + ((t_width / 2) / 2) + 12, display.height() - 76);
          u8g2Fonts.print(".");
          u8g2Fonts.print(t_dec);
        }
      }

      if (defaultdisplaymode == DDM_TH && (humidity != last_humidity || current_screen != Screen::SCREEN_TEMPERATURE)) {
        display.fillRect(0,(display.height() / 2) + 3 ,display.width(),(display.height() -23) - (display.height() / 2 ) , WHITE);
        char hum[4];
        itoa(this->humidity, hum, 10);

        u8g2Fonts.setFont(FONT_HUMIDITY);
        uint8_t h_width = u8g2Fonts.getUTF8Width(hum);
        u8g2Fonts.setFont(FONT_HUMIDITY_UNIT);
        uint8_t h_unit_width = u8g2Fonts.getUTF8Width(h_unit);

        u8g2Fonts.setFont(FONT_HUMIDITY);
        u8g2Fonts.setCursor((display.width() / 2) - ((h_unit_width + h_width) / 2), display.height() - 24);
        u8g2Fonts.print(hum);
        u8g2Fonts.setCursor((display.width() / 2) + ((h_width / 2) / 2), display.height() - 24);
        u8g2Fonts.setFont(FONT_HUMIDITY_UNIT);
        u8g2Fonts.print(h_unit);
      }

      if (battpct != last_battpct || current_screen != Screen::SCREEN_TEMPERATURE) {
        display.fillRect(display.width()/2-14 + 3 , display.height()-12, 4, 8, battpct > 40 ? BLACK: WHITE);
        display.fillRect(display.width()/2-14 + 3 + 4 + 1 , display.height()-12, 4, 8, battpct > 60 ? BLACK : WHITE);
        display.fillRect(display.width()/2-14 + 3 + 4 + 1 + 4 + 1 , display.height()-12, 4, 8, battpct > 80 ? BLACK : WHITE);
      }

      display.display();

      current_screen = Screen::SCREEN_TEMPERATURE;
      last_temperature = temperature;
      last_humidity = humidity;
      last_battpct = battpct;
  }

  void showKeyLabels() {
    current_screen = Screen::SCREEN_KEYLABELS;

    display.fillScreen(GxEPD_WHITE);

    for (uint8_t i = 0; i < display.height() / 4; i++)
      display.fillCircle(display.width()/2 -1 , i * 4, 1, BLACK);

    for (uint8_t i = 0; i < display.width() / 4; i++)
      display.fillCircle(i * 4, display.height()/2, 1, BLACK);

    u8g2Fonts.setFont(FONT_KEYLABEL_HEADER);
    for (uint8_t i = 0; i< DEVICE_CHANNEL_COUNT;i++) {
      if (DisplayFields[i].showHeader) {
        const char * ht = DisplayFields[i].HeaderText.c_str();
        if (i == 0) {
          display.drawRoundRect(0, 1, display.width() / 2 - 2, 14, 2, BLACK);
          u8g2Fonts.setCursor(centerPosition((display.width() / 2), ht),12);
        }
        if (i == 1) {
          display.drawRoundRect(display.width() / 2 + 1, 1, display.width() / 2-2, 14, 2, BLACK);
          u8g2Fonts.setCursor((display.width() / 2) + centerPosition((display.width() / 2), ht),12);
        }

        if (i == 2) {
          display.drawRoundRect(0, display.height()-15, display.width() / 2 - 2, 14, 2, BLACK);
          u8g2Fonts.setCursor(centerPosition((display.width() / 2), ht),display.height()-4);
        }
        if (i == 3) {
          display.drawRoundRect(display.width() / 2 + 1, display.height()-15, display.width() / 2-2, 14, 2, BLACK);
          u8g2Fonts.setCursor((display.width() / 2) + centerPosition((display.width() / 2), ht),display.height()-4);
        }

        u8g2Fonts.print(ht);

      }
    }

    u8g2Fonts.setFont(FONT_KEYLABEL_TEXT);
    const uint8_t w = (display.width() / 2) - 2;

    for (uint8_t i = 0; i< DEVICE_CHANNEL_COUNT;i++) {
      if (DisplayFields[i].MainText2 == "") {
        const char * mt1 = DisplayFields[i].MainText1.c_str();
        if (i == 0)
          u8g2Fonts.setCursor(centerPosition(w, mt1), ((display.height() / 8) * 2) + 14);
        if (i == 1)
          u8g2Fonts.setCursor((display.width() / 2) + centerPosition(w, mt1), ((display.height() / 8) * 2) + 14);
        if (i == 2)
          u8g2Fonts.setCursor(centerPosition(w, mt1), ((display.height() / 8) * 5) + 14);
        if (i == 3)
          u8g2Fonts.setCursor((display.width() / 2) + centerPosition(w, mt1), ((display.height() / 8) * 5) + 14);
        u8g2Fonts.print(mt1);
      } else {
        const char * mt1 = DisplayFields[i].MainText1.c_str();
        if (i == 0)
          u8g2Fonts.setCursor(centerPosition(w, mt1), ((display.height() / 8) * 3) - 10);
        if (i == 1)
          u8g2Fonts.setCursor((display.width() / 2) + centerPosition(w, mt1), ((display.height() / 8) * 3) - 10);
        if (i == 2)
          u8g2Fonts.setCursor(centerPosition(w, mt1), ((display.height() / 8) * 6) - 10);
        if (i == 3)
          u8g2Fonts.setCursor((display.width() / 2) + centerPosition(w, mt1), ((display.height() / 8) * 6) - 10);
        u8g2Fonts.print(mt1);

        const char * mt2 = DisplayFields[i].MainText2.c_str();
        if (i == 0)
          u8g2Fonts.setCursor(centerPosition(w, mt2), ((display.height() / 8) * 3) + 8);
        if (i == 1)
          u8g2Fonts.setCursor((display.width() / 2) + centerPosition(w, mt2), ((display.height() / 8) * 3) + 8);
        if (i == 2)
          u8g2Fonts.setCursor(centerPosition(w, mt2), ((display.height() / 8) * 6) + 8);
        if (i == 3)
          u8g2Fonts.setCursor((display.width() / 2) + centerPosition(w, mt2), ((display.height() / 8) * 6) + 8);
        u8g2Fonts.print(mt2);
      }
    }
    display.display();

    screen = Screen::SCREEN_TEMPERATURE;
  }

  void showInitScreen(char*serial) {
    screen = SCREEN_KEYLABELS;

    const char * asksinpp     PROGMEM = "AskSin++";
    const char * version      PROGMEM = "V " ASKSIN_PLUS_PLUS_VERSION;
    const char * compiledDate PROGMEM = __DATE__ ;
    const char * compiledTime PROGMEM = __TIME__;
    const char * ser                  = (char*)serial;

    display.fillScreen(GxEPD_WHITE);
    u8g2Fonts.setFont(FONT_INITSCREEN_BOLD);
    u8g2Fonts.setCursor(centerPosition(asksinpp), 26);
    u8g2Fonts.print(asksinpp);

    u8g2Fonts.setCursor(centerPosition(version), 50);
    u8g2Fonts.print(version);

    u8g2Fonts.setFont(FONT_INITSCREEN_REG);
    u8g2Fonts.setCursor(centerPosition(compiledDate), 82);
    u8g2Fonts.print(compiledDate);
    u8g2Fonts.setCursor(centerPosition(compiledTime), 110);
    u8g2Fonts.print(compiledTime);

    u8g2Fonts.setFont(FONT_INITSCREEN_BOLD);
    u8g2Fonts.setCursor(centerPosition((char*)serial), display.height()-4);
    u8g2Fonts.print(ser);

    display.display();

    set(seconds2ticks(2), sysclock);
  }

  void init() {
    u8g2Fonts.begin(display);
    display.init(57600);
    u8g2Fonts.setForegroundColor(BLACK);
    u8g2Fonts.setBackgroundColor(WHITE);
    display.setRotation(1);
  }
};



#endif /* EPAPERDISPLAY_H_ */
