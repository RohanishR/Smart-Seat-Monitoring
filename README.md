# 🪑 Smart Seat Monitoring System

An IoT-enabled intelligent seating system that integrates posture, pressure, physiological, and vision-based sensors. It continuously monitors student posture, health, engagement, and stress in real time. Unlike traditional classrooms or single-technology solutions, it uses multi-sensor fusion to provide accurate, non-intrusive monitoring. The system enhances concentration, improves posture habits, detects fatigue or health issues early, and alerts teachers to abnormal patterns—creating a smarter, healthier learning environment.  

---

## 🚀 Features

- 📡 Real-time seat occupancy monitoring  
- 🔌 ESP32 + sensor integration  
- 🌐 Live dashboard (Flask + HTML/CSS/JS)  
- 🧾 Attendance tracking system 
- 📊 Data visualization (charts & logs)  

---

## 📁 Project Structure
```
SSMS/
│
├── CameraWebServer/ # ESP32-CAM web server files
│ ├── .skip.esp32c3
│ ├── app_httpd.cpp
│ ├── camera_index.h
│ ├── camera_pins.h
│ └── CameraWebServer.ino
│
├── SSMv2/ # Main Ardiuno seat monitoring code
│ └── SSMv2.ino
│
├── templates/ # Frontend dashboard 
│ └── dashboard.html
│
├── server.py # Flask backend server
├── requirements.txt # Python dependencies
```

---

## 🧩 Description

- **CameraWebServer/** → Handles ESP32-CAM streaming functionality  
- **SSMv2/** → Core seat monitoring logic (sensor handling)  
- **templates/** → Web dashboard UI  
- **server.py** → Backend server (Flask + Serial communication)  
- **requirements.txt** → Required Python libraries  

---

# ✅ Smart Seat Monitoring – Setup & Usage Guide

## 🔌 Step 1: Connect Hardware
- Plug the **Arduino cable** into one laptop port.  
- Plug the **ESP32-CAM cable** into another port.  
- Confirm that the hardware lights turn on.

---

## 📶 Step 2: Connect to Wi‑Fi
- On your mobile, open **Hotspot settings**.  
- Change the Wi‑Fi name to **`localhost`** and password to **`12345678`**.  
- Connect both your **PC/Laptop** and **ESP32-CAM** to the same Wi‑Fi network.  

---

## ⚙️ Step 3: Installation & Setup

### 1️⃣ Clone the Repository
```bash
git clone https://github.com/RohanishR/Smart-Seat-Monitoring.git
cd Smart-Seat-Monitoring
```

### 2️⃣ Install Dependencies
```bash
pip install -r requirements.txt
```

### 3️⃣ Configure Serial Port
- Open **server.py**.  
- Update the line:
  ```python
  PORT = "COM3"
  ```
- Change `"COM3"` to match your system’s serial port.

### 4️⃣ Configure ESP32-CAM IP Address
- Open **server.py** and update:
  ```python
  ESP32_IP = "http://10.109.160.207"
  ```
- Open **./templates/dashboard.html** and update:
  ```javascript
  const ESP32_IP = "http://10.109.160.207";
  ```
- ⚠️ **Important:** Replace this IP with your ESP32-CAM’s current IP (visible in your hotspot’s connected devices list).

---

## ▶️ Step 4: Run the Application
```bash
python server.py
```
- Open your browser and go to:  
  **http://127.0.0.1:5000**

---

## 📷 Step 5: Configure ESP32-CAM
- Open your ESP32-CAM IP in browser (e.g., **http://10.109.160.207**).  
- Click **Start Stream** → enable **Face Recognition & Detection**.  
- Click **Enroll Faces** (look directly at the camera).  
- Wait until face recognition completes.

---

## 📊 Step 6: Access Dashboard
- Open **Chrome browser**.  
- Enter **http://127.0.0.1:5000**.  
- The dashboard will load and display sensor data.

---

## 🧪 Step 7: Test Sensors
- Place your finger on **MAX30100** and **DS18B20** sensors.  
- Press your hand firmly on the **FRS1 sensor**.  
- Tilt the **MPU6050 sensor** to test posture detection.  
- All sensor data from Arduino will appear on the dashboard.

---

## 🔌 Step 8: Disconnect
- Close the Wi‑Fi connection.  
- Remove both cables from the laptop ports.

---
