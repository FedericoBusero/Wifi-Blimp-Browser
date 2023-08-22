/*
   Code voor het besturen van een hover mbv Wifi via de browser

   Hoe gebruiken?
   Voeg wifi netwerk hover-xxxx toe met paswoord 12345678
   Er is op dat netwerk uiteraard geen internet, dus "wifi behouden" aanvinken indien dat gevraagd wordt
   Dan ga je naar de browser (chrome, firefox, safari, ..) naar de website 192.168.4.1 maar elke andere http-URL werkt ook bv. http://a.be

   De bovenste regel toont de connectie-status. Op ESP8266 wordt het voltage getoond tijdens de connectie, te calibreren met VOLTAGE_FACTOR
   De bovenste slider wordt niet gebruikt voorlopig, was om de servo te trimmen.
   De slider eronder stelt de zweefmotor in.
   Met de joystick worden 2 stuwmotoren bestuurd

*/

#include <ArduinoWebsockets.h> // uit arduino library manager : "ArduinoWebsockets" by Gil Maimon, https://github.com/gilmaimon/ArduinoWebsockets
#include <ESPAsyncWebServer.h> // https://github.com/me-no-dev/ESPAsyncWebServer
// ESP8266: https://github.com/me-no-dev/ESPAsyncTCP installeren

#include "GY521.h" // library; https://github.com/RobTillaart/GY521/    // nog niet getest bij R2D2
/*
   GY521 op Wemos D1 mini:
   SCL: D1
   SDA: D2
   XDA: niet aangesloten
   XCL: niet aangesloten
   AD0: niet aangesloten  . De vice heeft 0x68 als I2C adres, waarschijnlijk wordt het 0x69 als je dit naar 3.3V verhoogt
   INT: niet aangesloten
   VCC: 3.3V
   GND: uiteraard
*/
GY521 sensor(0x68);


#ifdef ARDUINO_ARCH_ESP32 // TODO eigenlijk S2 //OPGELET!!!! dit project is niet uitgewerkt voor ESP32
// #include <WiFi.h>
// #include <AsyncTCP.h> // https://github.com/me-no-dev/AsyncTCP
// //#include <ESP32Servo.h> // https://github.com/madhephaestus/ESP32Servo
// #define DEBUG_SERIAL Serial

// #define PWM_RANGE 255 // PWM range voor analogWrite (in ESP32Servo)

// Lolin ESP32-S2
// #define PIN_1AMOTOR          12 // Positie D8 op Wemos D1 mini
// #define PIN_2AMOTOR          11 // Positie D7 op Wemos D1 mini
// #define PIN_1BMOTOR          9  // Positie D6 op Wemos D1 mini
// #define PIN_2BMOTOR          7  // Positie D5 op Wemos D1 mini
// #define PIN_ZMOTOR           18 // Positie D3 op Wemos D1 mini
// #define PIN_LEDCONNECTIE     15 // De ingebouwde LED

// #define PIN_SERVO          17

// #define LED_BRIGHTNESS_ON  HIGH
// #define LED_BRIGHTNESS_OFF LOW

#else // ESP8266

ADC_MODE(ADC_VCC); // Nodig voor het inlezen van het voltage met ESP.getVcc

#include <ESP8266WiFi.h>
#include <Servo.h>
#include <ESPAsyncTCP.h> // https://github.com/me-no-dev/ESPAsyncTCP

#define PWM_RANGE 1023 // PWM range voor analogWrite
#define MOTOR_FREQ 400 // Frequentie van analogWrite in Hz, bepaalt het geluid van de motor

// #define MODE_ESP01
//
#ifdef MODE_ESP01 //OPGELET!!! dit project is niet uitgewerkt voor ESP01

// #define PIN_SERVO          0
// #define PIN_MOTOR          3
// #define PIN_LEDCONNECTIE   1

// // Pas de voltagefactor aan, dat is bij elke chip hetzelfde. Calibreer bv. met USB stroom die 3.3V op de chip moet geven
// #define VOLTAGE_FACTOR 1060.0f

#else // Wemos D1 mini, NodeMCU, ...
#define DEBUG_SERIAL Serial

//#define PIN_SERVO          Geen servo D3 // STOND OP D2 = GPIO4  op NodeMCU & Wemos D1 mini
#define PIN_1AMOTOR          D8 // D8 = GPIO15 op NodeMCU & Wemos D1 mini
#define PIN_2AMOTOR          D7
#define PIN_1BMOTOR          D6
#define PIN_2BMOTOR          D0  // om D5 vrij te maken voor speker, want met speaker op D0 werkt wifi opstart niet meer? Mogelijks door dubbel gebruik PIN_EXTERNSIGNAAL, maar probleem ontstond ook zonder speaker, maar aansluiting input andere ESP op D0
#define PIN_ZMOTOR           D3

