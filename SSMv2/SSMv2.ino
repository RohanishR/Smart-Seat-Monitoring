#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MPU6050.h"
#include <OneWire.h>
#include <DallasTemperature.h>

// -------- OLED & SEAT SENSOR DEFINITIONS --------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define FSR_SEAT A0
#define FSR_BACK A1
#define BUZZER 8
#define TEMP_PIN 2

MPU6050 mpu;
OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);

// -------- MAX30105 (HEART RATE) GLOBALS --------
MAX30105 particleSensor;

long lastBeat = 0;
float beatAvg = 0; // Changed to float for smoother background math
unsigned long fingerOffTime = 0; 

// -------- MAX30105 (SpO2) GLOBALS --------
#define BUFFER_SIZE 50 
float redBuffer[BUFFER_SIZE];
float irBuffer[BUFFER_SIZE];
int bufferIndex = 0;
bool bufferFilled = false; 

float spo2 = 0;
float calibrationFactor = 1.0;

// -------- TIMING GLOBALS --------
unsigned long lastSeatUpdate = 0;
unsigned long lastBuzzerToggle = 0;
bool buzzerState = false;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(400000); 

  pinMode(BUZZER, OUTPUT);
  mpu.initialize();
  sensors.begin();
  
  sensors.setWaitForConversion(false);
  sensors.requestTemperatures(); 

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED failed"));
    for(;;);
  }
  
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(20, 20);
  display.println(F("SMART"));
  display.println(F("SEAT"));
  display.display();

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println(F("MAX30105 failed."));
    while (1);
  }
  
  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x2F); 
  particleSensor.setPulseAmplitudeIR(0x2F);

  delay(2000); 
  display.clearDisplay();
}

