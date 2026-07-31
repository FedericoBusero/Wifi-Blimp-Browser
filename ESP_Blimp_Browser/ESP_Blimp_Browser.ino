/*
   Code for controlling an airship or 3-motor hovercraft via Wi-Fi using a browser

   How to use it?
   Add the Wi-Fi network ‘Hover-xxxxxx’ or ‘Blimp-xxxx’ with the password 12345678
   There is, of course, no internet connection on that network, so tick ‘Keep Wi-Fi on’ if promptedOn Android, it is often necessary to switch off mobile data
   Then open your browser (Chrome, Firefox, Safari, etc.) and go to the website http://h.be or 192.168.4.1

   The top line shows the connection status. On the ESP8266 and masynmachien boards, the voltage is displayed during the connection; this can be calibrated using VOLTAGE_FACTOR
   The top slider is used to control either the maximum power of the thrust motors or the sensitivity of the gyro (p-factor)
   The slider below it adjusts the hover/elevator motor.
   The joystick is used to control the two thrust motors

*/

#include <ArduinoWebsockets.h> // from arduino library manager : "ArduinoWebsockets" by Gil Maimon, https://github.com/gilmaimon/ArduinoWebsockets
#include "config.h"

#ifdef USE_FASTIMU
#include "FastIMU.h" // library; https://github.com/LiquidCGS/FastIMU minimum versie 1.2.8 voor LSM6DS3TR-C
#include "lowpass_filter.h"
FASTIMU_TYPE imu;
#endif

// Architecture dependent settings
#if defined(CONFIG_IDF_TARGET_ESP32C3)

#include <ESPAsyncWebSrv.h> // ESPAsyncWebSrv, version 1.2.6 by dvarrel : https://github.com/dvarrel/ESPAsyncWebSrv/
#include <WiFi.h>
#include <AsyncTCP.h> // https://github.com/me-no-dev/AsyncTCP

#define PWM_RANGE 255 // PWM range for analogWrite
#define MOTOR_MINSPEED 2

#elif defined(ARDUINO_ARCH_ESP32)

#include <ESPAsyncWebServer.h> // https://github.com/me-no-dev/ESPAsyncWebServer
#include <WiFi.h>
#include <AsyncTCP.h> // https://github.com/me-no-dev/AsyncTCP

#define PWM_RANGE 255 // PWM range for analogWrite
#define MOTOR_MINSPEED 0

#else // ESP8266
ADC_MODE(ADC_VCC); // Needed for reading the voltage with ESP.getVcc

#include <ESPAsyncWebServer.h> // https://github.com/me-no-dev/ESPAsyncWebServer, on ESP8266: install https://github.com/me-no-dev/ESPAsyncTCP
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h> // https://github.com/me-no-dev/ESPAsyncTCP

#define PWM_RANGE 1023 // PWM range for analogWrite
#define MOTOR_MINSPEED 0

#endif

#define USE_SOFTAP
const char ssid[] = WIFI_SOFTAP_SSID_PREFIX;
const char password[] = WIFI_SOFTAP_PASSWORD;

#ifdef USE_SOFTAP
#include <DNSServer.h>
DNSServer dnsServer;
#endif

#include "hovercontrol_html.h" // Do not move this code to the ino file, because it can mix up the preprocessor

using namespace websockets;
WebsocketsServer server;
AsyncWebServer webserver(80);
WebsocketsClient sclient;

// timeoutes
// Timeout to switch of motors for safety, after not receiving anything for x milliseconds
// Should be higher than timeout interval in html function ws_onopen_ping
#define TIMEOUT_MS_MOTORS 1200L
#define TIMEOUT_MS_LED 1L         // number of milliseconds the LED stays on after receiving a message
#define TIMEOUT_MS_STATUS 10000L  //  number of milliseconds between updating status&voltage
#define TIMEOUT_MS_JOYSTICK 2000L //  number of milliseconds after using the joystick, switching of L&R motors

unsigned long last_activity_message;

#include "Easer.h"

int ui_joystick_x = 0;
int ui_joystick_y = 0;
int ui_slider1 = 0; // -180 .. 180
int ui_slider2 = 0; // 0 .. 360

