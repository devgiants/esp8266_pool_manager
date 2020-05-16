/****************************
 * Includes
 */
#include <WiFi.h>
#include <string.h>
#include <Ticker.h>

// TODO handle why LCD_DELAY function is not taken in account
#define LCD_DELAY 500

#include <liquid-crystal-i2c-with-delay.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoLog.h>
#include <Keypad.h>

/****************************
 * Define
 */

// I2C communication definition
#define SDA 21
#define SCL 22

// LCD definition
#define LCD_ADDRESS 0x27
#define LCD_ROWS_NUMBER 4
#define LCD_COLS_NUMBER 20
#define CENTER "CENTER"
#define RIGHT "RIGHT"
#define CELSIUS_DEGREE_SYMBOL 0

// Keypad
#define KEYPAD_ROWS_NUMBER 4
#define KEYPAD_COLUMNS_NUMBER 4
#define KEYPAD_ROW_1 33
#define KEYPAD_ROW_2 25
#define KEYPAD_ROW_3 26
#define KEYPAD_ROW_4 27
#define KEYPAD_COLUMN_1 18
#define KEYPAD_COLUMN_2 5
#define KEYPAD_COLUMN_3 17
#define KEYPAD_COLUMN_4 16

// OneWire definition
#define ONE_WIRE_BUS 32
#define MIN_TEMPERATURE_PRECISION 9
#define MAX_TEMPERATURE_PRECISION 12

// GPIO Pin definition
#define FILTRATION_PUMP 23
#define HEATING_PUMP 19

// Time definition
#define UPDATE_TIMEOUT 10
#define LCD_AUTO_OFF_DELAY 25

// Screens definition
#define TEMPERATURE_SCREEN_ID 1
#define SECOND_SCREEN_ID 2

/****************************
 * Constants
 */
const String updateMessage = "MAJ...";
const String version = "V0.1";

// Main objects
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLS_NUMBER, LCD_ROWS_NUMBER);
Ticker updaterTicker;
Ticker screenSavingTicker;
OneWire ow(ONE_WIRE_BUS);
DallasTemperature dallasSensors(&ow);

DeviceAddress waterTempSensorAddress = {0x28, 0xCA, 0x98, 0xCF, 0x05, 0x0, 0x0, 0x51};
DeviceAddress heatWaterSystemTempSensorAddress = {0x28, 0xC4, 0xA8, 0xCF, 0x05, 0x0, 0x0, 0xC6};

DeviceAddress *dallasSensorsAddresses[] = {
    &waterTempSensorAddress,          // ID 0
    &heatWaterSystemTempSensorAddress // ID 1
};

// Init Keypad
char keys[KEYPAD_ROWS_NUMBER][KEYPAD_COLUMNS_NUMBER] = {
    {'1', '2', '3', '>'},
    {'4', '5', '6', '<'},
    {'7', '8', '9', 'Ʌ'},
    {'C', '0', 'E', 'V'}};

byte rowPins[KEYPAD_ROWS_NUMBER] = {KEYPAD_ROW_1, KEYPAD_ROW_2, KEYPAD_ROW_3, KEYPAD_ROW_4};
byte colPins[KEYPAD_COLUMNS_NUMBER] = {KEYPAD_COLUMN_1, KEYPAD_COLUMN_2, KEYPAD_COLUMN_3, KEYPAD_COLUMN_4};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, KEYPAD_ROWS_NUMBER, KEYPAD_COLUMNS_NUMBER);

int screens[] = {TEMPERATURE_SCREEN_ID, SECOND_SCREEN_ID};

/****************************
 * Variables
 */

// Output values
bool filtrationPumpValue = HIGH;
bool heatingPumpValue = HIGH;

// Degree symbol for display
uint8_t celsiusDegreeSymbol[8] = {0x8, 0xf4, 0x8, 0x43, 0x4, 0x4, 0x43, 0x0};

// Temperatures data
float waterTemperature, heatSystemWaterTemperature;

// Screen data
int currentScreen = TEMPERATURE_SCREEN_ID;
int screensNumber = sizeof(screens) / sizeof(screens[0]);

// Update
bool needToUpdateData = false;
bool needToUpdateDisplay = true;
bool screenAlive = true;

/**
 * Setup device an initiates everything
 */
void setup()
{

  // Stopping any air transmission so far
  WiFi.mode(WIFI_OFF);
  btStop();

  // Initiates serial connection
  Serial.begin(115200);
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);

  // Initiates I2C connection
  Log.trace("Init I2C connection.\n");
  Wire.begin(SDA, SCL);

  // Initiates DS sensors
  Log.trace("Initiates Dallas sensors.\n");
  dallasSensors.begin();

  // Init LCD
  Log.trace("Initiates LCD screen.\n");
  lcd.init();

  // Create custom chars and assign them to slots
  lcd.createChar(CELSIUS_DEGREE_SYMBOL, celsiusDegreeSymbol);

  // Print startup message
  lcd.backlight();
  lcd.setCursor(3, 0);
  lcd.print("SYSTEME DE");
  lcd.setCursor(2, 1);
  lcd.print("GESTION  PISCINE");
  lcd.setCursor(16, 3);
  lcd.print(version);
  delay(2000);
  lcd.clear();

  // Define pin mode for custom GPIO
  pinMode(FILTRATION_PUMP, OUTPUT);
  pinMode(HEATING_PUMP, OUTPUT);

  armUpdateTicker();
  armScreenSaverTicker();

  if (!dallasSensors.getAddress(waterTempSensorAddress, 0))
    Log.trace("Unable to find address for water temperature sensor.\n");
  if (!dallasSensors.getAddress(heatWaterSystemTempSensorAddress, 1))
    Log.trace("Unable to find address for heating system water temperature sensor.\n");

  // set the resolution to 9 bit per device
  Log.trace("Set temperature sensors to given precision\n");
  dallasSensors.setResolution(waterTempSensorAddress, MAX_TEMPERATURE_PRECISION);
  dallasSensors.setResolution(heatWaterSystemTempSensorAddress, MIN_TEMPERATURE_PRECISION);

  // Launch first data update
  needToUpdateData = true;
  needToUpdateDisplay = true;
}

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16)
      Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