// todo je kan toch niet debuggen als debug_serial en servo gelijktijdig aanstaan?? debug_serial en pin_servo kunnen niet gelijktijdig aan staan
#define PIN_SERVO            1 //TX dus, werkt alvast op ESP01, dus waarschijnlijk ook op andere SEP8266 modules


// todo define erbij voor r2d2 effects?

// WS2812 en led zit op zelfde pin !
#define PIN_WS2812 2 
#define PIN_LEDCONNECTIE 2 // jawel op zelfde bin als gewone LED, zou volgens WEMOS mini shield vbn moeten werken

//voor R2D2sound
#define speakerPin D5

// Pas de voltagefactor aan, dat is bij elke chip hetzelfde. Calibreer bv. met USB stroom die 3.3V op de chip moet geven
#define VOLTAGE_FACTOR 910.0f

#endif // MODE_ESP01

#define LED_BRIGHTNESS_ON  LOW
#define LED_BRIGHTNESS_OFF HIGH

#endif // ARDUINO_ARCH_ESP32

#include <FastLED.h>

#define LEDSTRIP_MAX_BRIGHTNESS 50
#define NUMLEDPIXELS      4
CRGB leds[NUMLEDPIXELS];
unsigned long LaatstMotorsOfGeluid; // todo beter wijzigen in volgendR2d2geluid (random slechts eenmaal uitvoeren)

#define USE_SOFTAP
#define WIFI_SOFTAP_CHANNEL 1 // 1-13
const char ssid[] = "R2-D";
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

// todo volgende regels enkel  als led pin verschillend van ws2812 pin #if x != y
#define TIMEOUT_MS_LED 1L        // Aantal milliseconden dat LED blijft branden na het ontvangen van een boodschap
unsigned long last_activity_message;

#define TIMEOUT_MS_VOLTAGE 10000L // Aantal milliseconden tussen update voltage

// In deze versie weer een servo
// We maken een servo "object" aan om de servo aan te sturen.
Servo servo1;

// De minimum en maximum hoek van de servo, pas dit gerust aan als de servo de uitersten niet kan halen
// De waarden zijn minimaal 0, maximaal 180
#define SERVO_HOEK_MIN 0
#define SERVO_HOEK_MAX 180

// We verplaatsen de servo in stapjes om geen al te bruuske bewegingen te maken
// Pas dit gerust aan, 1=servo traag bewegen, 2=normaal en vanaf 4 gaat het heel snel.
// De waarde is minimaal 1 en maximaal 180, dan is er geen vertraging meer
#define SERVO_HOEK_STAP 2

int Servopositie_x;   // -180 .. 180 niet gebruikt in deze motorversie
int TrimServopositie; // -180 .. 180 //voorlopig niet hernoemd, maar dubbel gebruikt
int servohoek = (SERVO_HOEK_MIN + SERVO_HOEK_MAX) / 2; // wel weer niet gebruikt in deze motorversie
int doel_servohoek;
int currentSlider2 = 0;

int currentX = 0;
int currentY = 0;
bool gyroBeschikbaar = false;

const float Cfactor = -2; // conversiefactor van gemeten gyroZ naar de arbitraire eenheden van currentX
const float maxPfactor = 4; // maximum voor de proportionele regelfactor Pfactor, bepaald met slider

//In Deze versie NIET:
// Bij het verhogen van de snelheid van de motor, doen we dat in stappen om niet te bruusk op te trekken
// want dit kan de hovercraft onbestuurbaar maken of teveel stroom trekken waardoor de chip gaat resetten
// Pas gerust aan, 1=traag optrekken, 5=snel optrekken
// De waarde is minimaal 1 en maximaal 1023
// #define MAX_MOTOR_SPEED_STAP 4

//int motor_snelheid = 0; niet meer gebruikt, was voor 1 motorversie

int max_motorsnelheid;
bool motors_halt;