#define MOTOR_FREQ 512 // Frequention for analogWrite in Hz, determines the sound of the motor

Easer motorZ_snelheid;
bool motors_halt;

class hbridge
{
public:
  hbridge(int _pin1, int _pin2)
      : pin1(_pin1), pin2(_pin2), currentspeed(0)
  {
  }

  void setSpeed(long motorspeed, long min_speed = 0)
  {
    if (abs(motorspeed) < min_speed)
    {
      motorspeed = 0;
    }

    if (motorspeed == currentspeed) return;

    if (motorspeed >= 0)
    {
      analogWrite(pin1, motorspeed);
      analogWrite(pin2, 0);
    }
    else
    {
      analogWrite(pin1, 0);
      analogWrite(pin2, -motorspeed);
    }
    currentspeed = motorspeed;
  }

  void halt()
  {
    analogWrite(pin1, 0);
    analogWrite(pin2, 0);
    currentspeed = 0;
  }

private:
  int pin1, pin2;
  int currentspeed;
};

#ifdef USE_CONFIG_BLIMP2Z
hbridge motorZ(PIN_1ZMOTOR, PIN_2ZMOTOR);
#endif
hbridge motorA(PIN_1AMOTOR, PIN_2AMOTOR);
hbridge motorB(PIN_1BMOTOR, PIN_2BMOTOR);

bool gyroBeschikbaar = false;
bool collision = false; // for collision detection

#ifdef USE_WS2812FX
#include <WS2812FX.h> // https://github.com/kitesurfer1404/WS2812FX
WS2812FX ws2812fx = WS2812FX(WS2812FX_NUMLEDS, PIN_WS2812FX, WS2812FX_RGB_ORDER + NEO_KHZ800);
#endif

void setup_pin_mode_output(int pin)
{
#ifdef ESP8266
  if ((pin == 1) || (pin == 3)) // RX & TX
  {
    pinMode(pin, FUNCTION_3);
  }
#endif
  pinMode(pin, OUTPUT);
}

#ifdef USE_FASTIMU
float getGyro()
{
  static LowPassFilter lpf(GYRO_LPF_TF);
  static unsigned long lastupdate_gyro = 0;

  unsigned long currentmillis = millis();
  if (currentmillis > lastupdate_gyro + 1) // minimum 1 ms between calling gyro
  {
    lastupdate_gyro = currentmillis;
    GyroData gyroData;
    float measured_value = 0.0;

    imu.update();
    imu.getGyro(&gyroData);
    switch (GYRO_DIRECTION)
    {
    case GYRO_DIRECTION_X:
      measured_value = gyroData.gyroX;
      break;

    case GYRO_DIRECTION_Y:
      measured_value = gyroData.gyroY;
      break;

    case GYRO_DIRECTION_Z:
      measured_value = gyroData.gyroZ;
      break;
    }
  #ifdef GYRO_FLIP
    measured_value = -measured_value;
  #endif

    float accelX = 0; // TODO
    float accelY = 0; // TODO
    float accelZ = 0; // TODO
    collision = (sq(accelX) + sq(accelY) + sq(accelZ)) > ACCELERATION_THRESHOLD; // VOOR BOTSDETECTIE

    return lpf(measured_value);
  }
  else
  {
    return lpf.getLastValue();
  }
}
#endif

float mapFloat(float value, float fromLow, float fromHigh, float toLow, float toHigh)
{
  return (value - fromLow) * (toHigh - toLow) / (fromHigh - fromLow) + toLow;
}

