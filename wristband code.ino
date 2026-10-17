/*
  ============================================================
              WOMEN SAFETY EMERGENCY DEVICE
  ============================================================

  CONTROLLER:
  ESP32

  MODULES:
  - 0.96" I2C OLED
  - NEO-6M GPS
  - SIM900A GSM
  - Push Button
  - LOW-LEVEL TRIGGER BUZZER

  ============================================================
  PIN CONNECTIONS
  ============================================================

  OLED:
    SDA  -> GPIO 21
    SCK  -> GPIO 22
    VDD  -> 3.3V
    GND  -> GND

  GPS:
    TX -> GPIO 16 (ESP32 RX2)
    RX -> GPIO 17 (ESP32 TX2)

  GSM:
    TX -> GPIO 26 (ESP32 RX)
    RX -> GPIO 27 (ESP32 TX)

  BUZZER:
    Signal -> GPIO 25
    GND    -> GND

  PUSH BUTTON:
    One side -> GPIO 4
    Other side -> GND

  ============================================================
  BUZZER LOGIC
  ============================================================

  This buzzer is LOW-LEVEL TRIGGER.

  HIGH = BUZZER OFF
  LOW  = BUZZER ON

  ============================================================
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>


// ============================================================
// OLED
// ============================================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);


// ============================================================
// GPS
// ============================================================

HardwareSerial gpsSerial(2);

#define GPS_RX 16
#define GPS_TX 17

TinyGPSPlus gps;


// ============================================================
// GSM
// ============================================================

HardwareSerial gsmSerial(1);

#define GSM_RX 26
#define GSM_TX 27


// ============================================================
// BUTTON + BUZZER
// ============================================================

#define BUTTON_PIN 4
#define BUZZER_PIN 25


// ============================================================
// PHONE NUMBER
// ============================================================

// CHANGE THIS TO THE RELATIVE'S NUMBER
const char PHONE_NUMBER[] = "9491650525";


// ============================================================
// BUTTON SETTINGS
// ============================================================

// Button must be held for 2 seconds
const unsigned long LONG_PRESS_TIME = 2000;


// ============================================================
// BUZZER SETTINGS
// ============================================================

// LOW-LEVEL TRIGGER
// HIGH = OFF
// LOW  = ON

bool emergencyActive = false;

unsigned long buzzerTimer = 0;

int buzzerStep = 0;


// ============================================================
// BUTTON VARIABLES
// ============================================================

bool buttonPressed = false;

unsigned long buttonStartTime = 0;


// ============================================================
// OLED DISPLAY FUNCTION
// ============================================================

void showOLED(
  String line1,
  String line2,
  String line3
)
{
  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);

  display.setCursor(0, 5);
  display.println(line1);

  display.setCursor(0, 27);
  display.println(line2);

  display.setCursor(0, 49);
  display.println(line3);

  display.display();
}


// ============================================================
// EMERGENCY BUZZER PATTERN
// ============================================================
//
// Pattern:
//
// BEEP
// pause
// BEEP
// pause
// BEEP BEEP BEEP
// pause
// Repeat
//
// LOW  = ON
// HIGH = OFF
//
// ============================================================

void updateEmergencyBuzzer()
{
  // ----------------------------------------------------------
  // If emergency is NOT active
  // keep buzzer OFF
  // ----------------------------------------------------------

  if (!emergencyActive)
  {
    digitalWrite(BUZZER_PIN, HIGH);
    return;
  }


  unsigned long currentTime = millis();


  switch (buzzerStep)
  {

    // --------------------------------------------------------
    // BEEP 1
    // --------------------------------------------------------

    case 0:

      digitalWrite(BUZZER_PIN, LOW);

      if (currentTime - buzzerTimer >= 200)
      {
        digitalWrite(BUZZER_PIN, HIGH);

        buzzerTimer = currentTime;

        buzzerStep = 1;
      }

      break;


    // --------------------------------------------------------
    // PAUSE
    // --------------------------------------------------------

    case 1:

      digitalWrite(BUZZER_PIN, HIGH);

      if (currentTime - buzzerTimer >= 200)
      {
        buzzerTimer = currentTime;

        buzzerStep = 2;
      }

      break;


    // --------------------------------------------------------
    // BEEP 2
    // --------------------------------------------------------

    case 2:

      digitalWrite(BUZZER_PIN, LOW);

      if (currentTime - buzzerTimer >= 200)
      {
        digitalWrite(BUZZER_PIN, HIGH);

        buzzerTimer = currentTime;

        buzzerStep = 3;
      }

      break;


    // --------------------------------------------------------
    // PAUSE
    // --------------------------------------------------------

    case 3:

      digitalWrite(BUZZER_PIN, HIGH);

      if (currentTime - buzzerTimer >= 200)
      {
        buzzerTimer = currentTime;

        buzzerStep = 4;
      }

      break;


    // --------------------------------------------------------
    // RAPID BEEP 1
    // --------------------------------------------------------

    case 4:

      digitalWrite(BUZZER_PIN, LOW);

      if (currentTime - buzzerTimer >= 120)
      {
        digitalWrite(BUZZER_PIN, HIGH);

        buzzerTimer = currentTime;

        buzzerStep = 5;
      }

      break;


    // --------------------------------------------------------
    // RAPID PAUSE
    // --------------------------------------------------------

    case 5:

      digitalWrite(BUZZER_PIN, HIGH);

      if (currentTime - buzzerTimer >= 120)
      {
        buzzerTimer = currentTime;

        buzzerStep = 6;
      }

      break;


    // --------------------------------------------------------
    // RAPID BEEP 2
    // --------------------------------------------------------

    case 6:

      digitalWrite(BUZZER_PIN, LOW);

      if (currentTime - buzzerTimer >= 120)
      {
        digitalWrite(BUZZER_PIN, HIGH);

        buzzerTimer = currentTime;

        buzzerStep = 7;
      }

      break;


    // --------------------------------------------------------
    // RAPID PAUSE
    // --------------------------------------------------------

    case 7:

      digitalWrite(BUZZER_PIN, HIGH);

      if (currentTime - buzzerTimer >= 120)
      {
        buzzerTimer = currentTime;

        buzzerStep = 8;
      }

      break;


    // --------------------------------------------------------
    // RAPID BEEP 3
    // --------------------------------------------------------

    case 8:

      digitalWrite(BUZZER_PIN, LOW);

      if (currentTime - buzzerTimer >= 120)
      {
        digitalWrite(BUZZER_PIN, HIGH);

        buzzerTimer = currentTime;

        // Start pattern again
        buzzerStep = 0;
      }

      break;
  }
}


// ============================================================
// GET GPS LOCATION
// ============================================================

bool getGPSLocation()
{
  Serial.println();
  Serial.println("================================");
  Serial.println("      GETTING GPS LOCATION");
  Serial.println("================================");


  showOLED(
    "WOMEN SAFETY",
    "GETTING GPS...",
    "Please wait"
  );


  unsigned long startTime = millis();


  // Wait maximum 60 seconds
  while (millis() - startTime < 60000)
  {

    // --------------------------------------------------------
    // Read GPS data
    // --------------------------------------------------------

    while (gpsSerial.available())
    {
      char c = gpsSerial.read();

      gps.encode(c);
    }


    // --------------------------------------------------------
    // Keep emergency buzzer running
    // --------------------------------------------------------

    updateEmergencyBuzzer();


    // --------------------------------------------------------
    // Check GPS location
    // --------------------------------------------------------

    if (gps.location.isValid())
    {
      Serial.println();
      Serial.println("GPS LOCATION FOUND!");

      Serial.print("Latitude : ");
      Serial.println(
        gps.location.lat(),
        6
      );

      Serial.print("Longitude: ");
      Serial.println(
        gps.location.lng(),
        6
      );

      return true;
    }


    delay(5);
  }


  Serial.println();
  Serial.println("GPS LOCATION NOT FOUND!");

  return false;
}


// ============================================================
// SEND EMERGENCY SMS
// ============================================================

bool sendEmergencySMS()
{
  Serial.println();
  Serial.println("================================");
  Serial.println("       SENDING EMERGENCY SMS");
  Serial.println("================================");


  showOLED(
    "WOMEN SAFETY",
    "SENDING SMS...",
    "Please wait"
  );


  // ----------------------------------------------------------
  // Set SMS text mode
  // ----------------------------------------------------------

  gsmSerial.println("AT+CMGF=1");

  delay(1000);


  // Clear old GSM responses
  while (gsmSerial.available())
  {
    gsmSerial.read();
  }


  // ----------------------------------------------------------
  // Start SMS
  // ----------------------------------------------------------

  gsmSerial.print("AT+CMGS=\"");

  gsmSerial.print(PHONE_NUMBER);

  gsmSerial.println("\"");


  delay(1500);


  // ----------------------------------------------------------
  // Create emergency message
  // ----------------------------------------------------------

  String message =
    "EMERGENCY ALERT!\n"
    "Women Safety Device has been activated.\n\n"
    "Location:\n"
    "https://maps.google.com/?q=";


  message += String(
    gps.location.lat(),
    6
  );


  message += ",";


  message += String(
    gps.location.lng(),
    6
  );


  // ----------------------------------------------------------
  // Send message
  // ----------------------------------------------------------

  gsmSerial.print(message);


  // CTRL + Z
  gsmSerial.write(26);


  Serial.println();
  Serial.println("SMS submitted.");
  Serial.println("Waiting for GSM response...");


  // ----------------------------------------------------------
  // Wait for response
  // ----------------------------------------------------------

  unsigned long startTime = millis();

  String response = "";


  while (millis() - startTime < 20000)
  {

    while (gsmSerial.available())
    {
      char c = gsmSerial.read();

      Serial.write(c);

      response += c;
    }


    // Keep emergency buzzer active
    updateEmergencyBuzzer();
  }


  Serial.println();


  // ----------------------------------------------------------
  // Check SMS result
  // ----------------------------------------------------------

  if (
    response.indexOf("OK") >= 0 ||
    response.indexOf("CMGS") >= 0
  )
  {
    Serial.println("SMS SENT SUCCESSFULLY!");

    return true;
  }


  Serial.println("SMS SEND FAILED!");

  return false;
}


// ============================================================
// SETUP
// ============================================================

void setup()
{
  Serial.begin(115200);


  // ==========================================================
  // BUZZER INITIALIZATION
  // ==========================================================
  //
  // IMPORTANT:
  // This buzzer is LOW-LEVEL TRIGGER.
  //
  // HIGH = OFF
  // LOW  = ON
  //
  // Set HIGH BEFORE making the pin OUTPUT.
  // ==========================================================

  digitalWrite(
    BUZZER_PIN,
    HIGH
  );

  pinMode(
    BUZZER_PIN,
    OUTPUT
  );

  digitalWrite(
    BUZZER_PIN,
    HIGH
  );


  // ==========================================================
  // BUTTON
  // ==========================================================

  pinMode(
    BUTTON_PIN,
    INPUT_PULLUP
  );


  // ==========================================================
  // I2C
  // ==========================================================

  Wire.begin(
    21,
    22
  );


  // ==========================================================
  // OLED
  // ==========================================================

  if (
    !display.begin(
      SSD1306_SWITCHCAPVCC,
      OLED_ADDRESS
    )
  )
  {
    Serial.println("OLED NOT FOUND!");

    while (true)
    {
      // Keep buzzer OFF if OLED fails
      digitalWrite(
        BUZZER_PIN,
        HIGH
      );

      delay(1000);
    }
  }


  // ==========================================================
  // GPS
  // ==========================================================

  gpsSerial.begin(
    9600,
    SERIAL_8N1,
    GPS_RX,
    GPS_TX
  );


  // ==========================================================
  // GSM
  // ==========================================================

  gsmSerial.begin(
    9600,
    SERIAL_8N1,
    GSM_RX,
    GSM_TX
  );


  delay(3000);


  // ==========================================================
  // FINAL BUZZER SAFETY
  // ==========================================================

  digitalWrite(
    BUZZER_PIN,
    HIGH
  );


  // ==========================================================
  // STARTUP OLED
  // ==========================================================

  showOLED(
    "WOMEN SAFETY",
    "DEVICE READY",
    "Press button"
  );


  Serial.println();
  Serial.println("================================");
  Serial.println("     WOMEN SAFETY DEVICE");
  Serial.println("           READY");
  Serial.println("================================");
  Serial.println();

  Serial.println(
    "Buzzer: OFF"
  );

  Serial.println(
    "Hold button for 2 seconds."
  );
}


// ============================================================
// LOOP
// ============================================================

void loop()
{
  // ==========================================================
  // CONTINUOUS GPS READING
  // ==========================================================

  while (gpsSerial.available())
  {
    char c = gpsSerial.read();

    gps.encode(c);
  }


  // ==========================================================
  // UPDATE BUZZER
  // ==========================================================

  updateEmergencyBuzzer();


  // ==========================================================
  // READ BUTTON
  // ==========================================================

  bool pressed =
    (digitalRead(BUTTON_PIN) == LOW);


  // ==========================================================
  // BUTTON JUST PRESSED
  // ==========================================================

  if (
    pressed &&
    !buttonPressed
  )
  {
    buttonPressed = true;

    buttonStartTime = millis();

    Serial.println();
    Serial.println("Button pressed...");
  }


  // ==========================================================
  // BUTTON IS BEING HELD
  // ==========================================================

  if (
    pressed &&
    buttonPressed &&
    !emergencyActive
  )
  {
    unsigned long holdTime =
      millis() - buttonStartTime;


    // ========================================================
    // LONG PRESS DETECTED
    // ========================================================

    if (
      holdTime >= LONG_PRESS_TIME
    )
    {
      emergencyActive = true;


      Serial.println();
      Serial.println("********************************");
      Serial.println("     EMERGENCY ACTIVATED!");
      Serial.println("********************************");


      // ------------------------------------------------------
      // OLED
      // ------------------------------------------------------

      showOLED(
        "WOMEN SAFETY",
        "DEVICE",
        "ACTIVATED!"
      );


      // ------------------------------------------------------
      // START BUZZER PATTERN
      // ------------------------------------------------------

      buzzerStep = 0;

      buzzerTimer = millis();


      // ------------------------------------------------------
      // GET GPS
      // ------------------------------------------------------

      bool gpsSuccess =
        getGPSLocation();


      // ------------------------------------------------------
      // SEND SMS
      // ------------------------------------------------------

      if (gpsSuccess)
      {
        bool smsSuccess =
          sendEmergencySMS();


        if (smsSuccess)
        {
          showOLED(
            "EMERGENCY",
            "SMS SENT",
            "LOCATION SHARED"
          );
        }
        else
        {
          showOLED(
            "SMS FAILED",
            "CHECK GSM",
            "CONNECTION"
          );
        }
      }
      else
      {
        showOLED(
          "GPS ERROR",
          "LOCATION",
          "NOT FOUND"
        );
      }
    }
  }


  // ==========================================================
  // BUTTON RELEASED
  // ==========================================================

  if (
    !pressed &&
    buttonPressed
  )
  {
    buttonPressed = false;


    Serial.println();
    Serial.println("Button released.");


    // --------------------------------------------------------
    // STOP EMERGENCY
    // --------------------------------------------------------

    emergencyActive = false;


    // --------------------------------------------------------
    // BUZZER OFF
    // --------------------------------------------------------

    digitalWrite(
      BUZZER_PIN,
      HIGH
    );


    // --------------------------------------------------------
    // RETURN TO READY SCREEN
    // --------------------------------------------------------

    showOLED(
      "WOMEN SAFETY",
      "DEVICE READY",
      "Press button"
    );
  }


  delay(10);
}