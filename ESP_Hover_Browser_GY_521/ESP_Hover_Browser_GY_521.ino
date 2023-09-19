/*
   Code voor het besturen van een hover mbv Wifi via de browser

   Hoe gebruiken?
   Voeg wifi netwerk hover-xxxx toe met paswoord 12345678
   Er is op dat netwerk uiteraard geen internet, dus "wifi behouden" aanvinken indien dat gevraagd wordt
   Dan ga je naar de browser (chrome, firefox, safari, ..) naar de website 192.168.4.1 of http://h.be

   De bovenste regel toont de connectie-status. Op ESP8266 wordt het voltage getoond tijdens de connectie, te calibreren met VOLTAGE_FACTOR
   De bovenste slider wordt gebruikt om de gevoeligheid van de gyro te regelen (p-factor)
   De slider eronder stelt de zweefmotor in.
   De derde slider stelt de maximale draaisnelheid in
   Met de joystick worden 2 stuwmotoren bestuurd

*/

#include <ArduinoWebsockets.h> // uit arduino library manager : "ArduinoWebsockets" by Gil Maimon, https://github.com/gilmaimon/ArduinoWebsockets
#include <ESPAsyncWebServer.h> // https://github.com/me-no-dev/ESPAsyncWebServer
// ESP8266: https://github.com/me-no-dev/ESPAsyncTCP installeren

#include "GY521.h" // library; https://github.com/RobTillaart/GY521/
/*
   Wemos D1 mini:
   SCL: D1
   SDA: D2
   XDA: niet aangesloten
   XCL: niet aangesloten
   AD0: niet aangesloten  . De vice heeft 0x68 als I2C adres, waarschijnlijk wordt het 0x69 als je dit naar 3.3V verhoogt
   INT: niet aangesloten
   VCC: 3V
   GND: uiteraard
*/

GY521 sensor(0x68);

#ifdef CONFIG_IDF_TARGET_ESP32S2
#include <WiFi.h>
#include <AsyncTCP.h> // https://github.com/me-no-dev/AsyncTCP

#define DEBUG_SERIAL Serial

#define PWM_RANGE 255 // PWM range voor analogWrite

// Lolin ESP32-S2
#define PIN_1AMOTOR          12 // Positie D8 op Wemos D1 mini
#define PIN_2AMOTOR          11 // Positie D7 op Wemos D1 mini
#define PIN_1BMOTOR          9  // Positie D6 op Wemos D1 mini
#define PIN_2BMOTOR          5  // Positie D0 op Wemos D1 mini
#define PIN_ZMOTOR           18 // Positie D3 op Wemos D1 mini
#define PIN_LEDCONNECTIE     15 // De ingebouwde LED 

#define LED_BRIGHTNESS_ON  HIGH
#define LED_BRIGHTNESS_OFF LOW

#else // ESP8266

ADC_MODE(ADC_VCC); // Nodig voor het inlezen van het voltage met ESP.getVcc

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h> // https://github.com/me-no-dev/ESPAsyncTCP

#define PWM_RANGE 1023 // PWM range voor analogWrite
#define MOTOR_FREQ 400 // Frequentie van analogWrite in Hz, bepaalt het geluid van de motor

#define DEBUG_SERIAL Serial

#define PIN_1AMOTOR          D8 // D8 = GPIO15 op NodeMCU & Wemos D1 mini
#define PIN_2AMOTOR          D7 // D8 = GPIO15 op NodeMCU & Wemos D1 mini
#define PIN_1BMOTOR          D6 // D8 = GPIO15 op NodeMCU & Wemos D1 mini
#define PIN_2BMOTOR          D0 // D0 = GPIO16 op NodeMCU & Wemos D1 mini
#define PIN_ZMOTOR           D3 // D8 = GPIO15 op NodeMCU & Wemos D1 mini
#define PIN_LEDCONNECTIE     2 // De ingebouwde LED zit op GPIO2 of GPIO16, dus aanpassen naar 16 als de LED niet werkt

// Pas de voltagefactor aan, dat is bij elke chip hetzelfde. Kalibreer bv. met USB stroom die 3.3V op de chip moet geven
#define VOLTAGE_FACTOR 910.0f
#define VOLTAGE_THRESHOLD 2.4 // onder dit voltage valt de chip uit om de batterij te beschermen

#define LED_BRIGHTNESS_ON  LOW
#define LED_BRIGHTNESS_OFF HIGH

#endif // CONFIG_IDF_TARGET_ESP32S2

#define USE_SOFTAP
#define WIFI_SOFTAP_CHANNEL 1 // 1-13
const char ssid[] = "HoverG-";
const char password[] = "12345678";

#ifdef USE_SOFTAP
#include <DNSServer.h>
DNSServer dnsServer;
#endif

