from flask import Flask, jsonify, render_template
import serial
import json
import threading
import time
import requests


# ==============================
# CONFIG
# ==============================
PORT = "COM3"
BAUD = 115200
ESP32_IP = "http://10.109.160.207"

app = Flask(__name__)

# ==============================
# GLOBAL DATA
# ==============================
latest_data = {
    "seat": "--",
    "posture": "--",
    "pitch": 0.0,
    "temp": 0.0,
    "hr": 0,
    "spo2": 0.0
}

# 🔥 student_key → history
student_data = {}

# 🔥 list of {id, name}
current_students = []

data_lock = threading.Lock()
ser = None

# ==============================
# SERIAL INIT
# ==============================
def init_serial():
    global ser
    try:
        ser = serial.Serial(PORT, BAUD, timeout=1)
        time.sleep(2)
        print(f"✅ Serial connected to {PORT}")
    except Exception as e:
        print(f"❌ Serial error: {e}")
        ser = None

# ==============================
# READ ARDUINO DATA
# ==============================
def read_serial():
    global latest_data

    while True:
        try:
            if ser is None or not ser.is_open:
                print("🔄 Connecting to Arduino...")
                init_serial()
                time.sleep(2)
                continue

            if ser.in_waiting > 0:
                line = ser.readline().decode(errors="ignore").strip()

                if not line:
                    continue

                print("RAW ->", line)

                if line.startswith("{") and line.endswith("}"):
                    data = json.loads(line)

                    with data_lock:
                        latest_data.update({
                            "seat": data.get("seat", "--"),
                            "posture": data.get("posture", "--"),
                            "pitch": float(data.get("pitch", 0)),
                            "temp": float(data.get("temp", 0)),
                            "hr": int(data.get("hr", 0)),
                            "spo2": float(data.get("spo2", 0))
                        })

                        # 🔥 assign to detected students
                        for person in current_students:
                            student_name = person["name"]
                            student_id = person["id"]

                            key = f"{student_id}_{student_name}"

                            if key not in student_data:
                                student_data[key] = []

                            student_data[key].append({
                                "name": student_name,
                                "posture": latest_data["posture"],
                                "temp": latest_data["temp"],
                                "hr": latest_data["hr"],
                                "spo2": latest_data["spo2"]
                            })

                            if len(student_data[key]) > 200:
                                student_data[key].pop(0)

                    print("DATA ->", latest_data)

        except Exception as e:
            print("Serial error:", e)
            time.sleep(1)

# ==============================
# FETCH ESP32 ATTENDANCE
# ==============================
def fetch_attendance():
    global current_students

    while True:
        try:
            res = requests.get(f"{ESP32_IP}/attendance", timeout=2)
            data = res.json()

            current_students = data

            print("👤 Detected:", current_students)

        except Exception as e:
            print("Attendance error:", e)

        time.sleep(3)

# ==============================
# ANALYSIS
# ==============================
def analyze_metrics(history):
    hrs = [d["hr"] for d in history if d["hr"] > 0]
    temps = [d["temp"] for d in history if d["temp"] > 0]
    spo2 = [d["spo2"] for d in history if d["spo2"] > 0]
    bad_posture = [d for d in history if d["posture"] != "Good"]

    avg_hr = sum(hrs)/len(hrs) if hrs else 0
    max_hr = max(hrs) if hrs else 0

    avg_temp = sum(temps)/len(temps) if temps else 0
    max_temp = max(temps) if temps else 0

    avg_spo2 = sum(spo2)/len(spo2) if spo2 else 0

    posture_ratio = len(bad_posture)/len(history) if history else 0

    return avg_hr, max_hr, avg_temp, max_temp, avg_spo2, posture_ratio

# ==============================
# ROUTES
# ==============================
@app.route("/")
def index():
    return render_template("dashboard.html")

@app.route("/data")
def data():
    return jsonify(latest_data)

@app.route("/students")
def students():
    return jsonify(current_students)


# ==============================
# RUN
# ==============================
if __name__ == "__main__":
    threading.Thread(target=read_serial, daemon=True).start()
    threading.Thread(target=fetch_attendance, daemon=True).start()

    print("\n🌐 Server running at ", ESP32_IP)

    app.run(host="0.0.0.0", port=5000, debug=False)