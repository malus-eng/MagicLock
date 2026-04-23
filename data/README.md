# Dataset and Data Collection Scripts

This folder contains the raw dataset used to train the gesture recognition model in Edge Impulse, along with the custom Python script (`record.py`) developed for efficient data acquisition.

## Dataset Overview
The dataset consists of time-series accelerometer data (X, Y, Z axes) captured at approximately 100Hz using the onboard LSM9DS1 IMU of the Arduino Nano 33 BLE.
The data is categorized into three classes to ensure robust recognition and minimize false positives:
* `one`: Data capturing the motion of drawing the number '1' in the air.
* `zero`: Data capturing the motion of drawing the number '0' in the air.
* `idle`: Background noise and random movements to serve as a negative class, preventing the model from misclassifying casual movements.

All raw data files are saved in standard CSV format, making them directly compatible with the Edge Impulse ingestion pipeline.

## Data Collection Tool (`record.py`)
To streamline the data collection process and ensure consistency across samples, a custom Python script using the `pyserial` library was developed. 

**How it works:**
1.  **Serial Communication**: The script establishes a serial connection with the Arduino Nano 33 BLE (baud rate: 115200).
2.  **Trigger Mechanism**: Upon pressing 'Enter' on the PC, the script sends a trigger command (`'r'`) to the wand.
3.  **Automated Logging**: The wand begins recording IMU data. The Python script listens to the serial port, extracts the payload (ignoring headers/footers), and automatically saves the continuous data stream into sequentially numbered CSV files (e.g., `one_01.csv`, `one_02.csv`).
4.  **Validation**: It includes a safety check to discard incomplete recordings (e.g., files with fewer than 50 lines of data).

**Usage:**
```bash
# Ensure the Arduino IDE Serial Monitor is closed before running
python record.py
