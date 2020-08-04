//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2016-10-31 papa Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
// 2020-08-01 jp112sdl Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------

// use Arduino IDE Board Setting: BOBUINO Layout

#define DEVICE_CHANNEL_COUNT 4

#include <Adafruit_GFX.h>
#include <Adafruit_SharpMem.h>
#include "U8G2_FONTS_GFX.h"
#define EI_NOTEXTERNAL
#include <EnableInterrupt.h>
#include <SPI.h>
#include <AskSinPP.h>
#include <LowPower.h>
#include <Register.h>
#include <MultiChannelDevice.h>
#include <Remote.h>
#include <sensors/Sht31.h>

#define CC1101_CS_PIN       10 // PB4
#define CC1101_GDO0_PIN      2 // PD2
#define CONFIG_BUTTON_PIN    8 // PD5
#define LED_PIN_1            4 // PB0
#define BTN01_PIN           A2 // PA5
#define BTN02_PIN           A4 // PA3
#define BTN03_PIN           A6 // PA1
#define BTN04_PIN           A5 // PA2

#define BATTERY_CRITICAL    22

#define SHARP_SCK  13
#define SHARP_MOSI 11
#define SHARP_SS   7

Adafruit_SharpMem display(&SPI, SHARP_SS, 128, 128);
U8G2_FONTS_GFX u8g2Fonts(display);
typedef enum screens { SCREEN_KEYLABELS, SCREEN_TEMPERATURE } Screen;

#define BLACK       0
#define WHITE       1
#define TEXT_FONT   u8g2_font_helvR14_tf
#define HEADER_FONT u8g2_font_helvR08_tf

#define CHANNEL_COUNT       4
#define TEXT_LENGTH        10
#define PEERS_PER_CHANNEL   8

#define SENSOR       Sht31<>

using namespace as;

typedef struct {
  String HeaderText  = "";
  String MainText1   = "";
  String MainText2   = "";
  bool   showHeader = true;
} DisplayRCConfig;
DisplayRCConfig DisplayFields[CHANNEL_COUNT];

const struct DeviceInfo PROGMEM devinfo = {
  {0xf3, 0x2f, 0x01},       // Device ID
  "JPRCDISTH1",             // Device Serial
  {0xf3, 0x2f},             // Device Model
  0x10,                     // Firmware Version
  as::DeviceType::Remote,   // Device Type
  {0x00, 0x00}              // Info Bytes
};

typedef LibSPI<CC1101_CS_PIN> SPIType;
typedef Radio<SPIType, CC1101_GDO0_PIN> RadioType;
typedef StatusLed<LED_PIN_1> LedType;
typedef AskSin<LedType, IrqInternalBatt, RadioType> Hal;
Hal hal;

class DisplayType : public Alarm {
private:
  uint8_t screen;
  uint8_t current_screen;
  uint8_t timeout;
  int16_t temperature;
  uint8_t humidity;
  uint16_t battery;
  uint16_t battery_low;
private:
  uint16_t centerPosition(const char * text) { return centerPosition(display.width(), text); }
  uint16_t centerPosition(uint8_t width, const char * text) { return (width / 2) - (u8g2Fonts.getUTF8Width(text) / 2); }
public:
  DisplayType () :  Alarm(seconds2ticks(1)), screen(SCREEN_KEYLABELS), current_screen(SCREEN_KEYLABELS), timeout(10), temperature(0), humidity(0), battery(0), battery_low(0) {}
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
      display.setRotation(3);

      const char * t_unit = "°C";
      const char * h_unit = "%rH";

      static int16_t last_temperature = 0;
      static uint8_t last_humidity = 0;
      static uint8_t last_battpct = 0;

      //draw main frame on first call
      if (current_screen != Screen::SCREEN_TEMPERATURE) {
        display.fillRect(0, 0, display.width(), display.height(), WHITE);
        for (uint8_t i = 0; i < 3; i ++)
          display.drawLine(0, (display.height() / 2) - 8 + i, display.width(), (display.height() / 2) - 8 + i, BLACK);

        display.drawLine(0, display.height() -17, display.width(), display.height() -17, BLACK);
        display.drawLine(0, display.height() -18, display.width(), display.height() -18, BLACK);

        display.drawRect(display.width()/2-14, display.height()-14, 20, 12, BLACK);
        display.fillRect(display.width()/2-14+20, display.height()-12, 4, 8, BLACK);
     }