void updateMotors()
{
  static unsigned long last_activity_joystick = 0;

  if (motors_halt)
  {
#ifdef USE_CONFIG_BLIMP2Z
    motorZ.halt();
#else
    analogWrite(PIN_ZMOTOR, 0);
#endif
    motorA.halt();
    motorB.halt();
  }
  else
  {
    float regelX = 0.0;
    const float max_draai_factor = GYRO_REGELING_MAX_DRAAI;
#ifdef XY_MOTOR_LIMIT_SLIDER
    float xy_motor_limit = mapFloat((float)ui_slider1, -180.0, 180.0, 0.2 * XY_MOTOR_MAX, XY_MOTOR_MAX);
#else
    float xy_motor_limit = XY_MOTOR_MAX;
#endif

    if (gyroBeschikbaar) // gyro
    {
#ifdef USE_FASTIMU
      // "gyro"-control

#ifdef XY_MOTOR_LIMIT_SLIDER
      float Pfactor = GYRO_REGELING_MAX_P;
#else
      float Pfactor = mapFloat((float)ui_slider1, -180.0, 180.0, 0.0, GYRO_REGELING_MAX_P);
#endif
      const float bias = GYRO_REGELING_BIAS;

      float werkelijke_draaisnelheid = getGyro();

      // steering in proportion to deviation. The X value from the joystick determines how fast we want to turn

      float doel_draaisnelheid = (float)ui_joystick_x * (-1.0) * max_draai_factor;
      regelX = Pfactor * (werkelijke_draaisnelheid - doel_draaisnelheid) - bias * doel_draaisnelheid;
      regelX = constrain(regelX, -180.0, 180.0);
#endif
    }
    else
    {
      regelX = (1.0) * (float)(ui_joystick_x)*max_draai_factor;
    }

#ifdef USE_CONFIG_BLIMP2Z
    int doel_motorZsnelheid = map(ui_slider2, 0, 360, -PWM_RANGE, PWM_RANGE);
#else
    int doel_motorZsnelheid = map(ui_slider2, 0, 360, 0, PWM_RANGE); // for hover motor
#endif
    if ((abs(ui_joystick_y)+1) * (abs(ui_joystick_x)+1) >= 5)    {
      last_activity_joystick = millis();
    }
    else
    {
#ifdef USE_CONFIG_HOVER3M
      doel_motorZsnelheid = 0; // When the joystick is centered, the hover motor is also switched off
#endif
    }
    if (millis() > last_activity_joystick + TIMEOUT_MS_JOYSTICK)
    {
      regelX = 0;
    }

#ifdef DEBUG_SERIAL
    //      DEBUG_SERIAL.print("  millis() ");
    //      DEBUG_SERIAL.println(millis());
    // DEBUG_SERIAL.print("  ui_joystick_x ");
    // DEBUG_SERIAL.println(ui_joystick_x);
    //      DEBUG_SERIAL.print("  ui_joystick_x ");
    //      DEBUG_SERIAL.println(ui_joystick_x);
    //      DEBUG_SERIAL.print("  Pfactor: ");
    //      DEBUG_SERIAL.print(Pfactor);
    // DEBUG_SERIAL.print("  regelX: ");
    // DEBUG_SERIAL.println(regelX);
#endif

    // converting x and y to motorspeed
    float ui_joystick_y_constrain = constrain((float)ui_joystick_y, -(180-fabsf(regelX)), 180-fabsf(regelX)); // maximal stearing also at full forward or backward
    float temp1 = constrain((float) ui_joystick_y_constrain + regelX, -180, 180);
    float temp2 = constrain((float) ui_joystick_y_constrain - regelX, -180, 180);

    float motorsnelheidA = mapFloat(-temp2, -180.0, 180.0, -(float)PWM_RANGE * xy_motor_limit, (float)PWM_RANGE * xy_motor_limit);
    float motorsnelheidB = mapFloat(-temp1, -180.0, 180.0, -(float)PWM_RANGE * xy_motor_limit, (float)PWM_RANGE * xy_motor_limit);

    motorA.setSpeed((long)motorsnelheidA, MOTOR_MINSPEED);
    motorB.setSpeed((long)motorsnelheidB, MOTOR_MINSPEED);

    motorZ_snelheid.easeTo(doel_motorZsnelheid);
    motorZ_snelheid.update();
#ifdef USE_CONFIG_BLIMP2Z
    motorZ.setSpeed(motorZ_snelheid.getCurrentValue(), MOTORZ_MINSPEED);
#else
    analogWrite(PIN_ZMOTOR, motorZ_snelheid.getCurrentValue()); // We adapt the motor speed to its new speed motorZ_snelheid
#endif

#ifdef DEBUG_SERIAL
    //   DEBUG_SERIAL.print(F("temp1 "));
    //   DEBUG_SERIAL.print(temp1);
    //   DEBUG_SERIAL.print(F("temp2 "));
    //   DEBUG_SERIAL.println(temp2);
    // DEBUG_SERIAL.print(F("motorsnelheid A="));
    // DEBUG_SERIAL.print(motorsnelheidA);
    // DEBUG_SERIAL.print(F(" B="));
    // DEBUG_SERIAL.println(motorsnelheidB);
#endif
  }
}

