/*
 * Project: MAX30205 Temperature Sensor
 * Author: Lazybaer
 * Date: 2024-07-14
 * For comprehensive documentation and examples, please visit:
 * https://docs.particle.io/firmware/best-practices/firmware-template/
 *
 * This program Prints temperature on terminal
 *
 * Hardware Connections (Breakoutboard to Arduino):
 * Vin  - 5V (3.3V is allowed)
 * GND - GND
 * SDA - A4 (or SDA)
 * SCL - A5 (or SCL)
*/
// Include Particle Device OS APIs
#include "Particle.h"
#include <Wire.h>
#include "MAX30205.h"

// FUNCTION DECLARATIONS
double readTemperature();
String readTemperatureAsString();

int setColor(bool red, bool green, bool blue);
int setColorByInt(int color);
int setColorByString(String strRGB);

void cycleColors();
int cycleColorsCmd(String command);

// CONSTANTS
const int WINDOW_SIZE = 10; // Number of readings to include in the moving average

const int PIN_RED = A3;
const int PIN_GREEN = A2;
const int PIN_BLUE = A1;

// Create an instance of the MAX30205 class
MAX30205 tempSensor;

// Let Device OS manage the connection to the Particle Cloud
SYSTEM_MODE(AUTOMATIC);
// Run the application and system concurrently in separate threads
SYSTEM_THREAD(ENABLED);
// Show system, cloud connectivity, and application logs over USB
// View logs with CLI using 'particle serial monitor --follow'
SerialLogHandler logHandler(LOG_LEVEL_ERROR);

// GLOBAL VARIABLES
double overheatThreshold = 37.5; // Define the temperature threshold for overheating
double spikeThreshold = 1.5; // Define how much change constitutes a spike

unsigned long READ_INTERVAL_MS = 1000;
unsigned long lastRead = 0;
int readIndex = 0;
int currentColor = 0;
double total = 0.0;
double temperatureReadings[WINDOW_SIZE];


void setup()
{
  Serial.begin(9600);
  Particle.variable("READ_INTERVAL_MS", &READ_INTERVAL_MS, INT);
  Particle.variable("OVERHEAT_THRESHOLD", &overheatThreshold, DOUBLE);
  Particle.variable("SPIKE_THRESHOLD", &spikeThreshold, DOUBLE);
  Particle.variable("TEMPERATURE", readTemperatureAsString);
  Particle.variable("CURRENT_COLOR", &currentColor, INT);
  // Particle.function("setColor", setColor);
  Particle.function("cycleColors", cycleColorsCmd);
  Particle.function("setColor", setColorByString);

  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_BLUE, OUTPUT);
  cycleColors();
  setColor(false, false, false);
  Wire.begin();

  // Initialize all readings to 0
  for (int i = 0; i < WINDOW_SIZE; i++)
  {
    temperatureReadings[i] = 0.0;
  }
  // tempSensor.scanAvailableSensors();
  tempSensor.begin(Wire, true, 0x90);

  // scan for temperature in every 30 sec untill a sensor is found. Scan for both addresses 0x48 and 0x49
  while (!tempSensor.scanAvailableSensors())
  {
    Serial.println("Couldn't find the temperature sensor, please connect the sensor.");
    delay(5000);
  }

  // set continuos mode, active mode
}

void loop()
{
  if((millis() - lastRead) >= READ_INTERVAL_MS) {
    lastRead = millis();
    float currentTemperature = readTemperature();

    // Update the total by subtracting the old reading and adding the new one
    total = total - temperatureReadings[readIndex];
    temperatureReadings[readIndex] = currentTemperature;
    total = total + temperatureReadings[readIndex];

    // Advance to the next position in the array
    readIndex = (readIndex + 1) % WINDOW_SIZE;

    // Calculate the moving average
    float movingAverage = total / WINDOW_SIZE;

    // Print the current temperature and moving average for monitoring purposes
    Serial.print("Current Temperature: ");
    Serial.print(currentTemperature);
    Serial.print(" C, Moving Average: ");
    Serial.print(movingAverage);
    Serial.println(" C");

    // Check for a spike
    if (abs(currentTemperature - movingAverage) >= spikeThreshold)
    {
      Serial.println("Spike Detected!");

      for(byte b = 0; b<3; ++b) { //CWD-- flash the LED 3 times
        setColor(true, false, true);
        delay(100);
      }

      // currentColor = 0;
      // Handle the spike (e.g., alarms, notifications)
    }

    if (currentTemperature > overheatThreshold)
    {
      Serial.println("Overheating Detected!");
      currentColor = 1;
    }
    // else
    // {
    //   currentColor = 0;
    // }
  }

  setColorByInt(currentColor);
}

double readTemperature()
{
  double tempC = tempSensor.getTemperature();
  // convert temp from C to F
  double tempF = tempC * 1.8 + 32;
  Serial.print(tempC, 2);
  Serial.print("`C / ");
  Serial.print(tempF, 2);
  Serial.println("`F");
  return tempC;
}

String readTemperatureAsString() {
  return String(readTemperature());
}

int setColor(bool red, bool green, bool blue) {
  // Serial.print(red ? "R+" : "R-");
  // Serial.print(green ? "G+" : "G-");
  // Serial.println(blue ? "B+" : "B-");

  digitalWrite(PIN_RED, red?HIGH:LOW );
  digitalWrite(PIN_GREEN, green?HIGH:LOW);
  digitalWrite(PIN_BLUE, blue?HIGH:LOW);

  return (red?1:0) + (green?2:0) + (blue?4:0);
}

int setColorByInt(int color) {
  return setColor(color & 1, color & 2, color & 4);
}

int setColorByString(String strRGB) {
  strRGB = strRGB.toLowerCase();
  Serial.println("Setting color to "+strRGB);

  if(strRGB == "red") {
    currentColor = setColor(true, false, false);
  } else if(strRGB == "green") {
    currentColor = setColor(false, true, false);
  } else if(strRGB == "blue") {
    currentColor = setColor(false, false, true);
  } else if(strRGB == "yellow") {
    currentColor = setColor(true, true, false);
  } else if(strRGB == "cyan") {
    currentColor = setColor(false, true, true);
  } else if(strRGB == "magenta") {
    currentColor = setColor(true, false, true);
  } else if(strRGB == "white") {
    currentColor = setColor(true, true, true);
  } else {
    currentColor = setColor(false, false, false);
  }

  return currentColor;
}

void cycleColors() {
  // Cycle through colors
  for(byte b = 0; b < 8; b++) {
    setColor(b & 1, b & 2, b & 4);
    delay(500);
  }
}

int cycleColorsCmd(String command) {
  cycleColors();
  return 1;
}