      if (temperature != last_temperature || current_screen != Screen::SCREEN_TEMPERATURE) {
        display.fillRect(0,0,display.width(),(display.height() / 2) - 8, WHITE);
        float temp_float = this->temperature / 10.0F;
        char t[4];
        dtostrf(temp_float, 2, 1, t);

        u8g2Fonts.setFont(u8g2_font_logisoso42_tn);
        u8g2Fonts.drawStr(3, display.height() - 78, t);
        u8g2Fonts.setCursor(u8g2Fonts.getUTF8Width(t) + 6, display.height() - 94);
        u8g2Fonts.setFont(u8g2_font_logisoso22_tf);
        u8g2Fonts.print(t_unit);
      }

      if (humidity != last_humidity || current_screen != Screen::SCREEN_TEMPERATURE) {
        display.fillRect(0,(display.height() / 2) - 5 ,display.width(),(display.height() -17) - (display.height() / 2 ) , WHITE);
        char hum[4];
        itoa(this->humidity, hum, 10);
        u8g2Fonts.setFont(u8g2_font_logisoso34_tn);
        uint8_t h_width = u8g2Fonts.getUTF8Width(hum);
        u8g2Fonts.setFont(u8g2_font_logisoso20_tr);
        uint8_t h_unit_width = u8g2Fonts.getUTF8Width(h_unit);
        u8g2Fonts.setFont(u8g2_font_logisoso34_tn);
        u8g2Fonts.setCursor((display.width() / 2) - ((h_unit_width + h_width) / 2), display.height() - 26);
        u8g2Fonts.print(hum);
        u8g2Fonts.setCursor((display.width() / 2) + ((h_width / 2) / 2), display.height() - 26);
        u8g2Fonts.setFont(u8g2_font_logisoso20_tr);
        u8g2Fonts.print(h_unit);
      }

      uint8_t max = 30 - battery_low;
      uint8_t diff = battery - battery_low;
      uint8_t battpct = (100 * diff) / max;

      if (battpct != last_battpct || current_screen != Screen::SCREEN_TEMPERATURE) {
        display.fillRect(display.width()/2-14 + 3 , display.height()-12, 4, 8, battpct > 40 ? BLACK: WHITE);
        display.fillRect(display.width()/2-14 + 3 + 4 + 1 , display.height()-12, 4, 8, battpct > 60 ? BLACK : WHITE);
        display.fillRect(display.width()/2-14 + 3 + 4 + 1 + 4 + 1 , display.height()-12, 4, 8, battpct > 80 ? BLACK : WHITE);
      }

      display.refresh();

