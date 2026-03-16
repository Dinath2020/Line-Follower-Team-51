#include <WiFi.h>
#include <WebServer.h>
#include <QTRSensors.h>

// --- 1. WI-FI SETTINGS ---
const char* ssid = "Dragster_Pro";     
const char* password = "password123";  
WebServer server(80);

// --- 2. SENSOR & MOTOR SETTINGS ---
QTRSensors qtr;
const uint8_t SensorCount = 8;
uint16_t sensorValues[SensorCount];

const int PWMA = 19; const int AIN1 = 18; const int AIN2 = 4; // Left Motor
const int PWMB = 15; const int BIN1 = 21; const int BIN2 = 22; // Right Motor

// --- 3. ROBOT STATE VARIABLES ---
bool isAutoRunning = false; 
float Kp = 0.08; 
float Kd = 0.50;  
float Ki = 0.00;
int baseSpeed = 60;
int maxSpeed = 127;
int lastError = 0;
int integral = 0;

// RC Speeds
int rcSpeed = 127; 
const int halfRC = 127; 
const int fullRC = 255;

// --- 4. THE WEB INTERFACE (HTML/CSS/JS) ---
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
  <title>Dragster Pro</title>
  <style>
    body { font-family: Arial, sans-serif; background-color: #1e1e1e; color: #fff; text-align: center; margin: 0; }
    .tab-container { display: flex; background: #333; }
    .tab { flex: 1; padding: 12px 5px; cursor: pointer; font-weight: bold; font-size: 14px; }
    .tab.active { background: #007bff; }
    .content { display: none; padding: 15px; }
    .content.active { display: block; }
    
    /* Buttons */
    button { background: #007bff; color: white; border: none; padding: 12px 15px; border-radius: 8px; font-size: 16px; margin: 5px 0; cursor: pointer; width: 100%; }
    button:active { background: #0056b3; }
    .btn-stop { background: #dc3545; font-size: 20px; font-weight: bold; padding: 20px; }
    .btn-cal { background: #ffc107; color: black; font-weight: bold; }
    .btn-start { background: #28a745; font-size: 20px; font-weight: bold; padding: 20px; }
    .btn-manual { background: #fd7e14; }
    .btn-reset { background: #6c757d; font-size: 16px; padding: 10px; width: 48%; }
    
    /* Timer Display */
    .timer-box { background: #000; color: #0f0; font-family: monospace; font-size: 36px; padding: 15px; border-radius: 8px; margin: 10px 0; border: 2px solid #333; }

    /* Inputs */
    .input-group { margin: 10px 0; text-align: left; background: #2c2c2c; padding: 15px; border-radius: 8px; }
    input[type=number] { width: 65px; padding: 5px; background: #444; color: white; border: 1px solid #666; font-weight: bold;}
    .live-status { color: #0f0; font-size: 12px; margin-bottom: 5px; }
    
    /* Sensor Data Box */
    .data-box { background: #111; padding: 10px; font-family: monospace; font-size: 12px; text-align: left; overflow-x: auto; color: #0f0; border-left: 4px solid #007bff; margin-top: 10px;}
    
    /* Manual Calibration Grid */
    .cal-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; font-size: 12px; margin-top: 10px; margin-bottom: 10px;}
    .cal-grid input { width: 45px; font-size: 11px; padding: 4px;}
    
    /* RC Grid */
    .rc-grid { display: grid; grid-template-columns: 1fr 1fr 1fr; gap: 10px; margin-top: 20px; }
    .rc-btn { padding: 30px 0; font-size: 20px; touch-action: manipulation;}
  </style>
</head>
<body>

  <div class="tab-container">
    <div class="tab active" onclick="switchTab('auto')">Race & Tune</div>
    <div class="tab" onclick="switchTab('calib')">Calibration</div>
    <div class="tab" onclick="switchTab('rc')">Manual RC</div>
  </div>

  <div id="auto" class="content active">
    <div id="timerDisplay" class="timer-box">00:00.000</div>
    
    <button class="btn-start" onclick="sendReq('/startAuto'); startTimer();">START RACING</button>
    <button class="btn-stop" onclick="sendReq('/stopAuto'); stopTimer();">STOP / E-BRAKE</button>
    
    <div style="text-align: right;">
      <button class="btn-reset" onclick="resetTimer()">Reset Timer</button>
    </div>
    
    <div class="input-group">
      <div class="live-status">✔ Synced with Robot's Live Memory</div>
      <b>Working PID & Speeds:</b><br><br>
      Kp: <input type="number" step="0.01" id="kp"> 
      Kd: <input type="number" step="0.1" id="kd"> 
      Ki: <input type="number" step="0.001" id="ki"><br><br>
      Base Speed: <input type="number" id="bs"> 
      Max Speed: <input type="number" id="ms"><br><br>
      <button onclick="saveTuning()">Push Settings to Car</button>
    </div>
  </div>

  <div id="calib" class="content">
    <div class="input-group">
      <b>1. Auto-Calibration:</b>
      <button id="calBtn" class="btn-cal" onclick="runCalibration()">Run 180° Sweep</button>
    </div>

    <div class="input-group">
      <b>2. Manual Calibration Override:</b>
      <div id="calGrid" class="cal-grid"></div>
      <button class="btn-manual" onclick="saveManualCal()">Push Manual Cal to Car</button>
    </div>

    <div class="input-group">
      <button onclick="checkSensors()" style="background: #17a2b8;">Take Manual Sensor Snapshot</button>
      <div id="sensorData" class="data-box">Calibrate or Check Sensors to view live data...</div>
    </div>
  </div>

  <div id="rc" class="content">
    <button class="btn-stop" onclick="sendReq('/stopAuto'); stopTimer();">Force Stop Auto Mode</button>
    <br><br>
    <div style="display:flex; gap:10px;">
      <button onclick="sendReq('/speed?mode=half')" style="background: #28a745;">Half Speed</button>
      <button onclick="sendReq('/speed?mode=full')" style="background: #dc3545;">FULL SPEED</button>
    </div>
    
    <div class="rc-grid">
      <div></div><button class="rc-btn" id="btnF">FWD</button><div></div>
      <button class="rc-btn" id="btnL">LEFT</button><button class="rc-btn" id="btnB">REV</button><button class="rc-btn" id="btnR">RIGHT</button>
    </div>
  </div>

  <script>
    // --- TIMER LOGIC START ---
    let startTime;
    let elapsedTime = 0;
    let timerInterval;
    let timerRunning = false;

    function startTimer() {
      if (!timerRunning) {
        startTime = Date.now() - elapsedTime;
        timerInterval = setInterval(updateTimer, 10);
        timerRunning = true;
      }
    }

    function stopTimer() {
      if (timerRunning) {
        clearInterval(timerInterval);
        timerRunning = false;
      }
    }

    function resetTimer() {
      clearInterval(timerInterval);
      timerRunning = false;
      elapsedTime = 0;
      document.getElementById("timerDisplay").innerText = "00:00.000";
    }

    function updateTimer() {
      elapsedTime = Date.now() - startTime;
      let diff = new Date(elapsedTime);
      let m = String(diff.getUTCMinutes()).padStart(2, '0');
      let s = String(diff.getUTCSeconds()).padStart(2, '0');
      let ms = String(diff.getUTCMilliseconds()).padStart(3, '0');
      document.getElementById("timerDisplay").innerText = m + ":" + s + "." + ms;
    }
    // --- TIMER LOGIC END ---


    function switchTab(tabId) {
      document.querySelectorAll('.content').forEach(el => el.classList.remove('active'));
      document.querySelectorAll('.tab').forEach(el => el.classList.remove('active'));
      document.getElementById(tabId).classList.add('active');
      event.target.classList.add('active');
    }

    function sendReq(url) { fetch(url); }

    function buildCalGrid() {
      let grid = document.getElementById('calGrid');
      let html = '';
      for(let i=0; i<8; i++){
        html += `<div>S${i}: <input id="min${i}" type="number" placeholder="Min"> <input id="max${i}" type="number" placeholder="Max"></div>`;
      }
      grid.innerHTML = html;
    }

    function syncLiveSettings() {
      fetch('/getPID')
        .then(response => response.json())
        .then(data => {
          document.getElementById('kp').value = data.p;
          document.getElementById('kd').value = data.d;
          document.getElementById('ki').value = data.i;
          document.getElementById('bs').value = data.bs;
          document.getElementById('ms').value = data.ms;
        });
      
      fetch('/getSensors')
        .then(response => response.json())
        .then(data => {
          for(let i=0; i<8; i++){
            document.getElementById('min'+i).value = data.min[i];
            document.getElementById('max'+i).value = data.max[i];
          }
        });
    }

    function saveTuning() {
      let p = document.getElementById('kp').value;
      let d = document.getElementById('kd').value;
      let i = document.getElementById('ki').value;
      let b = document.getElementById('bs').value;
      let m = document.getElementById('ms').value;
      fetch(`/setPID?p=${p}&d=${d}&i=${i}&bs=${b}&ms=${m}`).then(() => {
        syncLiveSettings(); 
        alert("PID & Speed Settings Active!");
      });
    }

    function saveManualCal() {
      let url = '/setCal?';
      for(let i=0; i<8; i++){
        let minVal = document.getElementById('min'+i).value;
        let maxVal = document.getElementById('max'+i).value;
        url += `min${i}=${minVal}&max${i}=${maxVal}&`;
      }
      fetch(url).then(() => {
        alert("Manual Calibration Overwritten!");
        checkSensors(); 
      });
    }

    function runCalibration() {
      let btn = document.getElementById('calBtn');
      btn.innerText = "Calibrating... (Wait 3 sec)";
      btn.style.backgroundColor = "#dc3545"; 
      btn.style.color = "white";
      
      fetch('/calibrate').then(() => {
        btn.innerText = "Run 180° Sweep";
        btn.style.backgroundColor = "#ffc107"; 
        btn.style.color = "black";
        checkSensors(); 
        syncLiveSettings(); 
      });
    }

    function checkSensors() {
      fetch('/getSensors')
        .then(response => response.json())
        .then(data => {
          let html = "<b>Live Snapshot:</b><br>";
          html += "Position: " + data.pos + "<br>";
          html += "Raw Data: [" + data.raw.join(", ") + "]<br><br>";
          html += "<b>Cal Min:  [" + data.min.join(", ") + "]</b><br>";
          html += "<b>Cal Max:  [" + data.max.join(", ") + "]</b><br>";
          document.getElementById('sensorData').innerHTML = html;
        });
    }

    function setupBtn(id, cmd) {
      let btn = document.getElementById(id);
      btn.addEventListener('touchstart', e => { e.preventDefault(); sendReq('/rc?cmd=' + cmd); });
      btn.addEventListener('touchend', e => { e.preventDefault(); sendReq('/stopAuto'); });
      btn.addEventListener('mousedown', e => { e.preventDefault(); sendReq('/rc?cmd=' + cmd); });
      btn.addEventListener('mouseup', e => { e.preventDefault(); sendReq('/stopAuto'); });
    }
    
    window.onload = function() {
      setupBtn('btnF', 'fwd'); setupBtn('btnB', 'rev'); 
      setupBtn('btnL', 'left'); setupBtn('btnR', 'right');
      buildCalGrid();
      syncLiveSettings(); 
    }
  </script>
</body>
</html>
)rawliteral";

// --- 5. SERVER ROUTING FUNCTIONS ---
void handleGetPID() {
  String json = "{\"p\":" + String(Kp, 3) + ", \"d\":" + String(Kd, 3) + ", \"i\":" + String(Ki, 3) + ", \"bs\":" + String(baseSpeed) + ", \"ms\":" + String(maxSpeed) + "}";
  server.send(200, "application/json", json);
}

void handleSensors() {
  uint16_t pos = qtr.readLineBlack(sensorValues);
  String json = "{\"pos\":" + String(pos) + ", \"raw\":[";
  for(int i=0; i<8; i++) { json += String(sensorValues[i]) + (i<7?",":""); }
  json += "], \"min\":[";
  for(int i=0; i<8; i++) { json += String(qtr.calibrationOn.minimum[i]) + (i<7?",":""); }
  json += "], \"max\":[";
  for(int i=0; i<8; i++) { json += String(qtr.calibrationOn.maximum[i]) + (i<7?",":""); }
  json += "]}";
  server.send(200, "application/json", json);
}

void handleCalibrate() {
  isAutoRunning = false;
  unsigned long startT = millis();
  
  setMotorSpeed(60, -60);
  while(millis() - startT < 800) { qtr.calibrate(); }
  setMotorSpeed(-60, 60);
  while(millis() - startT < 2400) { qtr.calibrate(); }
  setMotorSpeed(60, -60);
  while(millis() - startT < 3200) { qtr.calibrate(); }
  
  setMotorSpeed(0, 0);
  server.send(200, "text/plain", "Calibrated");
}

void handleSetPID() {
  if(server.hasArg("p")) Kp = server.arg("p").toFloat();
  if(server.hasArg("d")) Kd = server.arg("d").toFloat();
  if(server.hasArg("i")) Ki = server.arg("i").toFloat();
  if(server.hasArg("bs")) baseSpeed = server.arg("bs").toInt();
  if(server.hasArg("ms")) maxSpeed = server.arg("ms").toInt();
  server.send(200, "text/plain", "OK");
}

void handleSetCal() {
  for(int i=0; i<8; i++) {
    String minKey = "min" + String(i);
    String maxKey = "max" + String(i);
    if(server.hasArg(minKey)) qtr.calibrationOn.minimum[i] = server.arg(minKey).toInt();
    if(server.hasArg(maxKey)) qtr.calibrationOn.maximum[i] = server.arg(maxKey).toInt();
  }
  server.send(200, "text/plain", "OK");
}

void handleRC() {
  isAutoRunning = false; 
  String cmd = server.arg("cmd");
  if(cmd == "fwd") setMotorSpeed(rcSpeed, rcSpeed);
  else if(cmd == "rev") setMotorSpeed(-rcSpeed, -rcSpeed);
  else if(cmd == "left") setMotorSpeed(-rcSpeed, rcSpeed);
  else if(cmd == "right") setMotorSpeed(rcSpeed, -rcSpeed);
  server.send(200, "text/plain", "OK");
}

// --- 6. CORE SETUP ---
void setup() {
  Serial.begin(115200);
  
  qtr.setTypeRC();
  qtr.setSensorPins((const uint8_t[]){13, 14, 27, 26, 25, 33, 32, 23}, SensorCount);
  qtr.setTimeout(2500);

  // Allocate memory to prevent crashes
  qtr.calibrate(); 
  for(int i=0; i<SensorCount; i++) {
    qtr.calibrationOn.minimum[i] = 200;  
    qtr.calibrationOn.maximum[i] = 2000; 
  }

  pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);
  ledcAttach(PWMA, 5000, 8);
  ledcAttach(PWMB, 5000, 8);
  setMotorSpeed(0, 0); 

  WiFi.softAP(ssid, password);
  Serial.println("Wi-Fi Hotspot Started!");

  server.on("/", [](){ server.send(200, "text/html", htmlPage); });
  server.on("/startAuto", [](){ isAutoRunning = true; server.send(200, "text/plain", "OK"); });
  server.on("/stopAuto", [](){ isAutoRunning = false; setMotorSpeed(0,0); server.send(200, "text/plain", "OK"); });
  server.on("/calibrate", handleCalibrate);
  server.on("/getSensors", handleSensors);
  server.on("/getPID", handleGetPID); 
  server.on("/setPID", handleSetPID);
  server.on("/setCal", handleSetCal); 
  server.on("/rc", handleRC);
  server.on("/speed", [](){ rcSpeed = (server.arg("mode")=="full") ? fullRC : halfRC; server.send(200, "text/plain", "OK"); });
  
  server.begin();
}

// --- 7. MAIN LOOP ---
void loop() {
  server.handleClient();

  if (isAutoRunning) {
    uint16_t position = qtr.readLineBlack(sensorValues);
    
    // --- DASHED LINE GAP DETECTION ENGINE ---
    bool seesLine = false;
    for(int i = 0; i < SensorCount; i++) {
      // The QTR library calibrates data from 0 (White) to 1000 (Black).
      // If any sensor reads higher than 200, we know a line exists beneath the car.
      if (sensorValues[i] > 200) { 
        seesLine = true;
        break;
      }
    }

    int error;
    if (seesLine) {
      // We are on a solid line. Calculate the error normally.
      error = position - 3500;
    } else {
      // GAP DETECTED! We are over the white space of a dashed line.
      // Ignore the crazy position reading, and pretend the error hasn't changed.
      // This prevents the Kd shock-absorber from violently flinching.
      error = lastError; 
    }
    // ----------------------------------------

    integral += error;

    int motorSpeedAdjustment = (Kp * error) + (Kd * (error - lastError)) + (Ki * integral);
    lastError = error;

    int leftMotorSpeed = baseSpeed - motorSpeedAdjustment;
    int rightMotorSpeed = baseSpeed + motorSpeedAdjustment;

    if (leftMotorSpeed > maxSpeed) leftMotorSpeed = maxSpeed;
    if (rightMotorSpeed > maxSpeed) rightMotorSpeed = maxSpeed;
    if (leftMotorSpeed < -maxSpeed) leftMotorSpeed = -maxSpeed; 
    if (rightMotorSpeed < -maxSpeed) rightMotorSpeed = -maxSpeed;

    setMotorSpeed(leftMotorSpeed, rightMotorSpeed);
  }
}

// --- THE MASTER STEERING FUNCTION ---
void setMotorSpeed(int leftSpeed, int rightSpeed) {
  if (leftSpeed >= 0) {
    digitalWrite(AIN1, LOW); digitalWrite(AIN2, HIGH); ledcWrite(PWMA, leftSpeed);
  } else {
    digitalWrite(AIN1, HIGH); digitalWrite(AIN2, LOW); ledcWrite(PWMA, -leftSpeed); 
  }
  if (rightSpeed >= 0) {
    digitalWrite(BIN1, LOW); digitalWrite(BIN2, HIGH); ledcWrite(PWMB, rightSpeed);
  } else {
    digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW); ledcWrite(PWMB, -rightSpeed);
  }
}