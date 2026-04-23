#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ESP32Servo.h>
#include <Adafruit_NeoPixel.h>

// ================= Hardware Pins & Parameters =================
#define SERVO_PIN 4       
#define LED_PIN   5       
#define NUM_LEDS  1       
#define CORRECT_PASS "1010" // The Ultimate Password

Servo myServo;
Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// ================= BLE UUIDs =================
#define SERVICE_UUID        "19b10000-e8f2-537e-4f6c-d104768a1214"
#define CHARACTERISTIC_UUID "19b10001-e8f2-537e-4f6c-d104768a1214"

// ================= Global States =================
int currentAngle = 0;           // 0=Locked, 90=Unlocked
unsigned long errorTimer = 0;   
bool isErrorMode = false;       

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      Serial.println(" Magic Wand Connected!");
    }
    void onDisconnect(BLEServer* pServer) {
      Serial.println(" Connection lost, restarting advertising...");
      BLEDevice::startAdvertising(); 
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();
      
      if (rxValue.length() > 0) {
        String receivedPass = "";
        for (int i = 0; i < rxValue.length(); i++) {
          receivedPass += rxValue[i];
        }
        
        Serial.print("📡 Received code: "); Serial.println(receivedPass);

        if (receivedPass == CORRECT_PASS) {
          isErrorMode = false;
          // State toggle logic: if closed, open it; if open, close it.
          if (currentAngle == 0) {
            Serial.println("[Password Correct] Green light on, unlocking (90°)");
            currentAngle = 90;
          } else {
            Serial.println("[Password Correct] Green light on, locking (0°)");
            currentAngle = 0;
          }
          myServo.write(currentAngle);
          pixels.fill(pixels.Color(0, 255, 0)); // Green light
          pixels.show();
          
          delay(2000);
          pixels.fill(pixels.Color(0, 0, 255)); // Return to standby blue light
          pixels.show();
        } 
        else {
          Serial.println("[Password Incorrect] Red light warning");
          isErrorMode = true;
          errorTimer = millis(); 
          pixels.fill(pixels.Color(255, 0, 0)); 
          pixels.show();
        }
      }
    }
};

void setup() {
  Serial.begin(115200);

  pixels.begin();
  pixels.setBrightness(100);
  pixels.fill(pixels.Color(0, 0, 255)); 
  pixels.show();

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  myServo.setPeriodHertz(50);
  myServo.attach(SERVO_PIN, 500, 2400);
  myServo.write(0); 

  BLEDevice::init("SmartBenchLock"); 
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->setValue("Init");
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  BLEDevice::startAdvertising();

  Serial.println("=== Smart Locker Ready (Standby Blue Light) ===");
}

void loop() {
  if (isErrorMode && (millis() - errorTimer > 2500)) {
    isErrorMode = false;
    Serial.println("Warning ended, returning to standby blue light");
    pixels.fill(pixels.Color(0, 0, 255)); 
    pixels.show();
  }
  delay(20); 
}