      current_screen = Screen::SCREEN_TEMPERATURE;
      last_temperature = temperature;
      last_humidity = humidity;
      last_battpct = battpct;
  }

  void showKeyLabels() {
    current_screen = Screen::SCREEN_KEYLABELS;
    //display.fillRect(0, 0, display.width(), display.height(), WHITE);
    display.clearDisplay();
    for (uint8_t i = 0; i < display.height() / 4; i++)
      display.fillCircle(display.width()/2 -1 , i * 4, 1, BLACK);

    for (uint8_t i = 0; i < display.width() / 4; i++)
      display.fillCircle(i * 4, display.height()/2, 1, BLACK);

    display.refresh();

    u8g2Fonts.setFont(HEADER_FONT);
    for (uint8_t i = 0; i< CHANNEL_COUNT;i++) {
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
    display.refresh();


    u8g2Fonts.setFont(TEXT_FONT);
    const uint8_t w = (display.width() / 2) - 2;

    for (uint8_t i = 0; i< CHANNEL_COUNT;i++) {
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


    display.refresh();
    screen = Screen::SCREEN_TEMPERATURE;
  }

  void showInitScreen(char*serial) {
    screen = SCREEN_KEYLABELS;

    const char * asksinpp     PROGMEM = "AskSin++";
    const char * version      PROGMEM = "V " ASKSIN_PLUS_PLUS_VERSION;
    const char * compiledDate PROGMEM = __DATE__ ;
    const char * compiledTime PROGMEM = __TIME__;
    const char * ser                  = (char*)serial;

    u8g2Fonts.setFont(u8g2_font_helvB12_tr);
    u8g2Fonts.setCursor(centerPosition(asksinpp), 14);
    u8g2Fonts.print(asksinpp);

    u8g2Fonts.setCursor(centerPosition(version), 32);
    u8g2Fonts.print(version);

    u8g2Fonts.setFont(u8g2_font_helvR12_tr);
    u8g2Fonts.setCursor(centerPosition(compiledDate), 68);
    u8g2Fonts.print(compiledDate);
    u8g2Fonts.setCursor(centerPosition(compiledTime), 88);
    u8g2Fonts.print(compiledTime);

    u8g2Fonts.setFont(u8g2_font_helvB12_tr);
    u8g2Fonts.setCursor(centerPosition((char*)serial), display.height()-2);
    u8g2Fonts.print(ser);

    display.refresh();

    set(seconds2ticks(2), sysclock);
  }

  void init() {
    u8g2Fonts.begin(display);
    display.begin();
    u8g2Fonts.setForegroundColor(BLACK);
    u8g2Fonts.setBackgroundColor(WHITE);
    display.clearDisplay();
    display.setRotation(3);
  }
};
DisplayType Display;

DEFREGISTER(Reg0, MASTERID_REGS, DREG_LOWBATLIMIT, DREG_BACKONTIME)
class HBList0 : public RegList0<Reg0> {
  public:
    HBList0(uint16_t addr) : RegList0<Reg0>(addr) {}

    void defaults () {
      clear();
      lowBatLimit(24);
      backOnTime(10);
    }
};

DEFREGISTER(RCEPReg1, CREG_LONGPRESSTIME, CREG_AES_ACTIVE, CREG_DOUBLEPRESSTIME,
    0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
    0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    0x90)
class RCEPList1 : public RegList1<RCEPReg1> {
  public:
    RCEPList1 (uint16_t addr) : RegList1<RCEPReg1>(addr) {}

    bool showHeader (uint8_t value) const { return this->writeRegister(0x90, 0x01, 0, value & 0xff); }
    bool showHeader () const { return this->readRegister(0x90, 0x01, 0, false); }

    bool headerText (uint8_t value[TEXT_LENGTH]) const { for (int i = 0; i < TEXT_LENGTH; i++) { this->writeRegister(0x36 + i, value[i] & 0xff); } return true; }
    String headerText () const { String a = ""; for (int i = 0; i < TEXT_LENGTH; i++) { byte b = this->readRegister(0x36 + i, 0x20); if (b == 0x00) b = 0x20; a += char(b); } return a; }

    bool mainText1 (uint8_t value[TEXT_LENGTH]) const { for (int i = 0; i < TEXT_LENGTH; i++) { this->writeRegister(0x46 + i, value[i] & 0xff); } return true; }
    String mainText1 () const { String a = ""; for (int i = 0; i < TEXT_LENGTH; i++) { byte b = this->readRegister(0x46 + i, 0x20); if (b == 0x00) b = 0x20; a += char(b); } return a; }

    bool mainText2 (uint8_t value[TEXT_LENGTH]) const { for (int i = 0; i < TEXT_LENGTH; i++) { this->writeRegister(0x56 + i, value[i] & 0xff); } return true; }
    String mainText2 () const { String a = ""; for (int i = 0; i < TEXT_LENGTH; i++) { byte b = this->readRegister(0x56 + i, 0x20); if (b == 0x00) b = 0x20; a += char(b); } return a; }


    void defaults () {
      clear();
      //aesActive(false);
      uint8_t initValues[TEXT_LENGTH];
      memset(initValues, 0x00, TEXT_LENGTH);
      headerText(initValues);
      mainText1(initValues);
      mainText2(initValues);
    }
};

DEFREGISTER(THReg1, 0x01, 0x02, 0x20, 0x21)
class THList1 : public RegList1<THReg1> {
  public:
    THList1 (uint16_t addr) : RegList1<THReg1>(addr) {}

    bool sendIntervall (uint16_t value) const { return this->writeRegister(0x20, (value >> 8) & 0xff) && this->writeRegister(0x21, value & 0xff); }
    uint16_t sendIntervall () const { return (this->readRegister(0x20, 0) << 8) + this->readRegister(0x21, 0); }

    bool TemperatureOffset (int8_t value) const { return this->writeRegister(0x01, (value) & 0xff); }
    int8_t TemperatureOffset () const { return (int8_t)(this->readRegister(0x01, 0)); }

    bool HumidityOffset (int8_t value) const { return this->writeRegister(0x02, (value) & 0xff); }
    int8_t HumidityOffset () const { return (int8_t)(this->readRegister(0x02, 0)); }

    void defaults () {
      clear();
      TemperatureOffset(0);
      HumidityOffset(0);
      sendIntervall(180);
    }
};

class ConfigChannel : public RemoteChannel<Hal,PEERS_PER_CHANNEL,HBList0, RCEPList1>  {
public:
  ConfigChannel () : RemoteChannel()  {}
    virtual ~ConfigChannel () {}

    void configChanged() {
      RemoteChannel::configChanged();

      DisplayFields[number() -1].showHeader = this->getList1().showHeader();
      DisplayFields[number() -1].HeaderText = this->getList1().headerText();
      DisplayFields[number() -1].MainText1 = this->getList1().mainText1();
      DisplayFields[number() -1].MainText2 = this->getList1().mainText2();

      DisplayFields[number() -1].HeaderText.trim();
      DisplayFields[number() -1].MainText1.trim();
      DisplayFields[number() -1].MainText2.trim();

      typedef struct rmap { char in; String out; } rmap_t;
      const rmap_t rplmap[8] = { { '{', "ä" }, { '|', "ö" }, { '}', "ü" }, { '[', "Ä" }, { '#', "Ö" }, { '$', "Ü" }, { '~', "ß" }, { '\'', "=" } };
      for (uint8_t i = 0; i < 8; i++) {
        DisplayFields[number() -1].HeaderText.replace(String(rplmap[i].in), rplmap[i].out);
         DisplayFields[number() -1].MainText1.replace(String(rplmap[i].in), rplmap[i].out);
         DisplayFields[number() -1].MainText2.replace(String(rplmap[i].in), rplmap[i].out);
      }

      //DPRINT(F("RC (#"));DDEC(number());DPRINT(F(") showHeader : "));DDECLN(this->getList1().showHeader());
      //DPRINT(F("RC (#"));DDEC(number());DPRINT(F(") HeaderText : "));DPRINTLN(DisplayFields[number() -1].HeaderText);
      //DPRINT(F("RC (#"));DDEC(number());DPRINT(F(") MainText1  : "));DPRINTLN(this->getList1().mainText1());
      //DPRINT(F("RC (#"));DDEC(number());DPRINT(F(") MainText2  : "));DPRINTLN(this->getList1().mainText2());
    }
};

class WeatherEventMsg : public Message {
  public:
    void init(uint8_t msgcnt, int16_t temp, uint8_t humidity, bool batlow, uint8_t bvolt) {
      uint8_t t1 = (temp >> 8) & 0x7f;
      uint8_t t2 = temp & 0xff;
      if ( batlow == true ) {
        t1 |= 0x80; // set bat low bit
      }
      Message::init(0xd, msgcnt, 0x70, BIDI | WKMEUP, t1, t2);
      pload[0] = humidity & 0xff;
      pload[1] = bvolt & 0xff;
    }
};

class WeatherChannel : public Channel<Hal, THList1, EmptyList, List4, PEERS_PER_CHANNEL, HBList0>, public Alarm {

    WeatherEventMsg msg;
    int16_t         temp;
    uint8_t         humidity;

    SENSOR          sensor;
    uint16_t        millis;

  public:
    WeatherChannel () : Channel(), Alarm(5), temp(0), humidity(0), millis(0) {}
    virtual ~WeatherChannel () {}


    // here we do the measurement
    void measure () {
      DPRINT("Measure...\n");
      sensor.measure();

      temp = sensor.temperature();
      humidity = sensor.humidity();

      DPRINT(F("T/H = "));DDEC(temp);DPRINT(F("/"));DDECLN(humidity);
    }

    virtual void trigger (__attribute__ ((unused)) AlarmClock& clock) {
      uint8_t msgcnt = device().nextcount();
      // reactivate for next measure
      uint16_t updCycle = max(10,this->getList1().sendIntervall());
      tick = seconds2ticks(updCycle);
      clock.add(*this);
      measure();
      Display.setWeatherValues(temp, humidity, device().battery().current(), device().getList0().lowBatLimit());
      if (Display.currentScreen() == SCREEN_TEMPERATURE) Display.showTemp();
      msg.init(msgcnt, temp, humidity, device().battery().low(), device().battery().current());
      device().getList0().ledMode(false);
      device().broadcastEvent(msg);
      device().getList0().ledMode(true);
    }

    void setup(Device<Hal, HBList0>* dev, uint8_t number, uint16_t addr) {
      Channel::setup(dev, number, addr);
      sensor.init();
      sysclock.add(*this);
    }

    uint8_t status () const { return 0; }

    uint8_t flags () const { return 0; }

    virtual void configChanged () {
      //DPRINT(F("WC (#"));DDEC(number());DPRINT(F(") TEMP OFFSET : "));DDECLN(this->getList1().TemperatureOffset());
      //DPRINT(F("WC (#"));DDEC(number());DPRINT(F(") HUMI OFFSET : "));DDECLN(this->getList1().HumidityOffset());
      //DPRINT(F("WC (#"));DDEC(number());DPRINT(F(") SEND INTERV : "));DDECLN(this->getList1().sendIntervall());
    }
};

class MixDevice : public ChannelDevice<Hal, VirtBaseChannel<Hal, HBList0>, CHANNEL_COUNT+1, HBList0> {
  public:
      VirtChannel<Hal, WeatherChannel, HBList0> weatherChannel;
      VirtChannel<Hal, ConfigChannel,  HBList0> remChannel[CHANNEL_COUNT];
  public:
    typedef ChannelDevice<Hal, VirtBaseChannel<Hal, HBList0>, CHANNEL_COUNT+1, HBList0> DeviceType;
    MixDevice (const DeviceInfo& info, uint16_t addr) : DeviceType(info, addr) {
      DeviceType::registerChannel(weatherChannel, 5);
      for (uint8_t i = 0; i < CHANNEL_COUNT; ++i) DeviceType::registerChannel(remChannel[i], i + 1);
    }
    virtual ~MixDevice () {}

    ConfigChannel& btnChannel (uint8_t c) {
      return remChannel[c - 1];
    }

    virtual void configChanged () {
      uint8_t disptimeout = getList0().backOnTime();
      //DPRINT(F("List0 DISP TIMEOUT  : ")); DDECLN(this->getList0().backOnTime());
      Display.setScreenKeysTimeout(disptimeout);
      uint8_t lowbat = getList0().lowBatLimit();
      if( lowbat > 0 )
        battery().low(lowbat);
      //DPRINT(F("List0 LOWBAT        : ")); DDECLN(this->getList0().lowBatLimit());
    }
};

MixDevice sdev(devinfo, 0x20);


class ConfBtn : public ConfigButton<MixDevice>  {
public:
  ConfBtn (MixDevice& i) : ConfigButton(i) {}
  virtual ~ConfBtn () {}

  virtual void state (uint8_t s) {
    uint8_t old = ButtonType::state();
    ButtonType::state(s);
    if( s == ButtonType::released ) {
      Display.setNextScreen(Screen::SCREEN_KEYLABELS);
    }
    else if( s == ButtonType::longreleased ) {
      sdev.startPairing();
    }
    else if( s == ButtonType::longpressed ) {
      if( old == ButtonType::longpressed ) {
        if( sdev.getList0().localResetDisable() == false ) {
          sdev.reset(); // long pressed again - reset
        }
      }
      else {
        sdev.led().set(LedStates::key_long);
      }
    }
  }
};

ConfBtn cfgBtn(sdev);
void setup() {
  DINIT(57600, ASKSIN_PLUS_PLUS_IDENTIFIER);

  Display.init();
  uint8_t serial[11];
  sdev.getDeviceSerial(serial);
  serial[10] = 0;
  Display.showInitScreen((char*)serial);

  sdev.init(hal);

  hal.battery.init(seconds2ticks(60UL*60),sysclock);
  hal.battery.critical(BATTERY_CRITICAL);

  remoteChannelISR(sdev.btnChannel(1), BTN01_PIN);
  remoteChannelISR(sdev.btnChannel(2), BTN02_PIN);
  remoteChannelISR(sdev.btnChannel(3), BTN03_PIN);
  remoteChannelISR(sdev.btnChannel(4), BTN04_PIN);

  buttonISR(cfgBtn, CONFIG_BUTTON_PIN);
  while (hal.battery.current() == 0);
  sdev.initDone();
}


void loop() {
  bool worked = hal.runready();
  bool poll = sdev.pollRadio();
  if ( worked == false && poll == false ) {
    if (hal.battery.critical()) {
      hal.activity.sleepForever(hal);
    }
      hal.activity.savePower<Sleep<>>(hal);
  }
}
