#include <Arduino.h>
#include <tiandiguailikaipizhixing-project-1_inferencing.h> 
#include <ArduinoBLE.h>
#include <Arduino_LSM9DS1.h>
#include <Adafruit_NeoPixel.h>

// ================= Pins & Hardware Config =================
#define LED_PIN           11
#define WAKE_BUTTON_PIN   12
#define RECORD_BUTTON_PIN A6
#define NUM_LEDS          2
void sendPasswordToLocker(const char* pwd);
// ================= Smart Power Management =================
// 5 minutes = 5 * 60 * 1000 = 300,000 milliseconds
unsigned long IDLE_TIMEOUT = 300000; 
unsigned long lastActivityTime = 0;

Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

const char* lockerServiceUUID = "19b10000-e8f2-537e-4f6c-d104768a1214";
const char* lockerCharUUID    = "19b10001-e8f2-537e-4f6c-d104768a1214";

char currentPassword[5] = {0}; 
int pwdIndex = 0;              

enum WandState { SLEEP_MODE, WAKE_MODE };
WandState currentState = SLEEP_MODE;

float features[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];

int raw_feature_get_data(size_t offset, size_t length, float *out_ptr) {
    memcpy(out_ptr, features + offset, length * sizeof(float));
    return 0;
}

// Helper function: Handle sleep mode transition uniformly
void goToSleep() {
  Serial.println("Wand entering sleep mode...");
  pixels.clear(); 
  pixels.show();
  currentState = SLEEP_MODE;
  memset(currentPassword, 0, sizeof(currentPassword));
  pwdIndex = 0;
}

void setup() {
  Serial.begin(115200);
  pinMode(WAKE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RECORD_BUTTON_PIN, INPUT_PULLUP);
  
  pixels.begin();
  pixels.setBrightness(30); 
  pixels.clear(); pixels.show();

  if (!IMU.begin()) { Serial.println("IMU Error"); while (1); }
  if (!BLE.begin()) { Serial.println("BLE Error"); while (1); }
  
  Serial.println("🪄 Wand initialization complete.");
}