void motors_pause()
{
#ifdef DEBUG_SERIAL
  DEBUG_SERIAL.println(F("motors_pause"));
#endif

  motors_halt = true;
  updateMotors();
}

void motors_resume()
{
#ifdef DEBUG_SERIAL
  DEBUG_SERIAL.println(F("motors_resume"));
#endif
  motors_halt = false;
  updateMotors();
}

void init_motors()
{
  ui_slider1 = 0;
#ifdef USE_CONFIG_BLIMP2Z
  ui_slider2 = 180;
#else
  ui_slider2 = 0;
#endif
  ui_joystick_x = 0;
  ui_joystick_y = 0;
  motorZ_snelheid.setValue(0);
  motors_halt = false;

  updateMotors();
}

void led_init()
{
#ifdef PIN_LEDCONNECTIE
  setup_pin_mode_output(PIN_LEDCONNECTIE);
#endif
}

void led_set(int ledmode, boolean except_when_dual_use)
{
#ifdef PIN_LED_DUALUSE
  if (except_when_dual_use)
    return;
#endif
#ifdef PIN_LEDCONNECTIE
  digitalWrite(PIN_LEDCONNECTIE, ledmode);
#endif
}

void init_voltage_monitor()
{
#if defined(ESP32) && defined(PIN_BATMONITOR)
  analogSetAttenuation(ADC_0db); // On the ESP32 based masynmachien boards, we use an external resistor bridge to measure the battery voltage and set the internal resistor bridge to ‘no voltage division’
#endif
}

float getVoltage()
{
#ifdef ESP8266
  return (float)ESP.getVcc() / (float)VOLTAGE_FACTOR; // On ESP8266 modules, VCC is connected to the single ADC pins
#elif defined(ESP32) && defined(PIN_BATMONITOR) && defined(VOLTAGE_FACTOR)
  return (float)analogRead(PIN_BATMONITOR) / (float)VOLTAGE_FACTOR; // On ESP32 modules, VBAT can be connected to an ADC1 pin via a voltage divider (do not use ADC2)
#else
  return (float)0;
#endif
}

void collision_effect() // for collision detection (TODO)
{
#ifdef USE_WS2812FX
  static unsigned long last_collision_effect = 0;
  unsigned long currentmillis = millis();

  if (collision)
  {
    ws2812fx.setColor(WS2812FX_COLLISION);
    last_collision_effect = currentmillis;
  }
  else
  {
    if (currentmillis > last_collision_effect + TIMEOUT_MS_COLLISION) // longer than TIMEOUT_MS_COLLISION ago that a collision was detected
    {
      ws2812fx.setColor(WS2812FX_COLOR);
    }
  }
#endif
  
}