// void led_fill (long led_colour) // (nog) niet gebruikt
// {
//     //current_led_color = CHSV( configdata.hue, 255, configdata.brightness); // nog niet gebruikt
//   fill_solid (leds, NUMLEDPIXELS, led_colour);
//   FastLED.show();
// }

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
  static int counter = 0; // todo wordt niet gebruikt

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
    // todo beter andere naam kiezen zoals werkelijke_draaisnelheid voor het geval gyro anders bevestigd is
    float gyroZ;
    float regelX = 0;
     
    if (gyroBeschikbaar) {// gyro
      sensor.read();
      gyroZ = sensor.getGyroZ();

      // "gyro"-regeling
      // TODO: map is integer macro? p factor enkel integer?
      float Pfactor = map(TrimServopositie, -180, 180, 0, maxPfactor); // TrimServopositie slider voorlopig dubbel gebruikt

      regelX = ((1 + (Pfactor)) * (float)currentX) - (Pfactor * Cfactor * (gyroZ)); // bijgestuurde x in verhouding tot afwijking op gewenste draaisnelheid, X van joystick is de gewenste draaisnelheid

      if (counter % 10 == 0)
      {
        // Serial.print("gyroZ: ");
        // Serial.println(gyroz, 3);
      }
    }
    else {
      gyroZ = 0;
      regelX = (float)currentX;
    }
    counter++;

    /* We berekenen naar welke doelpositie we de servo willen krijgen:
        we herschalen de som van de slider posities in de browser ( Servopositie_x (-180 .. 180) en TrimServopositie (-180 .. 180) )
        naar de minimum en maximum graden die de servo motor aankan (SERVO_HOEK_MIN .. SERVO_HOEK_MAX)
    */
    doel_servohoek = map(Servopositie_x + TrimServopositie, -360, 360, SERVO_HOEK_MIN, SERVO_HOEK_MAX);

    /*
      We gaan de servo nog niet onmiddellijk naar zijn nieuwe positie doel_servohoek brengen, maar elke keer dat we hier passeren
      gaan we ietsje dichter naar zijn doel. Daartoe beperken we de verplaatsing t.o.v. de oude servohoek tot maximum SERVO_HOEK_STAP stappen
    */
    servohoek = constrain(doel_servohoek, servohoek - SERVO_HOEK_STAP, servohoek + SERVO_HOEK_STAP);
    servohoek = doel_servohoek;
    // todo ifdef servopin
    // todo vertraagcode kan er beter uit en gyro corretie erin
    servo1.write(servohoek);  // We verplaatsen de servo naar de nieuwe positie servohoek

    /*
      We gaan de motor nog niet onmiddellijk naar zijn snelheid doel_motorsnelheid brengen, maar elke keer dat we hier passeren
      gaan we ietsje dichter naar zijn doel. Daartoe mag hij elke keer maximum MAX_MOTOR_SPEED_STAP verhogen in snelheid
    */
    /*
      #ifdef DEBUG_SERIAL
      DEBUG_SERIAL.print(F("doel_motorsnelheid="));
      DEBUG_SERIAL.println(doel_motorsnelheid);
      DEBUG_SERIAL.print(F("motor_snelheid="));
      DEBUG_SERIAL.println(motor_snelheid);
      #endif
    */
    // motor_snelheid = min(doel_motorsnelheid, motor_snelheid + MAX_MOTOR_SPEED_STAP);

    int z_motorsnelheid = map(currentSlider2, 0, 360, 0, PWM_RANGE);

    // todo onduidelijke implicit typecasting int/float in abs
    // gehele lichtcode zou eigenlijk beter in een nieuwe functie update_lichten die opgeroepen wordt vanuit loop
    if (abs(currentY * currentX) < 5) { //opgelet gebeurt soms tussendoor heel kort blijkbaar geled op geflikker WS2812?!
      z_motorsnelheid = 0; // bij joystick los ook zweefmotor uit
      regelX = 0;
      leds[0] = CRGB::Red;
      leds[3] = CRGB::DarkGreen;
      FastLED.show();
    }
    else {
      LaatstMotorsOfGeluid = millis(); // bijhouden hoe lang geleden motors nog aan.
      leds[0] = CRGB::Blue;
      leds[3] = CRGB::Yellow;
      FastLED.show();
    }

#ifdef DEBUG_SERIAL
    //      DEBUG_SERIAL.print("  millis() ");
    //      DEBUG_SERIAL.println(millis());
    DEBUG_SERIAL.print("  currentX ");
    DEBUG_SERIAL.println(currentX);
    //      DEBUG_SERIAL.print("  currentY ");
    //      DEBUG_SERIAL.println(currentY);
    //      DEBUG_SERIAL.print("  Pfactor: ");
    //      DEBUG_SERIAL.print(Pfactor);
    DEBUG_SERIAL.print("  regelX: ");
    DEBUG_SERIAL.println(regelX);
