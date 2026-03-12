#include <QTRSensors.h>

QTRSensors qtr;

const uint8_t SensorCount = 8;
uint16_t rawValues[SensorCount]; // Stores the raw discharge time (in microseconds)
uint16_t calValues[SensorCount]; // Stores the calibrated values (0 to 1000)

void setup() {
  Serial.begin(115200); // Fast baud rate for ESP32

  // Configure the 8 safe bidirectional pins
  qtr.setTypeRC();
  qtr.setSensorPins((const uint8_t[]){13, 14, 27, 26, 25, 33, 32, 23}, SensorCount);

  // You can adjust this timeout. 2500 is standard. 
  // If your line is very black, you can lower it to 1000 to make the loop run faster.
  qtr.setTimeout(2500); 

  Serial.println("Starting Calibration... Sweep the robot over the line!");
  delay(1000);

  // Turn on ESP32 onboard LED to show calibration is happening (Usually GPIO 2)
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);

  // Calibration Loop (Takes about 10 seconds)
  // You MUST slide the sensors back and forth over the black line and white background
  for (uint16_t i = 0; i < 400; i++) {
    qtr.calibrate();
  }
  
  digitalWrite(2, LOW); // Calibration done
  Serial.println("Calibration Complete!");
  delay(1000);
}

void loop() {
  // 1. Get the RAW discharge times (for your own testing/curiosity)
  // This will be a number between 0 and your Timeout (e.g., 2500)
  qtr.read(rawValues);

  // 2. Get the POSITION value (0 to 7000)
  // This also updates 'calValues' with the mapped 0-1000 numbers
  // 0 = Line is under sensor 1. 7000 = Line is under sensor 8. 3500 = Perfectly centered.
  uint16_t position = qtr.readLineBlack(calValues);

  // --- PRINTING TO SERIAL MONITOR ---
  
  // Print Raw Values
  Serial.print("RAW: ");
  for (uint8_t i = 0; i < SensorCount; i++) {
    Serial.print(rawValues[i]);
    Serial.print('\t'); // Tab spacing for clean columns
  }

  // Print the calculated Position
  Serial.print(" | POS: ");
  Serial.println(position);

  // Run the loop fast, but add a tiny delay so the Serial Monitor doesn't crash your PC
  delay(50); 
}