void setup()
{
  setup_pin_mode_output(PIN_1AMOTOR);
  setup_pin_mode_output(PIN_2AMOTOR);
  setup_pin_mode_output(PIN_1BMOTOR);
  setup_pin_mode_output(PIN_2BMOTOR);
#ifdef USE_CONFIG_BLIMP2Z
  setup_pin_mode_output(PIN_1ZMOTOR);
  setup_pin_mode_output(PIN_2ZMOTOR);
#else
  setup_pin_mode_output(PIN_ZMOTOR);
#endif

#ifdef ESP8266
  // As the PWM range of `analogWrite` is either 255 or 1023, depending on the version of the Arduino ESP8266, we set the range to 1023
  analogWriteRange(PWM_RANGE);

  // Change the frequency of `analogWrite` from 1000 Hz to 400 Hz for a more pleasant motor sound
  analogWriteFreq(MOTOR_FREQ);
#if ARDUINO_ESP8266_MAJOR >= 3
  // workaround for extreme slow servo write from Arduino 3 on: https://github.com/esp8266/Arduino/issues/8081
  enablePhaseLockedWaveform();
#endif
#elif defined(ESP32)
  // Change the frequency of `analogWrite` from 1000 Hz to 400 Hz for a more pleasant motor sound
  analogWriteFrequency(MOTOR_FREQ);
#endif
#ifdef USE_CONFIG_BLIMP2Z
  motorZ.halt();
#else
  analogWrite(PIN_ZMOTOR, 0);
#endif

  motorA.halt();
  motorB.halt();

  delay(200); // waiting 200 milliseconds till the current is stable

#ifdef DEBUG_SERIAL
  delay(1000);
  DEBUG_SERIAL.begin(115200);
  DEBUG_SERIAL.println(F("\nHover Browser setup started"));
#endif

  led_init();

  // LED flashes 2 times to indicate a reboot
  led_set(LED_BRIGHTNESS_ON, false);
  delay(10);
  led_set(LED_BRIGHTNESS_OFF, false);
  delay(100);
  led_set(LED_BRIGHTNESS_ON, false);
  delay(10);
  led_set(LED_BRIGHTNESS_OFF, false);

#ifdef USE_CONFIG_BLIMP2Z
  motorZ_snelheid.begin(0, true);
#else
  motorZ_snelheid.begin(0, false);
#endif
  motorZ_snelheid.set_speed((float)MOTORZ_TIME_UP / (float)PWM_RANGE);

  init_motors();

  gyroBeschikbaar = false;

#ifdef USE_FASTIMU
  // setup gyro module
#ifdef PIN_SDA
  Wire.begin(PIN_SDA, PIN_SCL);
#else
  Wire.begin();
#endif
  Wire.setClock(400000); //400khz clock
  delay(100);
  for (int t = 0; t < 3; t++) // try 3 times if the gyro is available
  {
    calData calib = { 0 };  //Calibration data
    int err = imu.init(calib, IMU_I2C_ADDRESS);
    if (err != 0)
    {
#ifdef DEBUG_SERIAL
      DEBUG_SERIAL.print(millis());
      DEBUG_SERIAL.println("\tCould not connect to gyro");
#endif
      delay(1000);
    }
    else
    {
      gyroBeschikbaar = true;
      break;
    }
  }

  if (gyroBeschikbaar)
  {
    imu.setGyroRange(500);
    imu.setAccelRange(8);

#ifdef DEBUG_SERIAL
    DEBUG_SERIAL.println("start...");
#endif

    // set all calibration errors to zero
    // TODO
  }
#endif // USE_FASTIMU
#ifdef PIN_LED_DUALUSE
  led_init();
#endif

  // Wifi settings
  WiFi.persistent(true);

  uint8_t macAddr[6];
  WiFi.macAddress(macAddr);

#if defined(USE_SOFTAP)
  WiFi.disconnect();
  /* set up an access point */
  WiFi.mode(WIFI_AP);

  // ssidmac = ssid + 4 last hexadecimal values of the MAC-address
  char ssidmac[33];
  sprintf(ssidmac, "%s%02X%02X", ssid, macAddr[4], macAddr[5]);
  WiFi.softAP(ssidmac, password, WIFI_SOFTAP_CHANNEL);
  IPAddress apIP = WiFi.softAPIP();
#ifdef DEBUG_SERIAL
  DEBUG_SERIAL.print(F("SoftAP SSID="));
  DEBUG_SERIAL.println(ssidmac);
  DEBUG_SERIAL.print(F("IP: "));
  DEBUG_SERIAL.println(apIP);
#endif
  /* set up DNS server translating all domains to apIP */
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(53, "h.be", apIP);

#else
  WiFi.softAPdisconnect(true);
  // host_name = "Hover-" + 6 hexadecimal values of  van het MAC-adres
  char host_name[33];
  sprintf(host_name, "Hover-%02X%02X%02X", macAddr[3], macAddr[4], macAddr[5]);
#ifdef DEBUG_SERIAL
  DEBUG_SERIAL.print(F("Hostname: "));
  DEBUG_SERIAL.println(host_name);
#endif
#ifdef ESP8266
  WiFi.hostname(host_name);
#else // ESP32
  WiFi.setHostname(host_name);
#endif

  // Connect to wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Waiting a moment for connection with the Wi-Fi network
  for (int i = 0; i < 15 && WiFi.status() != WL_CONNECTED; i++)
  {
#ifdef DEBUG_SERIAL
    DEBUG_SERIAL.print('.');
#endif
    delay(1000);
  }

#ifdef DEBUG_SERIAL
  DEBUG_SERIAL.print("\nWiFi connected - IP address: ");
  DEBUG_SERIAL.println(WiFi.localIP());
#endif

#endif

  webserver.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
               {
#ifdef DEBUG_SERIAL
    DEBUG_SERIAL.println(F("on HTTP_GET: return"));
#endif
    request->send(200, "text/html", index_html); });

  webserver.begin();
  server.listen(82);
