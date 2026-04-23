#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ESP32Servo.h>
#include <Adafruit_NeoPixel.h>

// ================= 引脚与参数 =================
#define SERVO_PIN 4       
#define LED_PIN   5       
#define NUM_LEDS  1       
#define CORRECT_PASS "1010" // 🎯 终极密码

Servo myServo;
Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// ================= 蓝牙 UUID =================
#define SERVICE_UUID        "19b10000-e8f2-537e-4f6c-d104768a1214"
#define CHARACTERISTIC_UUID "19b10001-e8f2-537e-4f6c-d104768a1214"

// ================= 全局状态 =================
int currentAngle = 0;           // 0=锁死, 90=开启
unsigned long errorTimer = 0;   
bool isErrorMode = false;       

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      Serial.println("🔗 魔法手杖已连接！");
    }
    void onDisconnect(BLEServer* pServer) {
      Serial.println("💔 连接断开，重新开启广播...");
      BLEDevice::startAdvertising(); 
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();
      
      if (rxValue.length() > 0) {
        // 将收到的数据转换为 C++ String 方便比对
        String receivedPass = "";
        for (int i = 0; i < rxValue.length(); i++) {
          receivedPass += rxValue[i];
        }
        
        Serial.print("📡 收到暗号: "); Serial.println(receivedPass);

        if (receivedPass == CORRECT_PASS) {
          isErrorMode = false;
          // 状态翻转逻辑：如果关着就开，如果开着就关
          if (currentAngle == 0) {
            Serial.println("🔓 [密码正确] 亮绿灯，开锁 (90°)");
            currentAngle = 90;
          } else {
            Serial.println("🔒 [密码正确] 亮绿灯，关锁 (0°)");
            currentAngle = 0;
          }
          myServo.write(currentAngle);
          pixels.fill(pixels.Color(0, 255, 0)); // 亮绿灯
          pixels.show();
          
          // 绿灯保持 2 秒后恢复待机蓝灯
          delay(2000);
          pixels.fill(pixels.Color(0, 0, 255)); 
          pixels.show();
        } 
        else {
          Serial.println("❌ [密码错误] 亮红灯警告");
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

  // 初始化硬件：默认蓝灯待机，锁死 0 度
  pixels.begin();
  pixels.setBrightness(100);
  pixels.fill(pixels.Color(0, 0, 255)); // 待机蓝灯
  pixels.show();

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  myServo.setPeriodHertz(50);
  myServo.attach(SERVO_PIN, 500, 2400);
  myServo.write(0); 

  // 初始化蓝牙 Server
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

  Serial.println("=== 🛡️ 智能锁盒已就绪 (待机蓝灯) ===");
}

void loop() {
  // 密码错误红灯 2.5 秒后自动恢复待机蓝灯
  if (isErrorMode && (millis() - errorTimer > 2500)) {
    isErrorMode = false;
    Serial.println("🔄 警告结束，恢复待机蓝灯");
    pixels.fill(pixels.Color(0, 0, 255)); 
    pixels.show();
  }
  delay(20); 
}