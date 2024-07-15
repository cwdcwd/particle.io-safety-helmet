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

float readTemperature();
void setColor(bool red, bool green, bool blue);
void cycleColors();

// CONSTANTS
const float OVERHEAT_THRESHOLD = 28.0; // 37.5; // Define the temperature threshold for overheating
const float SPIKE_THRESHOLD = 1.5; // Define how much change constitutes a spike
const int READ_INTERVAL_MS = 1000;
const int WINDOW_SIZE = 10; // Number of readings to include in the moving average

const int PIN_RED = A3;
const int PIN_GREEN = A2;
const int PIN_BLUE = A1;

MAX30205 tempSensor;

// Let Device OS manage the connection to the Particle Cloud
SYSTEM_MODE(AUTOMATIC);
// Run the application and system concurrently in separate threads
SYSTEM_THREAD(ENABLED);
// Show system, cloud connectivity, and application logs over USB
// View logs with CLI using 'particle serial monitor --follow'
SerialLogHandler logHandler(LOG_LEVEL_ERROR);

// GLOBAL VARIABLES
int readIndex = 0;
int iColor = 0;
float total = 0.0;
float temperatureReadings[WINDOW_SIZE];

void setup()
{
  Serial.begin(9600);
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
  if (abs(currentTemperature - movingAverage) >= SPIKE_THRESHOLD)
  {
    Serial.println("Spike Detected!");

    for(byte b = 0; b<3; ++b) { //CWD-- flash the LED 3 times
      setColor(true, false, true);
      delay(100);
    }

    setColor(false, false, false);
    // Handle the spike (e.g., alarms, notifications)
  }

  if (currentTemperature > OVERHEAT_THRESHOLD)
  {
    setColor(true, false, false);
  }
  else
  {
    setColor(false, false, false);
  }

  delay(READ_INTERVAL_MS);
}

float readTemperature() {
  float tempC = tempSensor.getTemperature();
  // convert temp from C to F
  float tempF = tempC * 1.8 + 32;
  Serial.print(tempC, 2);
  Serial.print("`C / ");
  Serial.print(tempF, 2);
  Serial.println("`F");
  return tempC;
}

void setColor(bool red, bool green, bool blue) {
  digitalWrite(PIN_RED, red?HIGH:LOW );
  digitalWrite(PIN_GREEN, green?HIGH:LOW);
  digitalWrite(PIN_BLUE, blue?HIGH:LOW);
}

void cycleColors() {
  // Cycle through colors
  for(byte b = 0; b < 8; b++) {
    setColor(b & 1, b & 2, b & 4);
    delay(500);
  }
}