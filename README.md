## Project Overview
This project, developed a decentralized authentication system using Edge AI and Bluetooth Low Energy (BLE). It features a "Magic Wand" (Arduino Nano 33 BLE) that recognizes spatial gestures (digits '1' and '0') using an onboard IMU and a neural network model. Upon successful authentication of a 4-digit sequence, the wand triggers a "Smart Locker" (ESP32-S3) to actuate a physical lock via an SG90 servo.

## System Architecture
The system follows a local Machine-to-Machine (M2M) communication pattern:
- **Client (Magic Wand)**: Performs real-time IMU sampling (100Hz), TinyML inference, and state management.
- **Server (Smart Locker)**: Acts as a BLE peripheral, validating received credentials and driving high-current actuators (Servo).

## Hardware Components
- **Microcontrollers**: Arduino Nano 33 BLE, ESP32-S3 (DevKitM).
- **Sensors/Actuators**: LSM9DS1 IMU (onboard), SG90 Servo Motor, WS2812B NeoPixel LEDs.
- **Interfaces**: Limit switches (D12 for Power, A6 for Recording).

## Repository Structure
- `/src/wand_nano33ble`: Arduino source code for the gesture transmitter.
- `/src/locker_esp32s3`: PlatformIO project for the receiver and actuator.
- `/data`: Raw CSV datasets and automated collection scripts (`record.py`).
- `/hardware`: Wiring diagrams and BOM.

## Key Technical Solutions
- **I2C Bus Stability**: Implemented hardware power cycling (`IMU.end()`) during inference to prevent I2C FIFO overflow and Mbed OS kernel panics.
- **Memory Optimization**: Replaced `String` classes with static `char[]` arrays to eliminate heap fragmentation in memory-constrained environments.
- **Flash Partitioning**: Configured `board = esp32-s3-devkitm-1` in PlatformIO to resolve binary image header size mismatches.

## Installation & Deployment
1. **Model**: Import the Edge Impulse `.zip` library from `/src` into your Arduino IDE.
2. **Locker**: Deploy the PlatformIO project in `/src/locker_esp32s3` to the ESP32-S3.
3. **Wand**: Upload the `.ino` sketch to the Nano 33 BLE, ensuring the Edge Impulse header name matches your build.


# Define the content for the Project Report
report_content = """# CASA0018 Project Report: Edge AI Gesture-Controlled Authentication

## 1. Problem Context and Research Question
In the context of touchless interaction and physical security, traditional biometric or mechanical locks often lack flexibility or require centralized network infrastructure. This project investigates the feasibility of deploying a localized, low-latency gesture recognition system using TinyML. The primary research question is: *How can high-frequency spatial temporal data be classified on ultra-low-power microcontrollers to provide robust, decentralized access control?*

## 2. Data Collection and Processing
The dataset was built using a custom Python automation script (`record.py`) interfacing with the LSM9DS1 IMU. 
- **Sampling Strategy**: 3-axis accelerometer data was sampled at 100Hz.
- **Classes**: Three labels were defined: '1', '0', and 'idle'. The 'idle' class is critical for real-world deployment to minimize false positives caused by accidental movement.
- **Preprocessing**: Data was processed using Spectral Analysis in Edge Impulse to extract frequency-domain features, effectively reducing the noise inherent in manual gesture execution.

## 3. Documentation of Methods and Results
### 3.1 Model Architecture
A Convolutional Neural Network (CNN) architecture was selected for its strength in recognizing patterns in time-series data. The model was quantized to int8 to fit the 256KB RAM constraints of the Nano 33 BLE while maintaining >95% accuracy in controlled testing.

### 3.2 Technical Implementation Challenges
The development process involved several critical iterations:
- **Watchdog & RTOS Conflicts**: Early versions suffered from system crashes during the 1-second inference window. By implementing a 1ms `delay()` in the sampling loop and temporarily suspending the IMU bus (`IMU.end()`), the Mbed OS watchdog was satisfied, ensuring system stability.
- **Flash Memory Misalignment**: The ESP32-S3 initially reported binary header size errors (8192k vs 4096k). This was solved by reconfiguring the PlatformIO environment to target the `devkitm-1` variant, correctly aligning the partition table with the physical hardware.

## 4. Results and Testing
Functional testing confirmed a seamless "Sleep-Wake-Identify-Transmit" loop. The wand successfully transmits the "1010" credential over BLE to the locker, which rotates the servo to 90 degrees with a green LED visual confirmation. False rejection rates were lowered by implementing a 0.6 confidence threshold for each digit recognition.

## 5. Critical Reflection
### 5.1 Limitations
- **User Specificity**: The model currently exhibits overfitting to the primary developer's gesture speed. Expanding the dataset to include diverse biometric profiles would improve generalizability.
- **BLE Security**: The current protocol uses plaintext transmission. Future iterations should implement a challenge-response handshake with cryptographic nonces to prevent replay attacks.

### 5.2 Future Improvements
Transitioning from a basic CNN to a Gated Recurrent Unit (GRU) might better handle the variable-length nature of human gestures. Additionally, utilizing the ESP32-S3's hardware acceleration for BLE encryption would significantly enhance the system's security posture.