#ifdef DEBUG_SERIAL
  DEBUG_SERIAL.print(F("Is server live? "));
  DEBUG_SERIAL.println(server.available());
#endif

#ifdef USE_WS2812FX
  ws2812fx.init();
  ws2812fx.setBrightness(WS2812FX_BRIGHTNESS);
  ws2812fx.setSpeed(WS2812FX_SPEED);
  ws2812fx.setColor(WS2812FX_COLOR);
  ws2812fx.setMode(WS2812FX_MODE);
  ws2812fx.start();
#endif

  init_voltage_monitor();

  last_activity_message = millis();
}

void handle_message(websockets::WebsocketsMessage msg)
{
  const char *msgstr = msg.c_str();
  const char *p;

#ifdef DEBUG_SERIAL
  //  DEBUG_SERIAL.println();
  //  DEBUG_SERIAL.print(F("handle_message "));
#endif

  int id = atoi(msgstr);
  int param1 = 0;
  int param2 = 0;

  p = strchr(msgstr, ':');
  if (p)
  {
    param1 = atoi(++p);
    p = strchr(p, ',');
    if (p)
    {
      param2 = atoi(++p);
    }
  }

#ifdef DEBUG_SERIAL
  //  DEBUG_SERIAL.println(msgstr);
  //  DEBUG_SERIAL.print(F(" id = "));
  //  DEBUG_SERIAL.print(id);
  //  DEBUG_SERIAL.print(F(" param1 = "));
  //  DEBUG_SERIAL.print(param1);
  //  DEBUG_SERIAL.print(F(" param2 = "));
  //  DEBUG_SERIAL.println(param2);
#endif
  led_set(LED_BRIGHTNESS_ON, true);
  last_activity_message = millis();

  switch (id)
  {
  case 0: // ping
    break;

  case 1: // joystick
    ui_joystick_x = param1;
    ui_joystick_y = param2;
    updateMotors();
    break;

  case 2: // slider2
    ui_slider2 = param1;
    updateMotors();
    break;

  case 3: // slider1
    ui_slider1 = param1;
    updateMotors();
    break;
  }
  if (motors_halt)
  {
    motors_resume();
  }
}

void onConnect()
{
#ifdef PIN_LED_DUALUSE
  digitalWrite(PIN_LEDCONNECTIE, LOW);
#else
  led_set(LED_BRIGHTNESS_OFF, false);
#endif
#ifdef DEBUG_SERIAL
  DEBUG_SERIAL.println(F("onConnect"));
#endif
  init_motors();
}

void onDisconnect()
{
#ifdef DEBUG_SERIAL
  DEBUG_SERIAL.println(F("onDisconnect"));
#endif
  init_motors();
#ifdef PIN_LED_DUALUSE
  led_init();
#endif
}