#include "hovercontrol_html.h" // Deze code niet verplaatsen naar de ino file, want de preprocessor kan die overhoop halen

using namespace websockets;
WebsocketsServer server;
AsyncWebServer webserver(80);
WebsocketsClient sclient;

// timeoutes
#define TIMEOUT_MS_MOTORS 2500L // Timeout om motoren uit veiligheid stil te leggen, na x milliseconden niks te hebben ontvangen
#define TIMEOUT_MS_LED 1L        // Aantal milliseconden dat LED blijft branden na het ontvangen van een boodschap
#define TIMEOUT_MS_VOLTAGE 10000L // Aantal milliseconden tussen update voltage

unsigned long last_activity_message;

int ui_slider1; // -180 .. 180
int ui_slider2 = 0; // 0 .. 360
int ui_slider3 = 0; // 0 .. 200
int ui_joystick_x = 0;
int ui_joystick_y = 0;

bool gyroBeschikbaar = false;
int max_motorsnelheid;
bool motors_halt;

void setup_pin_mode_output(int pin)
{
#if defined(ESP8266)
  if ((pin == 1) || (pin == 3)) // RX & TX
  {
    pinMode (pin, FUNCTION_3);
  }
#endif
  pinMode(pin, OUTPUT);
}

void hbridge_setspeed(int pin1, int pin2, long motorspeed)
{
  if (motorspeed >= 0)
  {
    digitalWrite(pin1, HIGH);
    analogWrite(pin2, PWM_RANGE - motorspeed);
  }
  else
  {
    digitalWrite(pin1, LOW);
    analogWrite(pin2, -motorspeed);
  }
}

