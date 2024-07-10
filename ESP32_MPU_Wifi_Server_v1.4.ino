#include <Arduino.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
// #include <Wire.h>
#include <WiFiManager.h>
#include <WebServer.h>
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps612.h"
// #include <string>
// #include <iostream>
// using namespace std;

#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    #include "Wire.h"
#endif

#define TRIGGER_PIN 4

Adafruit_MPU6050 mpu;
MPU6050 mpu68;
WiFiManager wm;
WiFiManagerParameter custom_field;
WebServer server(80);

bool mpuDetected = false;
bool wm_nonblocking = true;

// RGB LED
const int Red = 13;
const int Green = 12;
const int Blue = 14;
const int LED_BUILTIN = 2;

const int motorBase = 27;

// MPU Control/Status 
bool dmpReady = false;
uint8_t devStatus;
uint16_t packetSize;
uint16_t fifoCount;
uint8_t fifoBuffer[64];

// Orientation/Motion
Quaternion q; // [w, x, y, z]
VectorInt16 aa; // Acceleration data
VectorInt16 aaReal; // Gravity-free acceleration reading
VectorInt16 aaWorld; // World-frame acceleration. Gravity free accel with orientaiton
VectorFloat gravity; // Gravity Vector
float ypr[3]; //[yaw, pitch, roll]

// MPU6050 Data
float accelerationX = 0.0;
float accelerationY = 0.0;
float accelerationZ = 0.0;

float rotationX = 0.0;
float rotationY = 0.0;
float rotationZ = 0.0;

float temperature = 0.0;

// Initial Data
float initialX = 0.0;
float initialY = 0.0;
float initialZ = 0.0;
float initialGX = 0.0;
float initialGY = 0.0;
float initialGZ = 0.0;
float initialTemp = 0.0;

const float movementThreshold = 3.5;
bool isMotion = false;

unsigned long startTime = 0;
const unsigned long interval = 10000;

// Posture var
bool isBadPosture;
String leanType;
float deviation = 0.0;

// Store the HTML page as a string
const char* htmlPage = R"(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Posture Pro</title>
  <link rel="stylesheet" href="styles.css">
</head>
<style>
    /* Default styles */
body {
  font-family: Arial, sans-serif;
  margin: 0;
  padding: 0;
  background-color: #f4f4f4;
}

header {
  background-color: #333;
  color: #fff;
  padding: 20px;
  text-align: center;
}

h1 {
  margin: 0;
  text-align: center; /* Center align title */
}

.wifi-info {
  float: left;
}

.status {
  float: right;
}

main {
  padding: 20px;
  display: flex;
  flex-wrap: wrap;
}

.posture-section {
  flex: 1;
  padding-right: 10px;
  margin-bottom: 20px;
}

#sensor-data-box {
  flex: 1;
  padding-left: 10px;
}

.data-box {
  background-color: #fff;
  border-radius: 10px;
  box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
  margin-bottom: 20px;
  padding: 20px;
}

.sensor-info {
  display: flex;
  justify-content: space-between;
  margin-bottom: 10px;
  align-items: center;
  /* padding-right: 20px; */
}

.sensor-info .label {
  flex: 1;
  font-weight: bold;
}

.sensor-info .value {
  flex: 2;
  font-weight: bold;
  margin-left: 20px;
}

.motor-log {
  background-color: #fff;
  border-radius: 10px;
  box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
  padding: 20px;
  width: 100%;
  margin-top: 20px;
}

.theme-label {
  margin-left: 10px;
}

.switch {
  position: relative;
  display: inline-block;
  width: 60px;
  height: 34px;
}

.switch input {
  opacity: 0;
  width: 0;
  height: 0;
}

.slider {
  position: absolute;
  cursor: pointer;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background-color: #ccc;
  -webkit-transition: .4s;
  transition: .4s;
  border-radius: 34px;
}

.slider:before {
  position: absolute;
  content: "";
  height: 26px;
  width: 26px;
  left: 4px;
  bottom: 4px;
  background-color: white;
  -webkit-transition: .4s;
  transition: .4s;
  border-radius: 50%;
}

