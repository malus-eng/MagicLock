### 🔌 2. Hardware List & Wiring Summary


#### Device 1: Magic Wand (Client / Transmitter)
**Core Board**: Arduino Nano 33 BLE (Built-in LSM9DS1 9-axis IMU)

* **Wake/Power Button**:
    * Pin 1: Connects to the **`D12`** pin on the Nano 33 BLE.
    * Pin 2: Connects to **`GND`**.
* **Record Button**:
    * Pin 1: Connects to the **`A6`** pin on the Nano 33 BLE.
    * Pin 2: Connects to **`GND`**.
* **Status Indicator (WS2812B NeoPixel Module - 2 LEDs)**:
    * `DIN` (Data In): Connects to the **`D11`** pin on the Nano 33 BLE.
    * `VCC` (Power): Connects to the **`3.3V`** pin on the Nano 33 BLE.
    * `GND` (Ground): Connects to **`GND`**.

#### Device 2: Lock Box (Server / Receiver)
**Core Board**: ESP32-S3 (DevKitC / DevKitM)

* **Physical Lock Actuator (SG90 Servo)**:
    * `Signal` (Orange/Yellow wire): Connects to the **`GPIO 4`** pin on the ESP32-S3.
    * `VCC` (Red power wire): Connects to the **`5V`** pin (or VBUS). 
    * `GND` (Brown/Black wire): Connects to **`GND`**.
* **Status Indicator (WS2812B NeoPixel Module - 1 LED)**:
    * `DIN` (Data In): Connects to the **`GPIO 5`** pin on the ESP32-S3.
    * `VCC` (Power): Connects to **`3.3V`** or **`5V`** (WS2812B is usually compatible with both; connecting to 3.3V is recommended to prevent logic level mismatch).
    * `GND` (Ground): Connects to **`GND`**.