void loop() {
  // ---------------- [State 1: Sleep Mode] ----------------
  if (currentState == SLEEP_MODE) {
    if (digitalRead(WAKE_BUTTON_PIN) == LOW) {
      delay(50);
      if (digitalRead(WAKE_BUTTON_PIN) == LOW) {
        Serial.println("Woken up! Entering active state...");
        pixels.fill(pixels.Color(255, 255, 255)); pixels.show();
        currentState = WAKE_MODE;
        lastActivityTime = millis(); // Record activation starting point
        while(digitalRead(WAKE_BUTTON_PIN) == LOW); 
      }
    }
  }

  // ---------------- [State 2: Active Mode] ----------------
  else if (currentState == WAKE_MODE) {
    
    // Check for auto-shutdown timeout
    if (millis() - lastActivityTime > IDLE_TIMEOUT) {
      Serial.println("5-minute timeout, auto-power off.");
      goToSleep();
      return; 
    }

    // Check for manual shutdown via D12
    if (digitalRead(WAKE_BUTTON_PIN) == LOW) {
      delay(50);
      if (digitalRead(WAKE_BUTTON_PIN) == LOW) {
        Serial.println("User manually pressed the power button.");
        goToSleep();
        while(digitalRead(WAKE_BUTTON_PIN) == LOW); 
        return;
      }
    }

    // --- Recognition Logic (Press A6) ---
    if (digitalRead(RECORD_BUTTON_PIN) == LOW) {
      lastActivityTime = millis(); // Refresh activity timestamp
      Serial.println("Start recording...");
      pixels.fill(pixels.Color(0, 0, 255)); pixels.show();
      
      int feature_ix = 0;
      unsigned long last_sample_time = micros();
      unsigned long sample_interval = 1000000 / EI_CLASSIFIER_FREQUENCY;

      while (digitalRead(RECORD_BUTTON_PIN) == LOW && (feature_ix + 2) < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
        if (micros() - last_sample_time >= sample_interval) {
          last_sample_time = micros(); 
          float x, y, z;
          if (IMU.accelerationAvailable()) {
            IMU.readAcceleration(x, y, z);
            features[feature_ix++] = x;
            features[feature_ix++] = y;
            features[feature_ix++] = z;
          }
        }
        delay(1); 
      }

      while (feature_ix < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
         if (feature_ix >= 3) { features[feature_ix] = features[feature_ix - 3]; }
         else { features[feature_ix] = 0; }
         feature_ix++;
      }

      pixels.fill(pixels.Color(255, 255, 255)); pixels.show();
      IMU.end(); 
      delay(100);

      signal_t features_signal;
      features_signal.total_length = sizeof(features) / sizeof(features[0]);
      features_signal.get_data = &raw_feature_get_data;
      ei_impulse_result_t result = { 0 };
      run_classifier(&features_signal, &result, false);

      IMU.begin();
      delay(50);

      String recognizedNumber = "";
      float max_value = 0;
      for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
          if (result.classification[i].value > max_value) {
              max_value = result.classification[i].value;
              recognizedNumber = result.classification[i].label;
          }
      }
      
      if (max_value > 0.6) {
         if (recognizedNumber == "one" || recognizedNumber == "1") {
             if (pwdIndex < 4) { currentPassword[pwdIndex++] = '1'; }
         } 
         else if (recognizedNumber == "zero" || recognizedNumber == "0") {
             if (pwdIndex < 4) { currentPassword[pwdIndex++] = '0'; }
         }
         Serial.print("Current password: "); Serial.println(currentPassword);
         lastActivityTime = millis(); // ⚡ Successful recognition counts as activity
      }

      if (pwdIndex >= 4) {
        Serial.println("Password complete, firing!");
        sendPasswordToLocker(currentPassword);
        
        // Reset current password buffer but keep the wand active
        memset(currentPassword, 0, sizeof(currentPassword));
        pwdIndex = 0;
        Serial.println("Reset complete, waiting for next gesture...");
        lastActivityTime = millis(); // ⚡ Firing complete, reset timer
      }
      delay(500); 
    } 
    else {
      float dummyX, dummyY, dummyZ;
      while (IMU.accelerationAvailable()) { IMU.readAcceleration(dummyX, dummyY, dummyZ); }
      delay(10); 
    }
  }
}

void sendPasswordToLocker(const char* pwd) {
  BLEDevice peripheral;
  BLE.scan(); 
  unsigned long startMillis = millis();
  
  Serial.println("Radar fully active, searching for [SmartBenchLock] ...");
  
  while (millis() - startMillis < 5000) {
    peripheral = BLE.available();
    if (peripheral) {
      if (peripheral.localName() == "SmartBenchLock" || peripheral.hasService(lockerServiceUUID)) {
        Serial.println("Target locked! Stopping scan.");
        BLE.stopScan();
        break;
      }
    }
  }

  if (peripheral && (peripheral.localName() == "SmartBenchLock" || peripheral.hasService(lockerServiceUUID))) {
    Serial.println("Attempting physical connection...");
    if (peripheral.connect()) {
      Serial.println("Connection successful! Initiating hack...");
      if (peripheral.discoverAttributes()) {
        BLECharacteristic lockerChar = peripheral.characteristic(lockerCharUUID);
        if (lockerChar) {
          Serial.print("Injecting ultimate password: "); Serial.println(pwd);
          lockerChar.writeValue((const uint8_t*)pwd, strlen(pwd));
          Serial.println("Injection complete!");
        }
      }
      peripheral.disconnect();
    } else {
      Serial.println("Failed to establish connection!");
    }
  } else {
    Serial.println("Scan timeout, locker not found nearby.");
  }
}