/**
 * Loop function
 */
void loop()
{
  char key = keypad.getKey();

  // Handle keypress
  if (key != NO_KEY)
  {
    /* Log.trace("Key pressed : %s\n", key); */

    reArmScreenSaverTicker();

    // Handle actions only if screen alive
    if (screenAlive)
    {
      switch (key)
      {
      case '>':
        Log.trace("Max screen: %d\n", screensNumber);
        Log.trace("Current screen: %d\n", currentScreen);
        if (currentScreen == screensNumber)
        {
          currentScreen = screens[0];
        }
        else
        {
          currentScreen++;
        }

        Log.trace("New screen: %d\n", currentScreen);

        needToUpdateDisplay = true;
        break;

      case '<':
        Log.trace("Current screen: %d\n", currentScreen);
        Log.trace("First screen: %d\n", screens[0]);
        if (currentScreen == screens[0])
        {
          currentScreen = screens[screensNumber - 1];
        }
        else
        {
          currentScreen--;
        }

        needToUpdateDisplay = true;
        break;
      }
    }

    // Reactivate screen
    else
    {
      needToUpdateDisplay = true;
    }
  }

  // Handle update data
  if (needToUpdateData)
  {
    updateData();
    needToUpdateData = false;
  }
  // Handle update display
  if (needToUpdateDisplay)
  {
    updateDisplay();
    needToUpdateDisplay = false;
  }
}

void updateData()
{
  // Display update message only if screen alive
  if (screenAlive)
  {
    displayUpdateMessage("MAJ...");
  }

  Log.trace("START UPDATE DATA\n");

  // Update OneWire sensors data
  Log.trace("Request temperatures\n");
  dallasSensors.requestTemperatures();

  waterTemperature = dallasSensors.getTempC(waterTempSensorAddress);
  heatSystemWaterTemperature = dallasSensors.getTempC(heatWaterSystemTempSensorAddress);

  Log.trace("Water temperature : %F\n", waterTemperature);
  Log.trace("Heating system water temperature: %F\n", heatSystemWaterTemperature);

  digitalWrite(FILTRATION_PUMP, filtrationPumpValue);
  digitalWrite(HEATING_PUMP, heatingPumpValue);

  /*  filtrationPumpValue = !filtrationPumpValue;
  heatingPumpValue = !heatingPumpValue; */

  // Trigger display update only if screen is alive
  if (screenAlive)
  {
    needToUpdateDisplay = true;
  }

  Log.trace("END UPDATE DATA\n");
}

void updateDisplay()
{

  // If screen not alive, light it on
  if (!screenAlive)
  {
    lcd.backlight();
    screenAlive = true;
    armScreenSaverTicker();
  }

  switch (currentScreen)
  {
  case TEMPERATURE_SCREEN_ID:
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("Tp: " + String(waterTemperature, 1) + (char)CELSIUS_DEGREE_SYMBOL);

    lcd.rightToLeft();
    lcd.setCursor(LCD_COLS_NUMBER - 1, 0);
    lcd.print(reverse("Tc: " + String(heatSystemWaterTemperature, 0) + (char)CELSIUS_DEGREE_SYMBOL));
    lcd.leftToRight();

    displayArrows();
    break;

  case SECOND_SCREEN_ID:
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("TEST");
    displayArrows();
    break;
  }
}

void powerOffScreen()
{
  Log.trace("Shutdown screen : %d timeout reached\n", LCD_AUTO_OFF_DELAY);
  lcd.noBacklight();
  screenAlive = false;
  screenSavingTicker.detach();
}

/**
 * Function to calculate spaces to set for center/right alignment 
 */
int calculateIndent(String stringToDisplay, String mode = CENTER)
{
  // Only if string is smaller than one line
  if (stringToDisplay.length() <= LCD_COLS_NUMBER)
  {
    return (int)(LCD_COLS_NUMBER - stringToDisplay.length()) / 2;
  }
  // If bigger, it will be 0
  else
    return 0;
}

void displayUpdateMessage(String message)
{
  lcd.setCursor(calculateIndent(message), 3);
  lcd.print(message);
}

String reverse(String message)
{
  String reversedMessage = "";

  for (int i = message.length() - 1; i >= 0; i--)
  {
    reversedMessage.concat(message[i]);
  }
  return reversedMessage;
}

void displayArrows()
{
  // Display arrows
  lcd.setCursor(0, 3);
  lcd.print("<");
  lcd.setCursor(19, 3);
  lcd.print(">");
}

void armUpdateTicker()
{
  // Update data every UPDATE_TIMEOUT seconds
  updaterTicker.attach(UPDATE_TIMEOUT, updateData);
}

void armScreenSaverTicker()
{
  // Power off LCD for device and energy savings every LCD_AUTO_OFF_DELAY seconds
  screenSavingTicker.attach(LCD_AUTO_OFF_DELAY, powerOffScreen);
}

void reArmScreenSaverTicker() 
{
  Log.trace("Rearm screen saving watchdog");  
  screenSavingTicker.detach();
  armScreenSaverTicker();
}