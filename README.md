# Desktop Clock DIY

This is a simple Arduino-based desktop clock with PC App support. The clock can be used to display custom messages, set alerts, add alarms, and act as a personal desktop timekeeper. Only easily available items are used to build this!

The project features a DS3231 Real-Time Clock (RTC) and a 16x2 I2C LCD, paired with a standalone Python desktop application (`Tkinter` + `PySerial`) to control the hardware over USB.

<img width="450" alt="Desktop Clock Hardware" src="https://github.com/user-attachments/assets/18075749-0317-4910-b407-6878ea1e8fc6" />

---

##  Features
* **Real-Time Clock & Thermometer:** Displays 24-hour time, full date, day of the week, and live ambient temperature (`°C`).
* **One-Click PC Time Sync:** Syncs the DS3231 RTC module to your computer's clock instantly or allows custom manual date/time entry.
* **Smooth Scrolling Text Engine:** Automatically scrolls long daily reminders and temporary custom messages (up to 60 characters).
* **Daily Sound Alarm:** Triggers a high-pitched double-beep pattern and visual alert banner.
* **Hourly Westminster Chime:** Plays the full 16-note Westminster melody on the hour. Can be toggled ON/OFF from the app.

---

##  Hardware Pinout 

| Component | Pin | Arduino Pin (Uno / Nano) |
| :--- | :--- | :--- |
| **16x2 I2C LCD (`0x27`)** | SDA / SCL / VCC / GND | `A4` / `A5` / `5V` / `GND` |
| **DS3231 RTC (`0x68`)** | SDA / SCL / VCC / GND | `A1` / `A2` *(SoftwareWire)* / `5V` / `GND` |
| **Passive Buzzer** | Positive (+) / Negative (-) | `D8` / `GND` |
| **LCD Backlight Dimmer** | PWM Control | `D9` |

---

##  How to Run the PC App

Ensure you have Python installed, then install the required serial library:
```bash
pip install pyserial
python ClockApp.py
```
---

##  Arduino Dependencies
Before compiling `clock.ino`, ensure you have installed the following libraries via the Arduino IDE Library Manager:
* **LiquidCrystal I2C** by Frank de Brabander
* **SoftwareWire** by Testato

---

##  Future Improvements (To-Do)
* Save alarm and reminder settings to the Arduino's EEPROM so they survive power outages.
* Add a 12-hour / 24-hour format toggle.
* Add a "Night Mode" to automatically mute the Westminster chime between 10:00 PM and 7:00 AM.

---

##  License
This project is open-source and available under the MIT License. Feel free to fork, modify, and build your own versions!
