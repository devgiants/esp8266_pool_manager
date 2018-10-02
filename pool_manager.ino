/****************************
 * Includes
 */
#include <string.h>
#include <Ticker.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>


/****************************
 * Define
 */

#define LOG_SERIAL_PREFIX "Pool Manager : "
 
// I2C communication definition
#define SDA 0
#define SCL 2


// LCD definition
#define LCD_ADDRESS 0x27
#define LCD_ROWS_NUMBER 4
#define LCD_COLS_NUMBER 20
#define CENTER "CENTER"
#define RIGHT "RIGHT"

#define CELSIUS_DEGREE_SYMBOL 0

// OneWire definition
#define ONE_WIRE_BUS 4
#define MIN_TEMPERATURE_PRECISION 9
#define MAX_TEMPERATURE_PRECISION 12

// GPIO Pin definition
#define FILTRATION_PUMP 12
#define HEATING_PUMP 13

// Time definition
#define UPDATE_TIMEOUT 10


/****************************
 * Constants
 */
const String updateMessage = "MAJ...";
const String version = "V1.0"; 


// Main objects
LiquidCrystal_I2C lcd(LCD_ADDRESS,LCD_COLS_NUMBER,LCD_ROWS_NUMBER);
Ticker updaterTicker;
OneWire ow(ONE_WIRE_BUS);
DallasTemperature dallasSensors(&ow);


DeviceAddress waterTempSensorAddress = { 0x28, 0xCA, 0x98, 0xCF, 0x05, 0x0, 0x0, 0x51 };
DeviceAddress heatWaterSystemTempSensorAddress   = { 0x28, 0xC4, 0xA8, 0xCF, 0x05, 0x0, 0x0, 0xC6 };

DeviceAddress* dallasSensorsAddresses[] = { 
  &waterTempSensorAddress,                // ID 0
  &heatWaterSystemTempSensorAddress       // ID 1
};

// Variables
bool filtrationPumpValue = HIGH;
bool heatingPumpValue = HIGH;

uint8_t celsiusDegreeSymbol[8] = {0x8,0xf4,0x8,0x43,0x4,0x4,0x43,0x0};

float waterTemperature, heatSystemWaterTemperature;

/**
 * Setup device an initiates everything
 */
void setup() {

  // Initiates serial connection
  Serial.begin(9600);
  
  // Initiates I2C connection
  printLog("Init I2C connection.");
  Wire.begin(SDA, SCL);
  

  // Initiates DS sensors
  printLog("Initiates Dallas sensors.");
  dallasSensors.begin();

  // Init LCD
  printLog("Initiates LCD screen.");
  lcd.init();

  // Create custom chars and assign them to slots
  lcd.createChar(CELSIUS_DEGREE_SYMBOL, celsiusDegreeSymbol);
  
  // Print startup message
  lcd.backlight();
  lcd.setCursor(3,0);
  lcd.print("SYSTEME DE");
  lcd.setCursor(2,1);
  lcd.print("GESTION  PISCINE");  
  lcd.setCursor(16,3);   
  lcd.print(version);

  delay(2000);

  lcd.clear();
  

  // Display arrows
  lcd.setCursor(0,3);
  lcd.print("<");
  lcd.setCursor(19,3);
  lcd.print(">");

  pinMode(FILTRATION_PUMP, OUTPUT);
  pinMode(HEATING_PUMP, OUTPUT);

  // Update data every UPDATE_TIMEOUT seconds
  updaterTicker.attach(UPDATE_TIMEOUT, updateData);

  if (!dallasSensors.getAddress(waterTempSensorAddress, 0)) printLog("Unable to find address for water temperature sensor.");
  if (!dallasSensors.getAddress(heatWaterSystemTempSensorAddress, 1)) printLog("Unable to find address for heating system water temperature sensor.");

  // set the resolution to 9 bit per device
  printLog("Set temperature sensors to given precision");
  dallasSensors.setResolution(waterTempSensorAddress, MAX_TEMPERATURE_PRECISION);
  dallasSensors.setResolution(heatWaterSystemTempSensorAddress, MIN_TEMPERATURE_PRECISION); 

  // Launch first data update
  updateData();
}


// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

/**
 * Loop function
 */
void loop() {
  
}


void updateData() {
  // Display update message
  displayUpdateMessage("MAJ...");

  // Update OneWire sensors data
  printLog("UPDATE DATA - Request temperatures");
  dallasSensors.requestTemperatures();

  waterTemperature = dallasSensors.getTempC(waterTempSensorAddress);
  heatSystemWaterTemperature = dallasSensors.getTempC(heatWaterSystemTempSensorAddress);
  
  Serial.println("Water temperature : " + String(waterTemperature));
  Serial.println("Heating system water temperature: " + String(heatSystemWaterTemperature));

  lcd.setCursor(0,0);
  lcd.print("Tp: " + String(waterTemperature, 1) + (char)CELSIUS_DEGREE_SYMBOL);

  lcd.rightToLeft();
  lcd.setCursor(LCD_COLS_NUMBER-1,0); 
  lcd.print(reverse("Tc: " + String(heatSystemWaterTemperature, 0) + (char)CELSIUS_DEGREE_SYMBOL));
  lcd.leftToRight();
    
  digitalWrite(FILTRATION_PUMP, filtrationPumpValue);
  digitalWrite(HEATING_PUMP, heatingPumpValue);

  filtrationPumpValue = !filtrationPumpValue;
  heatingPumpValue = !heatingPumpValue; 

  // Remove update message
  displayUpdateMessage("      ");
}


/**
 * Function to calculate spaces to set for center/right alignment 
 */
int calculateIndent(String stringToDisplay, String mode = CENTER) {
  // Only if string is smaller than one line
  if(stringToDisplay.length() <= LCD_COLS_NUMBER) {
    return (int)(LCD_COLS_NUMBER - stringToDisplay.length())/2;
  }
  // If bigger, it will be 0
  else return 0;
}

void displayUpdateMessage(String message) {
  lcd.setCursor(calculateIndent(message),3);
  lcd.print(message);
}


void printLog(String message) {
  Serial.print(LOG_SERIAL_PREFIX);
  Serial.println(message);
}

String reverse(String message) { 
  String reversedMessage = "";
    
  for(int i= message.length()-1;i>=0;i--) {        
    reversedMessage.concat(message[i]);
  }
  return reversedMessage;
}