input:checked + .slider {
  background-color: #2196F3;
}

input:focus + .slider {
  box-shadow: 0 0 1px #2196F3;
}

input:checked + .slider:before {
  -webkit-transform: translateX(26px);
  -ms-transform: translateX(26px);
  transform: translateX(26px);
}

.slider.round {
  border-radius: 34px;
}

.slider.round:before {
  border-radius: 50%;
}

/* Responsive styles */
@media screen and (max-width: 768px) {
  header {
    padding: 10px;
  }

  main {
    padding: 10px;
    flex-direction: column;
  }

  .data-box {
    padding: 10px;
  }

  .sensor-info {
    flex-direction: column;
    margin-right: 0;
    margin-bottom: 10px;
    width: 100%;
  }

  .sensor-info .label {
    margin-bottom: 5px;
  }
}

@media screen and (max-width: 480px) {
  .wifi-info,
  .status {
    float: none;
    display: block;
    text-align: center;
    margin-bottom: 10px;
  }

  .sensor-info .label,
  .sensor-info .value {
    font-size: 14px;
  }

  .theme-label {
    display: block;
    margin-top: 10px;
  }
}


</style>
<body>
  <header>
    <div class="wifi-info">
        <div class="wifi-info">WiFi: <span id="wifiSSID">Loading...</span>, Signal Strength: <span id="wifiSignalStrength">Loading...</span></div>
    </div>
    <h1>Posture Pro</h1>
    <div class="status" id="mpu-status">MPU6050 Status: Loading...</div>
  </header>
  <main>
    <div class = "posture-section">
      <div class = "data-box">
        <h2>Posture Status</h2>
        <div id = "posture-status">
          <div class = "posture-info" id = "posture-condition">Posture Loading...</div>
          <div class = "posture-info" id = "lean-type">Lean Type Loading...</div>
          <div class = "posture-info" id = "posture-deviation">Deviation: Loading...</div>
        </div>
      </div>
    </div>
    <div class="data-box" id = "sensor-data-box">
      <h2>MPU6050 Sensor Data</h2>
      <div class="sensor-data" id="mpu-data">
        <div class="sensor-info">
          <span class="label">Acceleration:</span>
          <span class="value">
            X: <span id="accelerationX">0</span>&emsp;&emsp;Y: <span id="accelerationY">0</span>&emsp;&emsp;Z: <span id="accelerationZ">0</span>
          </span>
        </div>
        <div class="sensor-info">
          <span class="label">Rotation:</span>
          <span class="value">
            X: <span id="rotationX">0</span>&emsp;&emsp;Y: <span id="rotationY">0</span>&emsp;&emsp;Z: <span id="rotationZ">0</span>
          </span>
        </div>
        <div class="sensor-info">
          <span class="label">Temperature:</span>
          <span class="value" id="temperature">0</span>
        </div>
      </div>
      <div class="initial-readings">
      <h2>Initial Readings</h2>
      <div class="data">
        <span class="label">Initial Acceleration:</span>
        <span class="value" id="initialAcceleration">Loading...</span>
      </div>
      <div class="data">
        <span class="label">Initial Gyro:</span>
        <span class="value" id="initialGyro">Loading...</span>
      </div>
      <div class="data">
        <span class="label">Temperature:</span>
        <span class="value" id="initialTemperature">Loading...</span>
      </div>
    </div>
    </div>
    <div class="motor-log">
      <h2>Motor Usage Log</h2>
      <ul id="motor-usage">
        <!-- Motor usage log will be dynamically populated here -->
      </ul>
    </div>
  </main>
  <footer>
    <label class="switch">
      <input type="checkbox" id="theme-toggle">
      <span class="slider round"></span>
    </label>
    <span class="theme-label">Toggle Dark Theme</span>
  </footer>
  <script>
    window.onload = function() {
        function updateSensorData(data) {
            // Update MPU6050 sensor data
            document.getElementById("accelerationX").textContent = data.accelerationX;
            document.getElementById("accelerationY").textContent = data.accelerationY;
            document.getElementById("accelerationZ").textContent = data.accelerationZ;
            document.getElementById("rotationX").textContent = data.rotationX;
            document.getElementById("rotationY").textContent = data.rotationY;
            document.getElementById("rotationZ").textContent = data.rotationZ;
            document.getElementById("temperature").textContent = data.temperature;
        }

        // Function to update initial readings
            function updateInitialReadings(data) {
                const initialAcceleration = `X = ${data.initialX.toFixed(2)} Y = ${data.initialY.toFixed(2)} Z = ${data.initialZ.toFixed(2)}`;
                const initialGyro = `X = ${data.initialGX.toFixed(2)} Y = ${data.initialGY.toFixed(2)} Z = ${data.initialGZ.toFixed(2)}`;
                const initialTemperature = data.initialTemp.toFixed(2);

                document.getElementById("initialAcceleration").textContent = initialAcceleration;
                document.getElementById("initialGyro").textContent = initialGyro;
                document.getElementById("initialTemperature").textContent = initialTemperature;
            }

            function updateWifiStatus(data) {
                // Update WiFi SSID and signal strength
                document.getElementById("wifiSSID").textContent = data.wifiSSID;
                document.getElementById("wifiSignalStrength").textContent = data.wifiSignalStrength;
            }

            function updatePostureStatus(data) {
              const postureCondition = data.isBadPosture ? "Bad Posture Detected" : "Good Posture";
              const leanData = data.leanType;
              const postureDeviation = data.deviation;

              document.getElementById("posture-condition").textContent = `Posture: ${postureCondition}`;
              document.getElementById("lean-type").textContent = `Lean Type: ${leanData}`;
              document.getElementById("posture-deviation").textContent = `Deviation: ${postureDeviation} degrees`;
             }

            function updateMpuStatus(data) {
                // Update MPU6050 connection status
                const mpuStatusElement = document.getElementById("mpu-status");
                if (data.mpuDetected) {
                mpuStatusElement.textContent = "MPU6050 Status: Connected";
                mpuStatusElement.style.color = "#00ff00"; // Green color
                } else {
                mpuStatusElement.textContent = "MPU6050 Status: Not Detected";
                mpuStatusElement.style.color = "#ff0000"; // Red color
                }
            }

            // Function to fetch sensor data from ESP32 and update HTML elements
            function fetchData() {
                fetch("/sensorData")
                .then(response => response.json())
                .then((data) => {
                    updateWifiStatus(data);
                    updateMpuStatus(data);
                    updateInitialReadings(data);
                    updateSensorData(data);
                    updatePostureStatus(data);
                })
                .catch(error => console.error("Error fetching data:", error));
            }

            // Fetch sensor data periodically
            setInterval(fetchData, 1000); // Update every second

            // Function to toggle dark theme
            const themeToggle = document.getElementById("theme-toggle");
            themeToggle.addEventListener("change", function() {
                document.body.classList.toggle("dark-theme");
            });
    };

  </script>