void loop() {
  // ==========================================
  // 1. FAST LOOP: MAX30105 SENSOR
  // ==========================================
  long irValue = particleSensor.getIR();
  long redValue = particleSensor.getRed();

  if (irValue < 50000) {
    // FINGER REMOVED LOGIC (With 2-Second Grace Period)
    if (fingerOffTime == 0) fingerOffTime = millis();
    
    // Wipes the screen ONLY if finger is completely gone for 2 seconds
    if (millis() - fingerOffTime > 2000) { 
      beatAvg = 0;
      spo2 = 0;
      bufferFilled = false;
      bufferIndex = 0;
    }
  } else {
    // FINGER IS ON SENSOR
    fingerOffTime = 0; // Reset the removal timer

    // Calculate Heart Rate
    if (checkForBeat(irValue)) {
      long delta = millis() - lastBeat;
      lastBeat = millis();

      if (delta > 300 && delta < 2000) { // Allow 30 to 200 BPM
        float instantaneousBPM = 60.0 / (delta / 1000.0);
        
        if (instantaneousBPM > 30 && instantaneousBPM < 200) {
          if (beatAvg == 0) {
            beatAvg = instantaneousBPM; // Instant show on first beat
          } else {
            // FIX: Smoother 80/20 math prevents the numbers from wildly jumping
            beatAvg = (beatAvg * 0.8) + (instantaneousBPM * 0.2);
          }
        }
      }
    }

    // FIX: EXTENDED TIMEOUT. 
    // Sensor will "hold" the last good value for 7 seconds to survive finger twitches
    if (millis() - lastBeat > 7000) {
      beatAvg = 0; 
    }

    // Rolling SpO2 Buffer
    redBuffer[bufferIndex] = redValue;
    irBuffer[bufferIndex] = irValue;
    bufferIndex++;

    if (bufferIndex >= BUFFER_SIZE) {
      bufferIndex = 0;
      bufferFilled = true; 
    }
  }

  // ==========================================
  // 2. SLOW LOOP: SMART SEAT, SpO2 CALC & DISPLAY
  // ==========================================
  if (millis() - lastSeatUpdate >= 500) {
    lastSeatUpdate = millis();

    if (bufferFilled) {
      float redDC = 0, irDC = 0, redAC = 0, irAC = 0;

      for (int i = 0; i < BUFFER_SIZE; i++) {
        redDC += redBuffer[i];
        irDC += irBuffer[i];
      }
      redDC /= BUFFER_SIZE; irDC /= BUFFER_SIZE;

      for (int i = 0; i < BUFFER_SIZE; i++) {
        redAC += pow(redBuffer[i] - redDC, 2);
        irAC += pow(irBuffer[i] - irDC, 2);
      }
      redAC = sqrt(redAC / BUFFER_SIZE);
      irAC = sqrt(irAC / BUFFER_SIZE);

      float R = (redAC / redDC) / (irAC / irDC);
      spo2 = (110 - 25 * R) * calibrationFactor;

      if (spo2 > 100) spo2 = 100;
      if (spo2 < 0) spo2 = 0;
    }

    float temperature = sensors.getTempCByIndex(0);
    sensors.requestTemperatures(); 

    int fsrSeat = analogRead(FSR_SEAT);
    int fsrBack = analogRead(FSR_BACK);

    int16_t ax, ay, az;
    mpu.getAcceleration(&ax, &ay, &az);
    float pitch = atan2(ax, az) * 57.3;

    String seatStatus;
    String postureStatus;

    if (fsrSeat > 100) {
      seatStatus = "Present";
      if (fsrBack > 50) postureStatus = "Leaning Back";
      else if (abs(pitch) > 20) postureStatus = "Bad";
      else postureStatus = "Good";
    } else {
      seatStatus = "Empty";
      postureStatus = "--";
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println(F("SMART SEAT"));
    display.drawLine(0, 10, 128, 10, WHITE);

    display.setCursor(0, 14); display.print(F("Seat: ")); display.println(seatStatus);
    display.setCursor(0, 24); display.print(F("Post: ")); display.println(postureStatus);
    display.setCursor(0, 34); display.print(F("Temp: ")); display.print(temperature); display.println(F(" C"));
    
    display.setCursor(0, 44);
    if (beatAvg > 0) { 
      display.print(F("HR  : ")); 
      display.print((int)beatAvg); // Cast back to integer for clean display
      display.println(F(" BPM")); 
    } 
    else if (particleSensor.getIR() > 50000) { display.println(F("HR  : Reading...")); } 
    else { display.println(F("HR  : 0 BPM")); }
    
    display.setCursor(0, 54);
    if (bufferFilled) { display.print(F("SpO2: ")); display.print(spo2); display.println(F(" %")); } 
    else if (particleSensor.getIR() > 50000) { display.println(F("SpO2: Reading...")); } 
    else { display.println(F("SpO2: 0.00 %")); }
    
    display.display();

    Serial.print(F("{"));
    Serial.print(F("\"seat\":\"")); Serial.print(seatStatus); Serial.print(F("\","));
    Serial.print(F("\"posture\":\"")); Serial.print(postureStatus); Serial.print(F("\","));
    Serial.print(F("\"pitch\":")); Serial.print(pitch); Serial.print(F(","));
    Serial.print(F("\"temp\":")); Serial.print(temperature); Serial.print(F(","));
    Serial.print(F("\"hr\":")); Serial.print((int)beatAvg); Serial.print(F(","));
    Serial.print(F("\"spo2\":")); Serial.print(spo2);
    Serial.println(F("}"));
  }

  // ==========================================
  // 3. NON-BLOCKING BUZZER LOGIC
  // ==========================================
  int currentFsrSeat = analogRead(FSR_SEAT);
  int currentFsrBack = analogRead(FSR_BACK);
  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);
  float currentPitch = atan2(ax, az) * 57.3;

  if (currentFsrSeat > 100 && (currentFsrBack > 50 || abs(currentPitch) > 20)) {
    if (millis() - lastBuzzerToggle > 150) { 
      buzzerState = !buzzerState;
      digitalWrite(BUZZER, buzzerState ? HIGH : LOW);
      lastBuzzerToggle = millis();
    }
  } else {
    digitalWrite(BUZZER, LOW);
  }
}