// --- PIN DEFINITIONS ---

// Left Motor (Motor A)
const int PWMA = 19; // Speed
const int AIN1 = 18; // Direction 1
const int AIN2 = 17; // Direction 2

// Right Motor (Motor B)
const int PWMB = 16; // Speed
const int BIN1 = 21; // Direction 1
const int BIN2 = 22; // Direction 2

// --- NEW ESP32 CORE 3.X PWM SETTINGS ---
const int freq = 5000;       // 5kHz frequency is great for N20 motors
const int resolution = 8;    // 8-bit resolution (0 to 255 speed)

// Test speed (0 to 255). 150 is safe for testing on a bench.
const int testSpeed = 150; 

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("--- Core 3.0 Dragster Motor Diagnostic Starting ---");
  Serial.println("REMINDER: Make sure the STBY pin is wired to 3.3V!");

  // 1. Set Direction Pins as Outputs
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);

  // 2. The New Core 3.x Way: Attach PWM directly to the pin (No more channels!)
  ledcAttach(PWMA, freq, resolution);
  ledcAttach(PWMB, freq, resolution);

  // Ensure motors are stopped at boot
  stopMotors();
  delay(2000);
}

void loop() {
  // --- TEST 1: LEFT MOTOR FORWARD ---
  Serial.println("Testing: LEFT Motor (A) -> FORWARD");
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);
  ledcWrite(PWMA, testSpeed); // Note: We now write to the PIN, not a channel
  delay(2000);
  stopMotors();
  delay(1000);

  // --- TEST 2: LEFT MOTOR REVERSE ---
  Serial.println("Testing: LEFT Motor (A) -> REVERSE");
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  ledcWrite(PWMA, testSpeed);
  delay(2000);
  stopMotors();
  delay(1000);

  // --- TEST 3: RIGHT MOTOR FORWARD ---
  Serial.println("Testing: RIGHT Motor (B) -> FORWARD");
  digitalWrite(BIN1, HIGH);
  digitalWrite(BIN2, LOW);
  ledcWrite(PWMB, testSpeed);
  delay(2000);
  stopMotors();
  delay(1000);

  // --- TEST 4: RIGHT MOTOR REVERSE ---
  Serial.println("Testing: RIGHT Motor (B) -> REVERSE");
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, HIGH);
  ledcWrite(PWMB, testSpeed);
  delay(2000);
  stopMotors();
  
  Serial.println("Test cycle complete. Restarting in 3 seconds...");
  Serial.println("-----------------------------------");
  delay(3000);
}

// Custom function to hard-brake both motors
void stopMotors() {
  // Setting both IN pins HIGH causes the TB6612 to short the motor leads together,
  // which acts as an electronic brake. Perfect for sharp S-bends!
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, HIGH);
  digitalWrite(BIN2, HIGH);
  
  // Write 0 speed directly to the pins
  ledcWrite(PWMA, 0);
  ledcWrite(PWMB, 0);
}