</body>
</html>
)";

// Function to handle requests for sensor data
void handleSensorData() {
  String data = String("{\"accelerationX\":") + String(accelerationX) + 
                String(",\"accelerationY\":") + String(accelerationY) + 
                String(",\"accelerationZ\":") + String(accelerationZ) + 
                String(",\"rotationX\":") + String(rotationX) + 
                String(",\"rotationY\":") + String(rotationY) + 
                String(",\"rotationZ\":") + String(rotationZ) + 
                String(",\"temperature\":") + String(temperature) +
                String(",\"initialX\":") + String(initialX) + 
                String(",\"initialY\":") + String(initialY) + 
                String(",\"initialZ\":") + String(initialZ) + 
                String(",\"initialGX\":") + String(initialGX) + 
                String(",\"initialGY\":") + String(initialGY) + 
                String(",\"initialGZ\":") + String(initialGZ) + 
                String(",\"initialTemp\":") + String(initialTemp) +
                String(",\"mpuDetected\":") + String(mpuDetected) +
                String(",\"wifiSSID\":\"") + WiFi.SSID() + "\"" +
                String(",\"wifiSignalStrength\":") + WiFi.RSSI() +
                String(",\"isBadPosture\":") + String(isBadPosture) +
                String(",\"leanType\":") + "\"" + leanType + "\"" +
                String(",\"deviation\":") + String(deviation) +
                String("}");

  server.send(200, "application/json", data);
}