void updatestatusbar()
{
#if defined(ESP8266) or ((defined(ESP32)) && defined(PIN_BATMONITOR))
  static unsigned long lastupdate_voltage = 0;
  unsigned long currentmillis = millis();
  char statusstr[50];

  if (currentmillis > lastupdate_voltage + TIMEOUT_MS_STATUS)
  {
    lastupdate_voltage = currentmillis;
    float voltage = getVoltage();

    if (voltage >= VOLTAGE_THRESHOLD)
    {
      if (gyroBeschikbaar)
      {
#ifdef USE_FASTIMU
        snprintf(statusstr, sizeof(statusstr), "%4.2f V gyro:%4.2f", voltage, getGyro());
#endif
      }
      else
      {
        snprintf(statusstr, sizeof(statusstr), "%4.2f V", voltage);
      }
#ifdef DEBUG_SERIAL
      DEBUG_SERIAL.print("Sending status: ");
      DEBUG_SERIAL.println(statusstr);
#endif
      sclient.send(statusstr);
    }
    else
    {
      snprintf(statusstr, sizeof(statusstr), "Battery low: %4.2f V. Shutting down", voltage);
#ifdef DEBUG_SERIAL
      DEBUG_SERIAL.print("Sending status: ");
      DEBUG_SERIAL.println(statusstr);
#endif
      sclient.send(statusstr);
      motors_pause();
      delay(20000); // message is shown for 20 seconds before disconnecting
      WiFi.mode(WIFI_OFF);
#ifdef ESP8266
      WiFi.forceSleepBegin();
#endif
      delay(1);
      while (1)
      {
        led_set(LED_BRIGHTNESS_ON, false);
        delay(10);
        led_set(LED_BRIGHTNESS_OFF, false);
        delay(5000);
      }
    }
  }
#else
  static unsigned long lastupdate_status = 0;
  unsigned long currentmillis = millis();
  char statusstr[50];

  if (currentmillis > lastupdate_status + TIMEOUT_MS_STATUS)
  {
    lastupdate_status = currentmillis;

    if (gyroBeschikbaar)
    {
#ifdef USE_FASTIMU
      snprintf(statusstr, sizeof(statusstr), "gyro:%4.2f", getGyro());
#endif
    }
    else
    {
      snprintf(statusstr, sizeof(statusstr), "");
    }
#ifdef DEBUG_SERIAL
    DEBUG_SERIAL.print("Sending status: ");
    DEBUG_SERIAL.println(statusstr);
#endif
    sclient.send(statusstr);
  }
#endif
}

void loop()
{
  static int is_connected = 0;

#if defined(USE_SOFTAP)
  dnsServer.processNextRequest();
#endif

  if (millis() > last_activity_message + TIMEOUT_MS_LED)
  {
    led_set(LED_BRIGHTNESS_OFF, true);
  }

  if (millis() > last_activity_message + TIMEOUT_MS_MOTORS)
  {

#ifdef DEBUG_SERIAL
    DEBUG_SERIAL.println(F("Safety shutdown ..."));
#endif
    motors_pause();

    last_activity_message = millis();
  }

  if (is_connected)
  {
    if (sclient.available())
    {                 // if return is non-zero, a client is connected
      sclient.poll(); // if return is non-zero, something was received

      updatestatusbar();

#ifdef USE_FASTIMU
      getGyro(); // update low pass filter gyro
#endif
      static unsigned long lastupdate_motors = 0;
      unsigned long currentmillis = millis();
      if (currentmillis > lastupdate_motors + 10) // minimum 10 ms between calling updatemotors if no new value is received from the browser
      {
        lastupdate_motors = currentmillis;
        updateMotors();
        collision_effect();
      }
    }
    else
    {
      // no longer connected
      onDisconnect();
      is_connected = 0;
    }
  }
  if (server.poll()) // if a new socket is requested
  {
#ifdef DEBUG_SERIAL
    DEBUG_SERIAL.print(F("server.poll is_connected="));
    DEBUG_SERIAL.println(is_connected);
#endif
    if (is_connected)
    {
      sclient.send("CLOSE");
    }

    sclient = server.accept();
#ifdef DEBUG_SERIAL
    DEBUG_SERIAL.println(F("Connection accept"));
#endif
    sclient.onMessage(handle_message);

    onConnect();
    is_connected = 1;
  }

  if (!is_connected)
  {
    led_set((millis() % 1000) > 500 ? LED_BRIGHTNESS_OFF : LED_BRIGHTNESS_ON, false);
  }
#ifdef USE_WS2812FX
  else
  {
    ws2812fx.service();
  }
#endif

  // delay(2);
}