#endif

    // x en y omzetten naar motorsnelheden
    float temp1 = (float)currentY + regelX; // mix na gyro regeling
    float temp2 = (float)currentY - regelX; // mix na gyro regeling
    // todo bug : waarden temp1&temp2 moeten binnen range -180 180 dus constrain te gebruiken
    int doel_motorsnelheidA = map(-temp2, -180, 180, -max_motorsnelheid, max_motorsnelheid);
    int doel_motorsnelheidB = map(-temp1, -180, 180, -max_motorsnelheid, max_motorsnelheid);

    hbridge_setspeed(PIN_1AMOTOR, PIN_2AMOTOR, doel_motorsnelheidA);
    hbridge_setspeed(PIN_1BMOTOR, PIN_2BMOTOR, doel_motorsnelheidB);
    analogWrite(PIN_ZMOTOR, z_motorsnelheid); // zweefmotor/ z-as motor naar zijn snelheid z_motorsnelheid

#ifdef DEBUG_SERIAL
    //   DEBUG_SERIAL.print(F("temp1 "));
    //   DEBUG_SERIAL.print(temp1);
    //   DEBUG_SERIAL.print(F("temp2 "));
    //   DEBUG_SERIAL.println(temp2);
    DEBUG_SERIAL.print(F("motorsnelheid A="));
    DEBUG_SERIAL.print(doel_motorsnelheidA);
    DEBUG_SERIAL.print(F(" B="));
    DEBUG_SERIAL.println(doel_motorsnelheidB);
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
  TrimServopositie = 0;
  Servopositie_x = 0;
  servohoek = (SERVO_HOEK_MIN + SERVO_HOEK_MAX) / 2;
  doel_servohoek = (SERVO_HOEK_MIN + SERVO_HOEK_MAX) / 2;

  //  motor_snelheid = 0;

  max_motorsnelheid = PWM_RANGE; // komt van (300 * PWM_RANGE) / 360; als startwaarde toen 2e slider hierop werkte.
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

  // De LED flasht 2x om te tonen dat er een reboot is
  setup_pin_mode_output(PIN_LEDCONNECTIE); 
  digitalWrite(PIN_LEDCONNECTIE, LED_BRIGHTNESS_ON);
  delay(10);
  digitalWrite(PIN_LEDCONNECTIE, LED_BRIGHTNESS_OFF);
  delay(100);
  digitalWrite(PIN_LEDCONNECTIE, LED_BRIGHTNESS_ON);
  delay(10);
  digitalWrite(PIN_LEDCONNECTIE, LED_BRIGHTNESS_OFF);
  delay(10);

  FastLED.addLeds<NEOPIXEL, PIN_WS2812>(leds, NUMLEDPIXELS);
  FastLED.setBrightness(LEDSTRIP_MAX_BRIGHTNESS);
  FastLED.clear();
  fill_solid (leds, NUMLEDPIXELS, CRGB::DarkOrange);
  FastLED.show();
  FastLED.delay(2);

  // todo ifdef servopin
  // steering servo PWM             hier servo tegelijk met x input naar motoren
  setup_pin_mode_output(PIN_SERVO);
  /* we verbinden de servo met de gekozen servopin PIN_SERVO en leggen de uiterste signalen vast:
     een blokgolf signaal van 544ms stemt overeen met de servo-arm op 0° en 2400ms met 180°).
  */
  servo1.attach(PIN_SERVO, 544, 2400);


  init_motors();

#if (PIN_WS2812 != PIN_LEDCONNECTIE)
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
      Serial.print(millis());
      Serial.println("\tCould not connect to GY521");
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
    Serial.println("start...");

    // todo set all calibration errors to zero
    sensor.gze = 0;
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

  //R2D2sound
  pinMode(speakerPin, OUTPUT);

  fill_solid (leds, NUMLEDPIXELS, CRGB::Black); //WS2812 leds uit
  FastLED.show();
}