void measurePosition() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  if (!dmpReady) return;
  
  if (mpu68.dmpGetCurrentFIFOPacket(fifoBuffer)) {
  mpu68.dmpGetQuaternion(&q, fifoBuffer);
  mpu68.dmpGetGravity(&gravity, &q);

  // Measure Readable World Acceleration
  // mpu68.dmpGetAccel(&aa, fifoBuffer);
  // mpu68.dmpGetLinearAccel(&aaReal, &aa, &gravity);
  // mpu68.dmpGetLinearAccelInWorld(&aaWorld, &aaReal, &q);
  // accelerationX = aaReal.x;
  // accelerationY = aaReal.y;
  // accelerationZ = aaReal.z;
  accelerationX = a.acceleration.x;
  accelerationY = a.acceleration.y;
  accelerationZ = a.acceleration.z - 9.8;
  // accelerationX = aaWorld.x;
  // accelerationY = aaWorld.y;
  // accelerationZ = aaWorld.z;

  // Measure Readable Yaw Pitch Roll 
  mpu68.dmpGetYawPitchRoll(ypr, &q, &gravity);
  rotationX = ypr[2] * 180; // Roll 
  rotationY = ypr[1] * 180; // Pitch
  rotationZ = ypr[0] * 180; // Yaw

  // temperature = mpu.getTemperature();
  temperature = temp.temperature;
  }
}

void motorVibrate(int count) {
  for (int i = 0; i < count; i++) {
    digitalWrite(motorBase,HIGH);
    delay(250);
    digitalWrite(motorBase,LOW);
  }
}

