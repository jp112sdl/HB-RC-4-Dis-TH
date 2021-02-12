//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2016-10-31 papa Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
// 2020-08-01 jp112sdl Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
// ci-test=yes board=1284pBOB aes=no

// use Arduino IDE Board Setting: BOBUINO Layout

#define SENSOR_ONLY

#define USE_LIPO

#ifdef USE_LIPO

#define ChargerISR(chan,pin) class __##pin##ISRHandler { \
  public: \
  static void isr () { chan.irq(); } \
     }; \
  chan.init(pin); \
  if( digitalPinToInterrupt(pin) == NOT_AN_INTERRUPT ) \
    enableInterrupt(pin,__##pin##ISRHandler::isr,CHANGE); \
  else \
    attachInterrupt(digitalPinToInterrupt(pin),__##pin##ISRHandler::isr,CHANGE);

#define V_BATT_MAX          40
#define B_CHARGING_PIN      25
#define BATTERY_MEASUREMENT IrqExternalBatt<A4, 24>
#define DEVICE_MODEL        {0xf3, 0x2e}
#define ADDITIONAL_CHANNELS 2
#else
#define V_BATT_MAX          30
#define BATTERY_MEASUREMENT IrqInternalBatt
#define DEVICE_MODEL        {0xf3, 0x2f}
#define ADDITIONAL_CHANNELS 1
#endif

#define DEVICE_CHANNEL_COUNT 4
#define BATTERY_CRITICAL    19 // initial value

#define EI_NOTEXTERNAL
#include <EnableInterrupt.h>

//#include "LcdDisplay.h"
#include "ePaperDisplay.h"

#include <SPI.h>
#include <AskSinPP.h>
#include <LowPower.h>
#include <Register.h>
#include <MultiChannelDevice.h>
#include <Remote.h>
#include <ContactState.h>
#include <sensors/Sht31.h>

// make compatible with v5.0.0
#ifndef ASKSIN_PLUS_PLUS_VERSION_STR
  #define ASKSIN_PLUS_PLUS_VERSION_STR ASKSIN_PLUS_PLUS_VERSION
#endif

#define CC1101_CS_PIN       10 // PB4
#define CC1101_GDO0_PIN      2 // PD2
#define BTN01_PIN           26 // PC4
#define BTN02_PIN           27 // PC5
#define BTN03_PIN           28 // PC6
#define BTN04_PIN           29 // PC7
#define CONFIG_BUTTON_PIN    8 // PD5
#define LED_PIN_1            5 // PB1
#define LED_PIN_2            4 // PB0

#define TEXT_LENGTH               10
#define PEERS_PER_CHANNEL          8

#define SENSOR       Sht31<>

const struct DeviceInfo PROGMEM devinfo = {
  {0xf3, 0x2f, 0x02},       // Device ID
  "JPRCDISTH2",             // Device Serial
  DEVICE_MODEL,             // Device Model
  0x10,                     // Firmware Version
  as::DeviceType::Remote,   // Device Type
  {0x00, 0x00}              // Info Bytes
};

typedef LibSPI<CC1101_CS_PIN> SPIType;
typedef Radio<SPIType, CC1101_GDO0_PIN> RadioType;
typedef DualStatusLed<LED_PIN_1, LED_PIN_2> LedType;
typedef AskSin<LedType, BATTERY_MEASUREMENT, RadioType> Hal;
Hal hal;

DisplayType Display;

#define DREG_DISPLAYMODE 0x03
DEFREGISTER(Reg0, MASTERID_REGS, DREG_LOWBATLIMIT, DREG_LEDMODE, DREG_BACKONTIME, DREG_DISPLAYMODE, DREG_TRANSMITTRYMAX)
class HBList0 : public RegList0<Reg0> {
  public:
    HBList0(uint16_t addr) : RegList0<Reg0>(addr) {}

    bool defaultDisplayMode (uint8_t value) const { return this->writeRegister(DREG_DISPLAYMODE,7,0,value); }
    uint8_t defaultDisplayMode () const { return this->readRegister(DREG_DISPLAYMODE,7,0,0); }

    bool displayHalfDegree (uint8_t value) const { return this->writeRegister(DREG_DISPLAYMODE,1,3,value); }
    bool displayHalfDegree () const { return this->readRegister(DREG_DISPLAYMODE,1,3,0); }

    void defaults () {
      clear();
#ifdef USE_LIPO
      lowBatLimit(31);
#else
      lowBatLimit(24);
#endif
      backOnTime(10);
      defaultDisplayMode(0);
      displayHalfDegree(false);
      ledMode(1);
      transmitDevTryMax(6);
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

    bool TemperatureOffsetIndex (uint8_t value) const { return this->writeRegister(0x01, (value) & 0xff); }
    uint8_t TemperatureOffsetIndex () const { return (int8_t)(this->readRegister(0x01, 0)); }

    bool HumidityOffsetIndex (uint8_t value) const { return this->writeRegister(0x02, (value) & 0xff); }
    uint8_t HumidityOffsetIndex () const { return (uint8_t)(this->readRegister(0x02, 0)); }

    void defaults () {
      clear();
      TemperatureOffsetIndex(7);
      HumidityOffsetIndex(5);
      sendIntervall(180);
    }
};

#ifdef USE_LIPO
DEFREGISTER(Reg1, CREG_AES_ACTIVE, CREG_MSGFORPOS, CREG_EVENTDELAYTIME, CREG_LEDONTIME)
class CRGSCList1 : public RegList1<Reg1> {
  public:
    CRGSCList1 (uint16_t addr) : RegList1<Reg1>(addr) {}
    void defaults () {
      clear();
      msgForPosA(1);
      msgForPosB(2);
      aesActive(false);
      eventDelaytime(0);
      ledOntime(100);
    }
};

class CRGPosition : public virtual Sensor {
public:
  enum State { NoPos=0, PosA, PosB, PosC };
protected:
  uint8_t  _position;
public:
  CRGPosition () : _position(NoPos) {}
  uint8_t position () { return _position; }
  uint8_t remap (uint8_t state) { return state; }
  uint32_t interval () { return seconds2ticks(5); }
};

class MyOnePinPosition : public CRGPosition {
private:
  uint8_t sens;
  bool _isCharging;
public:
  MyOnePinPosition () : sens(0), _isCharging(false) { _present = true; }

  void init (uint8_t pin) {
    sens=pin;
  }

  void measure (__attribute__((unused)) bool async=false) {
    _isCharging = ( AskSinBase::readPin(sens) == 0);
    static bool lastState=false;
    _position = ( _isCharging == true ) ? State::PosB : State::PosA;
    if (_isCharging != lastState) {
      if (_isCharging) {
        Display.showBatterySymbol(DisplayType::BS_CHARGING);
      } else {
        Display.setNextScreen(SCREEN_TEMPERATURE, true);
        hal.battery.resetCurrent();
      }
      lastState = _isCharging;
    }
  }

  bool isCharging() {
    return _isCharging;
  }
};

class CRGSCChannel : public StateGenericChannel<MyOnePinPosition,Hal,HBList0,CRGSCList1,DefList4,PEERS_PER_CHANNEL> {
public:
  typedef StateGenericChannel<MyOnePinPosition,Hal,HBList0,CRGSCList1,DefList4,PEERS_PER_CHANNEL> BaseChannel;
  CRGSCChannel () : BaseChannel() {};
  ~CRGSCChannel () {}

  void init (uint8_t pin) {
    BaseChannel::init();
    BaseChannel::possens.init(pin);
  }

  bool isCharging() {
    return possens.isCharging();
  }
};
#endif


class ConfigChannel : public RemoteChannel<Hal,PEERS_PER_CHANNEL,HBList0, RCEPList1>  {
public:
  ConfigChannel () : RemoteChannel()  {}
    virtual ~ConfigChannel () {}

    virtual void state(uint8_t s) {
      if (Display.currentScreen() == SCREEN_KEYLABELS) {
        Display.setNextScreen(SCREEN_KEYLABELS, false);
      }
      RemoteChannel::state(s);
    }

    void configChanged() {
      RemoteChannel::configChanged();

      uint8_t idx = number() -1;

      String ht = this->getList1().headerText();
      String mt1 = this->getList1().mainText1();
      String mt2 = this->getList1().mainText2();

      ht.trim();
      mt1.trim();
      mt2.trim();

      typedef struct rmap { char in; String out; } rmap_t;
      const rmap_t rplmap[] = { { '{', "ä" }, { '|', "ö" }, { '}', "ü" }, { '[', "Ä" }, { '#', "Ö" }, { '$', "Ü" }, { '~', "ß" }, { '\'', "=" } };
      for (uint8_t i = 0; i < 8; i++) {
         ht.replace(String(rplmap[i].in), rplmap[i].out);
         mt1.replace(String(rplmap[i].in), rplmap[i].out);
         mt2.replace(String(rplmap[i].in), rplmap[i].out);
      }

      DisplayFields[idx].showHeader = this->getList1().showHeader();
      DisplayFields[idx].HeaderText = ht;
      DisplayFields[idx].MainText1 = mt1;
      DisplayFields[idx].MainText2 = mt2;


      //DPRINT(F("RC (#"));DDEC(number());DPRINT(F(") showHeader : "));DDECLN(this->getList1().showHeader());
      //DPRINT(F("RC (#"));DDEC(number());DPRINT(F(") HeaderText : "));DPRINTLN(ht);
      //DPRINT(F("RC (#"));DDEC(number());DPRINT(F(") MainText1  : "));DPRINTLN(mt1);
      //DPRINT(F("RC (#"));DDEC(number());DPRINT(F(") MainText2  : "));DPRINTLN(mt2);
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

      temp += -35+5*this->getList1().TemperatureOffsetIndex();
      humidity += -5 + this->getList1().HumidityOffsetIndex();

      DPRINT(F("T/H = "));DDEC(temp);DPRINT(F("/"));DDECLN(humidity);
    }

    virtual void trigger (__attribute__ ((unused)) AlarmClock& clock) {

      // reactivate for next measure
      uint16_t updCycle = max(10,this->getList1().sendIntervall());
      tick = seconds2ticks(updCycle);
      clock.add(*this);

      DPRINT("Battery ext voltage: "); DDECLN(device().battery().voltageHighRes());
      measure();

      //send message
      msg.init(device().nextcount(), temp, humidity, device().battery().low(), device().battery().current());
      device().getList0().ledMode(false);
      device().broadcastEvent(msg);
      device().getList0().ledMode(true);

      //update display
      Display.setValues(temp, humidity, device().battery().current(), device().getList0().lowBatLimit());
      if (Display.currentScreen() == SCREEN_TEMPERATURE) Display.showTemp();
    }

    void setup(Device<Hal, HBList0>* dev, uint8_t number, uint16_t addr) {
      Channel::setup(dev, number, addr);
      sensor.init();
      sysclock.add(*this);
    }

    uint8_t status () const { return 0; }

    uint8_t flags () const { return 0; }

    virtual void configChanged () {
      DPRINT(F("WC (#"));DDEC(number());DPRINT(F(") TEMP OFFSET IDX : "));DDECLN(this->getList1().TemperatureOffsetIndex());
      DPRINT(F("WC (#"));DDEC(number());DPRINT(F(") HUMI OFFSET IDX : "));DDECLN(this->getList1().HumidityOffsetIndex());
      DPRINT(F("WC (#"));DDEC(number());DPRINT(F(") SEND INTERV     : "));DDECLN(this->getList1().sendIntervall());
    }
};

class MixDevice : public ChannelDevice<Hal, VirtBaseChannel<Hal, HBList0>, DEVICE_CHANNEL_COUNT+ADDITIONAL_CHANNELS, HBList0> {
  public:
#ifdef USE_LIPO
      VirtChannel<Hal, CRGSCChannel, HBList0> crgscChannel;
#endif
      VirtChannel<Hal, WeatherChannel, HBList0> weatherChannel;
      VirtChannel<Hal, ConfigChannel,  HBList0> remChannel[DEVICE_CHANNEL_COUNT];
  public:
    typedef ChannelDevice<Hal, VirtBaseChannel<Hal, HBList0>, DEVICE_CHANNEL_COUNT+ADDITIONAL_CHANNELS, HBList0> DeviceType;
    MixDevice (const DeviceInfo& info, uint16_t addr) : DeviceType(info, addr) {
      DeviceType::registerChannel(weatherChannel, 5);
#ifdef USE_LIPO
      DeviceType::registerChannel(crgscChannel, 6);
#endif
      for (uint8_t i = 0; i < DEVICE_CHANNEL_COUNT; ++i) DeviceType::registerChannel(remChannel[i], i + 1);
    }
    virtual ~MixDevice () {}

    ConfigChannel& btnChannel (uint8_t c) {
      return remChannel[c - 1];
    }

#ifdef USE_LIPO
    CRGSCChannel& crgChannel () {
      return crgscChannel;
    }
#endif

    virtual void configChanged () {
      uint8_t disptimeout = getList0().backOnTime();
      //DPRINT(F("List0 DISP TIMEOUT         : ")); DDECLN(this->getList0().backOnTime());
      Display.setScreenKeysTimeout(disptimeout);

      if (getList0().defaultDisplayMode() != Display.defaultDisplayMode()) {
        uint8_t ddm = getList0().defaultDisplayMode();
        DPRINT(F("List0 DEFAULT DISPLAYMODE  : ")); DDECLN(this->getList0().defaultDisplayMode());
        Display.defaultDisplayMode(ddm);
        if (Display.currentScreen() == SCREEN_TEMPERATURE) {
          Display.set(seconds2ticks(8), sysclock);
        }
      }

      Display.tempHalfDegree(getList0().displayHalfDegree());
      DPRINT(F("List0 SHOW TEMP HALFDEGREE : ")); DDECLN(getList0().displayHalfDegree());


      uint8_t lowbat = getList0().lowBatLimit();
      if( lowbat > 0 ) {
        battery().low(lowbat);
        battery().critical(lowbat - 1); // set critical bat value to "low bat" - 0.1V
      }
      DPRINT(F("List0 LOWBAT               : ")); DDECLN(this->getList0().lowBatLimit());
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
#ifdef USE_LIPO
      if (sdev.crgChannel().isCharging() == false)
#endif
      if (Display.currentScreen() != Screen::SCREEN_KEYLABELS)
        Display.setNextScreen(Screen::SCREEN_KEYLABELS);
    }
    else if( s == ButtonType::longreleased ) {
      sdev.startPairing();
    }
    else if( s == ButtonType::longpressed ) {
      if( old == ButtonType::longpressed ) {
        sdev.reset(); // long pressed again - reset
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
  Display.showInitScreen((char*)serial, sdev.getMasterID() > 0);

  sdev.init(hal);

  hal.battery.init(seconds2ticks(1),sysclock);
  hal.battery.critical(BATTERY_CRITICAL);

  remoteChannelISR(sdev.btnChannel(1), BTN01_PIN);
  remoteChannelISR(sdev.btnChannel(2), BTN02_PIN);
  remoteChannelISR(sdev.btnChannel(3), BTN03_PIN);
  remoteChannelISR(sdev.btnChannel(4), BTN04_PIN);
#ifdef USE_LIPO
  sdev.crgChannel().init(B_CHARGING_PIN);
  sdev.crgChannel().changed(true);
#endif

  buttonISR(cfgBtn, CONFIG_BUTTON_PIN);
  while (hal.battery.current() == 0);
  DPRINT("Battery voltage = ");DDECLN(hal.battery.voltageHighRes());
  sdev.initDone();
}

void loop() {
  bool worked = hal.runready();
  bool poll = sdev.pollRadio();
  if ( worked == false && poll == false ) {
    if (hal.battery.critical()) {
      Display.showBatterySymbol(DisplayType::BS_EMPTY);
#ifndef USE_LIPO
      hal.activity.sleepForever(hal);
#endif
    }
    hal.activity.savePower<Sleep<>>(hal);
  }
}