void kalibreer_gyro(int num_iter, float kalib_factor)
{
  float gz = 0;
  for (int i = 0; i < num_iter; i++)
  {
    sensor.read();
    gz -= sensor.getGyroZ();
  }
  sensor.gze += gz * kalib_factor;
  sensor.read();
#ifdef DEBUG_SERIAL
  //   DEBUG_SERIAL.print(F("sensor.gze   "));
  //   DEBUG_SERIAL.println(sensor.gze);
#endif
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

    if (gyroBeschikbaar)
    {
      snprintf(statusstr, sizeof(statusstr), "%4.2f V gze:%4.2f gz:%4.2f", voltage,sensor.gze,sensor.getGyroZ());
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
#endif
}

void handleSliderZSpeed(int value) // Z (zweef) motor besturing geworden
{
#ifdef DEBUG_SERIAL
  DEBUG_SERIAL.print(F("handleSliderZSpeed value="));
  DEBUG_SERIAL.println(value);
#endif
  //max_motorsnelheid = map(value, 0, 360, PWM_RANGE / 2, PWM_RANGE);
  currentSlider2 = value;

  updateMotors();
}

void handleSliderTrimServo(int value)
{
#ifdef DEBUG_SERIAL
  DEBUG_SERIAL.print(F("handleSliderTrimServo value="));
  DEBUG_SERIAL.println(value);
#endif

  TrimServopositie = value;

  updateMotors();
}

void handleJoystick(int x, int y)
{
#ifdef DEBUG_SERIAL
  //  todo code terugzetten
#endif

  currentX = x;
  currentY = y;


  //      doel_motorsnelheid = map(-y, 0, 180, 0, max_motorsnelheid);
  //
  Servopositie_x = x; //dus X doet zowel servo als motoren in deze versie
  //  if (y <= 0)
  //  {
  //    doel_motorsnelheid = map(-y, 0, 180, 0, max_motorsnelheid);
  //  }
  //  else
  //  {
  //    doel_motorsnelheid = 0;
  //  }

  updateMotors();
}

void handleButton1(int value)
{
#ifdef DEBUG_SERIAL
  DEBUG_SERIAL.print(F("handleButton1 value="));
  DEBUG_SERIAL.println(value);
#endif

  if (gyroBeschikbaar)
  {
    kalibreer_gyro(20, 0.05);
    updatestatusbar(true);
  }
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
   
#if (PIN_WS2812 != PIN_LEDCONNECTIE)
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

    case 2: handleSliderZSpeed(param1);
      break;

    case 3: handleSliderTrimServo(param1);
      break;

    case 10: handleButton1(param1); 
      break;
  }
  if (motors_halt)
  {
    motors_resume();
  }
}

void onConnect()
{
#if (PIN_WS2812 != PIN_LEDCONNECTIE)
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

void phrase1() {

  int k = random(1000, 2000);
  leds[1] = CRGB::DarkGreen;
  FastLED.show();
  for (int i = 0; i <=  random(100, 2000); i++) {

    delay(random(.9, 2));
    tone(speakerPin, k + (-i * 2));
    delay(random(.9, 2));
  }
  leds[1] = CRGB::Black;// uit
  FastLED.show();
  for (int i = 0; i <= random(100, 1000); i++) {
    delay(random(.9, 2));

    tone(speakerPin, k + (i * 10));
    delay(random(.9, 2));
  }
}

void phrase2() {

  int k = random(1000, 2000);
  leds[1] = CRGB::DarkGreen;
  FastLED.show();
  for (int i = 0; i <= -random(100, 2000); i--) {

    tone(speakerPin, k + (i * 2));
    delay(random(.9, 2));
  }
  leds[1] = CRGB::Black;
  FastLED.show();
  for (int i = 0; i <= random(100, 1000); i++) {

    tone(speakerPin, k + (-i * 10));
    delay(random(.9, 2));
  }
}

void R2D2sound() {
  int frequency;

  int K = 2000;
  switch (random(1, 7)) {

    case 1: phrase1(); break;
    case 2: phrase2(); break;
    case 3: phrase1(); phrase2(); break;
    case 4: phrase1(); phrase2(); phrase1(); break;
    case 5: phrase1(); phrase2(); phrase1(); phrase2(); phrase1(); break;
    case 6: phrase2(); phrase1(); phrase2(); break;

  }
  for (int i = 0; i <= random(3, 9); i++) {

    leds[1] = CRGB::DarkGreen;
    FastLED.show();
    //duur = random(70, 170);
    frequency = K + random(-1700, 2000);
    tone(speakerPin, frequency);
    //delay(duur);
    delay(random(70, 170));
    leds[1] = CRGB::Black;
    FastLED.show();
    noTone(speakerPin);
    delay(random(0, 30));
  }
  noTone(speakerPin);

}


void loop()
{
  static int is_connected = 0;

#if defined(USE_SOFTAP)
  dnsServer.processNextRequest();
#endif

  if (millis() > last_activity_message + TIMEOUT_MS_LED)
  {
#if (PIN_WS2812 != PIN_LEDCONNECTIE)
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

  // todo beter volgender2d2_show zodat random slechts 1 keer uitgevoerd wordt
  if ((millis() - LaatstMotorsOfGeluid) >= (unsigned long)random(3000, 30000)) // langer dan 3 a 30 sec motors niet aan.
  {
    R2D2sound();
    LaatstMotorsOfGeluid = millis(); //weer even wachten
  }

  if (!is_connected)
  {
    digitalWrite(PIN_LEDCONNECTIE, (millis() % 1000) > 500 ? LOW : HIGH);
  }

  // delay(2);
}