void setup() {
  #if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
      Wire.begin();
      Wire.setClock(400000); // 400kHz I2C clock. Comment this line if having compilation difficulties
  #elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
      Fastwire::setup(400, true);
  #endif
  WiFi.mode(WIFI_STA);
  Serial.begin(115200);
  while (!Serial)
    delay(10);
  
  pinMode(Red, OUTPUT);
  pinMode(Green, OUTPUT);
  pinMode(Blue, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(motorBase, OUTPUT);

  Serial.setDebugOutput(true);
  delay(1000);
  Serial.println("\n Starting");

  pinMode(TRIGGER_PIN,INPUT);

  Serial.println("Initializing ESP32");
  delay(500);
  digitalWrite(Red, HIGH);
  delay(500);
  digitalWrite(Green, HIGH);
  delay(500);
  digitalWrite(Blue, HIGH);

  // Connecting to Wifi network
  if (wm_nonblocking) wm.setConfigPortalBlocking(false);

  int customFieldLength = 50;

  const char* custom_radio_str = "<br/><label for='customFieldid'>Custom Field Label</label><input type = 'radio' name = 'customfieldid' value = '1' checked> One<br><input type = 'radio' name = 'customfieldid' value = '2'> Two<br><input type = 'radio' name = 'customfieldid' value = '3'> Three";
  new (&custom_field) WiFiManagerParameter(custom_radio_str);

  wm.addParameter(&custom_field);
  wm.setSaveParamsCallback(saveParamCallback);

  std::vector<const char *> menu = {"wifi","info","param","sep","restart","exit"};
  wm.setMenu(menu);

  // set dark theme
  wm.setClass("invert");

  wm.setConfigPortalTimeout(600);

  bool res;
  res = wm.autoConnect("PosturePro ESP32_AP"); //No password
  if(!res) {
    Serial.println("Failed to connect or hit timeout");
  }
  else {
    Serial.println("Connected Successfully...");
    digitalWrite(LED_BUILTIN,HIGH); 
  }

  // Route for serving the HTML page
  server.on("/", HTTP_GET, [&server]() {
    server.send(200, "text/html", htmlPage);
  });

  // Route for handling requests for sensor data
  server.on("/sensorData", HTTP_GET, handleSensorData);

  server.begin();
  Serial.println("HTTP Server Started");

  // Initialize device
  mpu.begin();
  Serial.println(F("Initializing I2C devices..."));
  mpu68.initialize();
  // Verify Connection
  Serial.println(F("Checking MPU6050 connections..."));
  Serial.println(mpu68.testConnection()); 
   if (!mpu68.testConnection())
  {
    Serial.println(F("MPU6050 connection failed"));
    mpuDetected = false;
    digitalWrite(Red,LOW);
  } else {
    Serial.println("MPU6050 connection successful");
    mpuDetected = true;
    digitalWrite(Red,HIGH);
  }

  // load and configure
  Serial.println("Initializing DMP...");
  devStatus = mpu68.dmpInitialize();

  //Calibrate MPU
  mpu68.setXAccelOffset(-1508);
  mpu68.setYAccelOffset(-889);
  mpu68.setZAccelOffset(1288);

  mpu68.setXGyroOffset(203);
  mpu68.setYGyroOffset(93);
  mpu68.setZGyroOffset(5);

  if(devStatus == 0) {
    mpu68.CalibrateAccel(6);
    mpu68.CalibrateGyro(6);
    mpu68.PrintActiveOffsets();
    Serial.println(F("Enabling DMP..."));
    mpu68.setDMPEnabled(true);
    dmpReady = true;
    packetSize = mpu68.dmpGetFIFOPacketSize(); //get expected dmp packet size for later comparison
  } else {
    Serial.println(F("DMP Initialization failed(code "));
    Serial.println(devStatus); // 1 = Initial memory load failed, 2 = DMP configuration failed
    Serial.println(F(")"));
  }

    // motorVibrate(2);
    delay(500);
  // Current Position
    measurePosition();
    initialX = accelerationX;
    initialY = accelerationY;
    initialZ = accelerationZ;
    initialGX = rotationX;
    initialGY = rotationY;
    initialGZ = rotationZ;
    initialTemp = temperature;
    startTime = millis(); //Record Start Time
    Serial.println();
    Serial.print("InitalX = ");
    Serial.print(initialX);
    Serial.print(", InitalY = ");
    Serial.print(initialY);
    Serial.print(", InitalZ = ");
    Serial.println(initialZ);
    Serial.println(startTime);
    // motorVibrate(2);
}

float integrateAcceleration(float initialAcceleration, float finalAcceleration, unsigned long dt) {
  // v = u + at
    float initialVelocity = initialAcceleration * 0;
    float finalVelocity = finalAcceleration * (dt/1000);
    float averageVelocity = (finalVelocity - initialVelocity); // / 2; // avg v = (vf+vi)/2
    //avg a = (iac + fa)/2 , displacement = a * t;
    // float averageAcceleration = (initialAcceleration + finalAcceleration) / 2;
    float displacement = averageVelocity / (dt/1000);
    return displacement;
}

void checkButton() {
  if (digitalRead(TRIGGER_PIN) == HIGH) {
    delay(50);
    if (digitalRead(TRIGGER_PIN) == HIGH) {
      Serial.println("Trigger Button Pressed");
      digitalWrite(Red,LOW);
      delay(3000);
      if (digitalRead(TRIGGER_PIN) == HIGH) {
        digitalWrite(Blue,LOW);
        Serial.println("Trigger Button Held");
        Serial.println("Erasing Config, restarting");
        wm.resetSettings();
        ESP.restart();
      }

      Serial.println("Starting Config Portal");
      digitalWrite(Green,LOW);
      digitalWrite(Red,HIGH);
      digitalWrite(Blue,HIGH);

      wm.setConfigPortalTimeout(600);

      if (!wm.startConfigPortal("PosturePro ESP32 OnDemandAP")) {
        Serial.println("Failed to connect or hit timeout");
        delay(3000);
      } else {
        Serial.println("Connected Successfully...");
        digitalWrite(LED_BUILTIN,HIGH);
        digitalWrite(Green,HIGH);
      }
    }
  }
}

String getParan(String name) {
  String value;
  if(wm.server->hasArg(name)) {
    value = wm.server->arg(name);
  }
  return value;
}

void saveParamCallback() {
  Serial.println("[CALLBACK] saveParamCallback fired");
  Serial.println("PARAM customfieldid = " + getParan("customfieldid"));
}

void loop() {
  if(wm_nonblocking) wm.process();
  checkButton();

  server.handleClient();

  if (!mpu68.testConnection())
  {
    Serial.println(F("MPU6050 connection failed"));
    mpuDetected = false;
    digitalWrite(Red,LOW);
  } else {
    mpuDetected = true;
    digitalWrite(Red,HIGH);
  }
  
  // Get MPU6050 Data
  measurePosition();
  Serial.println(millis());
  Serial.print("ypr\t");
  Serial.print(ypr[0] * 180/M_PI);
  Serial.print("\t");
  Serial.print(ypr[1] * 180/M_PI);
  Serial.print("\t");
  Serial.println(ypr[2] * 180/M_PI);
  // Serial.print("aworld\t");
  // Serial.print(aaReal.x);
  // Serial.print("\t");
  // Serial.print(aaReal.y);
  // Serial.print("\t");
  // Serial.println(aaReal.z);
  Serial.print("X = ");
  Serial.print(accelerationX);
  Serial.print(", Y = ");
  Serial.print(accelerationY);
  Serial.print(", Z = ");
  Serial.println(accelerationZ);
  Serial.println(startTime);

  // Check if 10sec have elapsed
  // if (mpuDetected == true) {
  //   if (millis() - startTime > 5000){
  //   float displacementX = integrateAcceleration(initialX,accelerationX,interval);
  //   float displacementY = integrateAcceleration(initialY,accelerationY,interval);
  //   float displacementZ = integrateAcceleration(initialZ,accelerationZ,interval);
  //   float distanceMoved = sqrt(pow(displacementX, 2) + pow(displacementY, 2) + pow(displacementZ, 2));
  //     if (distanceMoved > movementThreshold) {
  //       Serial.print("DistanceMoved: ");
  //       Serial.println(distanceMoved);
  //       digitalWrite(motorBase, HIGH);
  //       delay(2000);
  //       digitalWrite(motorBase, LOW);     
  //       startTime = millis(); 
  //     } 
  //   }
  // }

  if ((ypr[2] * 180 > 45 || ypr[2] * 180 < -45) && mpuDetected) {
    Serial.println("Forward Lean Detected");
    isBadPosture = true;
    if (ypr[2] * 180 > 45) {
      deviation = (ypr[2] * 180) - 45;
      leanType = "Forward Lean";
    } else if (ypr[2] * 180 < -45) {
      deviation = (ypr[2] * 180) + 45;
      leanType = "Backward Lean";
    }
    digitalWrite(motorBase, HIGH);
  } 
  else if ((ypr[0] * 180 > 35 || ypr[0] * 180 < -35) && mpuDetected) {
    Serial.println("Side Lean Detected");
    isBadPosture = true;
    if (ypr[0] * 180 > 35) {
      deviation = (ypr[2] * 180) - 35;
      leanType = "Side Lean - Right";
    } else if(ypr[0] * 180 < -35) {
      deviation = (ypr[0] * 180) + 35;
      leanType = "Side Lean - Left";
    }
    digitalWrite(motorBase, HIGH);
  } else {
    digitalWrite(motorBase, LOW);
    isBadPosture = false;
    leanType = "No lean detected";
    deviation = 0.0;
  }
   
  delay(500);
}
