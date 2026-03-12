#include <WiFi.h>
#include <WebServer.h>

// --- 1. WI-FI HOTSPOT SETTINGS ---
const char* ssid = "Dragster_V1";      // Your car's Wi-Fi network name
const char* password = "abcd1234";  // The Wi-Fi password (must be 8+ characters)

WebServer server(80);

// --- 2. MOTOR PIN DEFINITIONS ---
const int PWMA = 19; const int AIN1 = 18; const int AIN2 = 17; // Left Motor (A)
const int PWMB = 16; const int BIN1 = 21; const int BIN2 = 22; // Right Motor (B)

// --- 3. SPEED SETTINGS ---
int currentSpeed = 127;     // Starts at Half Speed by default for safety
const int halfSpeed = 127;  // 50% Power
const int fullSpeed = 255;  // 100% Power

// --- 4. THE WEBPAGE (HTML/CSS/JS) ---
// This creates the mobile-friendly buttons on your phone
const char htmlPage[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
  <title>Dragster Remote</title>
  <style>
    body { font-family: Arial, sans-serif; text-align: center; background-color: #222; color: white; margin-top: 20px; user-select: none; }
    .btn { background-color: #007bff; color: white; border: none; padding: 25px 40px; font-size: 24px; border-radius: 12px; margin: 10px; width: 140px; touch-action: manipulation; }
    .btn:active { background-color: #0056b3; }
    .speed-btn { background-color: #28a745; width: 160px; padding: 15px 20px; font-size: 20px; border-radius: 8px; border: none; color: white; margin: 5px; }
    .speed-btn:active { background-color: #1e7e34; }
    .controls { margin-top: 40px; }
  </style>
  <script>
    // Send the command to the ESP32 in the background
    function send(cmd) { fetch('/' + cmd); }

    // Make the car move ONLY when holding the button, and stop on release
    function setupButton(id, cmd) {
      let btn = document.getElementById(id);
      // Touch screens (Phones)
      btn.addEventListener('touchstart', function(e) { e.preventDefault(); send(cmd); });
      btn.addEventListener('touchend', function(e) { e.preventDefault(); send('stop'); });
      // Mouse clicks (Computers)
      btn.addEventListener('mousedown', function(e) { e.preventDefault(); send(cmd); });
      btn.addEventListener('mouseup', function(e) { e.preventDefault(); send('stop'); });
    }

    window.onload = function() {
      setupButton('btnF', 'forward');
      setupButton('btnB', 'backward');
      setupButton('btnL', 'left');
      setupButton('btnR', 'right');
    }
  </script>
</head>
<body>
  <h2>Dragster Control</h2>
  
  <div>
    <button class="speed-btn" onclick="send('half')">Half Speed</button>
    <button class="speed-btn" style="background-color: #dc3545;" onclick="send('full')">FULL SPEED</button>
  </div>

  <div class="controls">
    <button class="btn" id="btnF">FORWARD</button><br>
    <button class="btn" id="btnL">LEFT</button>
    <button class="btn" id="btnR">RIGHT</button><br>
    <button class="btn" id="btnB">REVERSE</button>
  </div>
</body>
</html>
)=====";

void setup() {
  Serial.begin(115200);
  
  // Setup Motor Pins
  pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);
  
  // Attach Core 3.0 PWM
  ledcAttach(PWMA, 5000, 8);
  ledcAttach(PWMB, 5000, 8);
  setMotorSpeed(0, 0); // Hard stop

  // Start Wi-Fi Hotspot
  Serial.println("Starting Wi-Fi Hotspot...");
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Hotspot Ready! Connect your phone to Wi-Fi: ");
  Serial.println(ssid);
  Serial.print("Then open your browser to: http://");
  Serial.println(IP);

  // Setup Web Server Routes
  server.on("/", []() { server.send(200, "text/html", htmlPage); });
  
  // Movement Commands
  server.on("/forward", []() { setMotorSpeed(currentSpeed, currentSpeed); server.send(200); });
  server.on("/backward", []() { setMotorSpeed(-currentSpeed, -currentSpeed); server.send(200); });
  server.on("/left", []() { setMotorSpeed(-currentSpeed, currentSpeed); server.send(200); }); // Spin left
  server.on("/right", []() { setMotorSpeed(currentSpeed, -currentSpeed); server.send(200); }); // Spin right
  server.on("/stop", []() { setMotorSpeed(0, 0); server.send(200); });
  
  // Speed Commands
  server.on("/half", []() { currentSpeed = halfSpeed; Serial.println("Mode: HALF SPEED"); server.send(200); });
  server.on("/full", []() { currentSpeed = fullSpeed; Serial.println("Mode: FULL SPEED"); server.send(200); });

  server.begin();
}

void loop() {
  // Constantly listen for Wi-Fi commands
  server.handleClient();
}

// --- THE MASTER STEERING FUNCTION (FIXED LOGIC) ---
void setMotorSpeed(int leftSpeed, int rightSpeed) {
  
  // === CONTROL LEFT MOTOR ===
  if (leftSpeed >= 0) {
    // FORWARD (Logic Flipped)
    digitalWrite(AIN1, LOW); 
    digitalWrite(AIN2, HIGH); 
    ledcWrite(PWMA, leftSpeed);
  } else {
    // REVERSE (Logic Flipped)
    digitalWrite(AIN1, HIGH); 
    digitalWrite(AIN2, LOW); 
    ledcWrite(PWMA, -leftSpeed); 
  }

  // === CONTROL RIGHT MOTOR ===
  if (rightSpeed >= 0) {
    // FORWARD (Logic Flipped)
    digitalWrite(BIN1, LOW); 
    digitalWrite(BIN2, HIGH); 
    ledcWrite(PWMB, rightSpeed);
  } else {
    // REVERSE (Logic Flipped)
    digitalWrite(BIN1, HIGH); 
    digitalWrite(BIN2, LOW); 
    ledcWrite(PWMB, -rightSpeed);
  }
}