void updateMotors()
{
  if (motors_halt)
  {
    analogWrite(PIN_ZMOTOR, 0);
    analogWrite(PIN_1AMOTOR, 0);
    analogWrite(PIN_2AMOTOR, 0);
    analogWrite(PIN_1BMOTOR, 0);
    analogWrite(PIN_2BMOTOR, 0);
  }
  else
  {
    float regelX;
    float max_draai_factor = (float)ui_slider3 / 100.0; // ui_slider3 : 0 .. 200 

    if (gyroBeschikbaar) // gyro
    {
      // "gyro"-regeling
      float Pfactor = ((float)ui_slider1 + 180.0) / 150.0; // aanpassen waarde -180 .. 180 naar 0 .. 2.4

      sensor.read();
      float werkelijke_draaisnelheid = sensor.getGyroZ(); // getGyroX, getGyroY zijn ook mogelijk afhankelijk van positie sensor

      // sturen in verhouding tot afwijking, X van joystick bepaalt hoe snel we willen draaien
      
      float doel_draaisnelheid = (float)ui_joystick_x * (-1.0) * max_draai_factor; 
      regelX = Pfactor * (werkelijke_draaisnelheid-doel_draaisnelheid); 
    }
    else
    {
       regelX = (1.0)*(float)(ui_joystick_x) * max_draai_factor;
    }

    int z_motorsnelheid = map(ui_slider2, 0, 360, 0, PWM_RANGE); // voor zweefmotor
    if (abs(ui_joystick_y * ui_joystick_x) < 5) {
      z_motorsnelheid = 0; // bij joystick los ook zweefmotor uit
      regelX = 0; 
    }

#ifdef DEBUG_SERIAL
    //      DEBUG_SERIAL.print("  millis() ");
    //      DEBUG_SERIAL.println(millis());
    DEBUG_SERIAL.print("  ui_joystick_x ");
    DEBUG_SERIAL.println(ui_joystick_x);
    //      DEBUG_SERIAL.print("  ui_joystick_x ");
    //      DEBUG_SERIAL.println(ui_joystick_x);
    //      DEBUG_SERIAL.print("  Pfactor: ");
    //      DEBUG_SERIAL.print(Pfactor);
    DEBUG_SERIAL.print("  regelX: ");
    DEBUG_SERIAL.println(regelX);
#endif

    // x en y omzetten naar motorsnelheden
    float temp1 = constrain((float)ui_joystick_y + regelX, -180, 180); 
    float temp2 = constrain((float)ui_joystick_y - regelX, -180, 180); 

    int motorsnelheidA = map(-temp2, -180, 180, -max_motorsnelheid, max_motorsnelheid);
    int motorsnelheidB = map(-temp1, -180, 180, -max_motorsnelheid, max_motorsnelheid);

    hbridge_setspeed(PIN_1AMOTOR, PIN_2AMOTOR, motorsnelheidA);
    hbridge_setspeed(PIN_1BMOTOR, PIN_2BMOTOR, motorsnelheidB);
    analogWrite(PIN_ZMOTOR, z_motorsnelheid); // zweefmotor/ z-as motor naar zijn snelheid z_motorsnelheid

#ifdef DEBUG_SERIAL
    //   DEBUG_SERIAL.print(F("temp1 "));
    //   DEBUG_SERIAL.print(temp1);
    //   DEBUG_SERIAL.print(F("temp2 "));
    //   DEBUG_SERIAL.println(temp2);
    DEBUG_SERIAL.print(F("motorsnelheid A="));
    DEBUG_SERIAL.print(motorsnelheidA);
    DEBUG_SERIAL.print(F(" B="));
    DEBUG_SERIAL.println(motorsnelheidB);
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
  ui_slider2 = 0;
  ui_slider3 = 0;
  ui_joystick_x = 0;
  ui_joystick_y = 0;
  max_motorsnelheid = PWM_RANGE;
  motors_halt = false;

  updateMotors();
}


void setup()
{
  setup_pin_mode_output(PIN_1AMOTOR);
  setup_pin_mode_output(PIN_2AMOTOR);
  setup_pin_mode_output(PIN_1BMOTOR);
  setup_pin_mode_output(PIN_2BMOTOR);
  setup_pin_mode_output(PIN_ZMOTOR);

#ifdef ESP8266
  // Aangezien de PWM range van analogWrite afhankelijk van de Arduino ESP8266 versie 255 ofwel 1023 is, stellen we de range vast in op 1023
  analogWriteRange(PWM_RANGE);

  // Verander de frequentie van analogWrite van 1000 Hz naar 400 Hz voor een aangenamer geluid
  analogWriteFreq(MOTOR_FREQ);
#endif
  analogWrite(PIN_ZMOTOR, 0);
  analogWrite(PIN_1AMOTOR, 0);
  analogWrite(PIN_2AMOTOR, 0);
  analogWrite(PIN_1BMOTOR, 0);
  analogWrite(PIN_2BMOTOR, 0);

  delay(200); // 200 milliseconden wachten tot de stroom stabiel is

#ifdef DEBUG_SERIAL
  delay (1000);
  DEBUG_SERIAL.begin(115200);
  DEBUG_SERIAL.println(F("\nHover Browser setup started"));
#endif

#ifdef PIN_LEDCONNECTIE
  setup_pin_mode_output(PIN_LEDCONNECTIE);

  // De LED flasht 2x om te tonen dat er een reboot is
  digitalWrite(PIN_LEDCONNECTIE, LED_BRIGHTNESS_ON);
  delay(10);
  digitalWrite(PIN_LEDCONNECTIE, LED_BRIGHTNESS_OFF);
  delay(100);
  digitalWrite(PIN_LEDCONNECTIE, LED_BRIGHTNESS_ON);
  delay(10);
  digitalWrite(PIN_LEDCONNECTIE, LED_BRIGHTNESS_OFF);
#endif

  init_motors();

#ifdef PIN_LEDCONNECTIE
  digitalWrite(PIN_LEDCONNECTIE, LED_BRIGHTNESS_ON );
#endif

  // setup gyro module
  Wire.begin();

  delay(100);

  gyroBeschikbaar = false;
  for (int t = 0; t < 3; t++) // 3 keer proberen of gyro beschikbaar is
  {
    if (sensor.wakeup() == false)
    {
#ifdef DEBUG_SERIAL
      DEBUG_SERIAL.print(millis());
      DEBUG_SERIAL.println("\tCould not connect to GY521");
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
    sensor.setAccelSensitivity(2);  // 8g
    sensor.setGyroSensitivity(1);   // 500 degrees/s

    sensor.setThrottle();
#ifdef DEBUG_SERIAL
    DEBUG_SERIAL.println("start...");
#endif

    // set all calibration errors to zero
    sensor.gxe = 0;
    sensor.gye = 0;
    sensor.gze = 0;
    sensor.read();
  }

  // Wifi instellingen
  WiFi.persistent(true);

  uint8_t macAddr[6];
  WiFi.macAddress(macAddr);

#if defined(USE_SOFTAP)
  WiFi.disconnect();
  /* zet een access point op */
  WiFi.mode(WIFI_AP);

  // ssidmac = ssid + 4 laatste hexadecimale waarden van het MAC-adres
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
  /* DNS server opzetten die alle domeinen vertaalt naar apIP */
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(53, "h.be", apIP);

#else
  WiFi.softAPdisconnect(true);
  // host_name = "Hover-" + 6 hexadecimale waarden van het MAC-adres
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

  // Even wachten tot er verbinding is met het wifi netwerk
  for (int i = 0; i < 15 && WiFi.status() != WL_CONNECTED; i++) {
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

  webserver.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
#ifdef DEBUG_SERIAL
    DEBUG_SERIAL.println(F("on HTTP_GET: return"));
#endif
    request->send(200, "text/html", index_html);
  });

  webserver.begin();
  server.listen(82);
#ifdef DEBUG_SERIAL
  DEBUG_SERIAL.print(F("Is server live? "));
  DEBUG_SERIAL.println(server.available());
#endif
  last_activity_message = millis();
}


void updatestatusbar(boolean forceupdate)
{
#ifdef ESP8266
  static unsigned long lastupdate_voltage = 0;
  unsigned long currentmillis = millis();
  char statusstr[80];

  if (forceupdate || (currentmillis > lastupdate_voltage + TIMEOUT_MS_VOLTAGE))
  {
    lastupdate_voltage = currentmillis;
    float voltage = ESP.getVcc() / VOLTAGE_FACTOR;

    if (voltage >= VOLTAGE_THRESHOLD)
    {
      if (gyroBeschikbaar)
      {
        snprintf(statusstr, sizeof(statusstr), "%4.2f V gz:%4.2f", voltage, sensor.getGyroZ());
      } else
      {
        snprintf(statusstr, sizeof(statusstr), "%4.2f V", voltage);
      }       
#ifdef DEBUG_SERIAL
      // DEBUG_SERIAL.print("Sending status: ");
      // DEBUG_SERIAL.println(statusstr);
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
      delay(20000); // boodschap wordt 20 seconden getoond in browser alvorens hij disconnecteert
      ESP.deepSleep(0);
    }
  }
#endif
}


void handleSlider2(int value) // Z (zweef) motor besturing geworden
{
#ifdef DEBUG_SERIAL
  DEBUG_SERIAL.print(F("handleSlider2 value="));
  DEBUG_SERIAL.println(value);
#endif
  //max_motorsnelheid = map(value, 0, 360, PWM_RANGE / 2, PWM_RANGE);
  ui_slider2 = value;

  updateMotors();
}

void handleSlider1(int value)
{
#ifdef DEBUG_SERIAL
  DEBUG_SERIAL.print(F("handleSlider1 value="));
  DEBUG_SERIAL.println(value);
#endif

  ui_slider1 = value;

  updateMotors();
}

void handleSlider3(int value)
{
#ifdef DEBUG_SERIAL
  DEBUG_SERIAL.print(F("handleSlider3 value="));
  DEBUG_SERIAL.println(value);
#endif

  ui_slider3 = value;
  updateMotors();
}


void handleJoystick(int x, int y)
{
#ifdef DEBUG_SERIAL
  // DEBUG_SERIAL.print(F("handleJoystick x="));
  // DEBUG_SERIAL.print(x);
  // DEBUG_SERIAL.print(F(" y="));
  // DEBUG_SERIAL.println(y);
#endif

  ui_joystick_x = x;
  ui_joystick_y = y;

  updateMotors();
}

void handle_message(websockets::WebsocketsMessage msg) {
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

#ifdef PIN_LEDCONNECTIE
  digitalWrite(PIN_LEDCONNECTIE, LED_BRIGHTNESS_ON);
#endif

  last_activity_message = millis();

  switch (id)
  {
    case 0:       // ping
      break;

    case 1:
      handleJoystick(param1, param2);
      break;

    case 2: handleSlider2(param1); // z-speed
      break;

    case 3: handleSlider1(param1); // p-control
      break;
     
    case 20: handleSlider3(param1); // draaisnelheid
      break;
  }
  if (motors_halt)
  {
    motors_resume();
  }
}

void onConnect()
{
#ifdef PIN_LEDCONNECTIE
  digitalWrite(PIN_LEDCONNECTIE, LED_BRIGHTNESS_OFF);
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
}

void loop()
{
  static int is_connected = 0;

#if defined(USE_SOFTAP)
  dnsServer.processNextRequest();
#endif

  if (millis() > last_activity_message + TIMEOUT_MS_LED)
  {
#ifdef PIN_LEDCONNECTIE
    digitalWrite(PIN_LEDCONNECTIE, LED_BRIGHTNESS_OFF);
#endif
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
    if (sclient.available()) { // als return non-nul, dan is er een client geconnecteerd
      sclient.poll(); // als return non-nul, dan is er iets ontvangen

      updatestatusbar(false);

      updateMotors();
    }
    else
    {
      // niet langer geconnecteerd
      onDisconnect();
      is_connected = 0;
    }
  }
  if (server.poll()) // als er een nieuwe socket aangevraagd is
  {
#ifdef DEBUG_SERIAL
    DEBUG_SERIAL.print(F("server.poll is_connected="));
    DEBUG_SERIAL.println(is_connected);
#endif
    if (is_connected) {
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
#ifdef PIN_LEDCONNECTIE
    digitalWrite(PIN_LEDCONNECTIE, (millis() % 1000) > 500 ? LOW : HIGH);
#endif
  }

  // delay(2);
}
