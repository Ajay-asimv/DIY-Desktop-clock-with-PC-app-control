#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareWire.h>
#include <string.h>
#include <stdio.h>

const uint8_t LCD_ADDRESS = 0x27;
const uint8_t LCD_COLUMNS = 16;
const uint8_t LCD_ROWS = 2;
const uint8_t RTC_ADDRESS = 0x68;
const uint8_t RTC_SDA_PIN = A1;
const uint8_t RTC_SCL_PIN = A2;
const uint8_t DIMMER_PIN = 9;
const uint8_t DIMMER_LEVEL = 18;
const unsigned long DISPLAY_REFRESH_MS = 200;
const uint8_t BUZZER_PIN = 8;

LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);
SoftwareWire rtcWire(RTC_SDA_PIN, RTC_SCL_PIN);

const char daysOfTheWeek[7][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char monthsOfTheYear[12][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
const byte degreeSymbol[8] = {B01100, B10010, B10010, B01100, B00000, B00000, B00000, B00000};

struct DateTime {
  uint16_t year; uint8_t month; uint8_t day; uint8_t dayOfWeek;
  uint8_t hour; uint8_t minute; uint8_t second;
};

unsigned long lastDisplayRefreshMs = 0;

// Reminder Memory
int reminderHour = -1;
int reminderMinute = -1;
char reminderText[64] = "";
bool isReminderActive = false;

// Alarm Memory
int alarmHour = -1;
int alarmMinute = -1;
bool isAlarmActive = false;
int lastAlarmBeepSecond = -1;

// Custom Message & Status Popup Memory
char customMsgHeader[17] = "  -- MESSAGE -- ";
char customMsgText[64] = "";
bool isCustomMsgActive = false;
unsigned long customMsgStartTime = 0;
unsigned long customMsgDurationMs = 0;

// Chime Tracker & Toggle
bool chimeEnabled = true;
bool hasChimedThisHour = false;

// Musical Notes
#define NOTE_G3  196
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_C3  131
#define NOTE_A5  880 // High-pitched note for Alarm

uint8_t bcdToDec(uint8_t val) { return (val / 16 * 10) + (val % 16); }
uint8_t decToBcd(uint8_t val) { return (val / 10 * 16) + (val % 10); }

bool readRtcRegisters(uint8_t startRegister, uint8_t *buffer, uint8_t length) {
  rtcWire.beginTransmission(RTC_ADDRESS);
  rtcWire.write(startRegister);
  if (rtcWire.endTransmission() != 0) return false;
  rtcWire.requestFrom(RTC_ADDRESS, length);
  for (uint8_t i = 0; i < length; i++) {
    if (rtcWire.available()) buffer[i] = rtcWire.read();
    else return false;
  }
  return true;
}

void setRtcTime(const DateTime &dt) {
  rtcWire.beginTransmission(RTC_ADDRESS);
  rtcWire.write(0x00);
  rtcWire.write(decToBcd(dt.second));
  rtcWire.write(decToBcd(dt.minute));
  rtcWire.write(decToBcd(dt.hour));
  rtcWire.write(decToBcd(dt.dayOfWeek));
  rtcWire.write(decToBcd(dt.day));
  rtcWire.write(decToBcd(dt.month));
  rtcWire.write(decToBcd(dt.year % 100)); 
  rtcWire.endTransmission();
}

DateTime getRtcTime() {
  DateTime dt = {0};
  uint8_t buffer[7];
  if (readRtcRegisters(0x00, buffer, 7)) {
    dt.second = bcdToDec(buffer[0] & 0x7F);
    dt.minute = bcdToDec(buffer[1]);
    dt.hour = bcdToDec(buffer[2] & 0x3F);
    dt.dayOfWeek = bcdToDec(buffer[3]);
    dt.day = bcdToDec(buffer[4]);
    dt.month = bcdToDec(buffer[5] & 0x1F);
    dt.year = 2000 + bcdToDec(buffer[6]);
  }
  return dt;
}

float getRtcTemperature() {
  static float smoothedTemp = -100.0f;
  static unsigned long lastTempReadMs = 0;

  // Only read the sensor once every 10 seconds (10000 ms)
  if (smoothedTemp == -100.0f || millis() - lastTempReadMs >= 10000UL) {
    lastTempReadMs = millis();
    uint8_t buffer[2];
    if (readRtcRegisters(0x11, buffer, 2)) {
      float rawTemp = (float)(int8_t)buffer[0] + ((buffer[1] >> 6) * 0.25f);
      
      if (smoothedTemp == -100.0f) {
        smoothedTemp = rawTemp; // First reading on boot
      } else {
        // Smooth filter: 80% old reading + 20% new reading
        smoothedTemp = (smoothedTemp * 0.8f) + (rawTemp * 0.2f);
      }
    }
  }
  return smoothedTemp;
}

void printPadded(const char* text) {
  char buf[LCD_COLUMNS + 1];
  int len = strlen(text);
  if (len > LCD_COLUMNS) len = LCD_COLUMNS;
  memcpy(buf, text, len);
  for (int i = len; i < LCD_COLUMNS; i++) buf[i] = ' ';
  buf[LCD_COLUMNS] = '\0';
  lcd.print(buf);
}

void printDigits(int digits) {
  if (digits < 10) lcd.print('0');
  lcd.print(digits);
}

void printScrollingLine(const char* text, int row) {
  int len = strlen(text);
  if (len <= LCD_COLUMNS) {
    lcd.setCursor(0, row);
    printPadded(text);
    return;
  }
  int maxScroll = len - LCD_COLUMNS;
  int frame = (millis() / 400) % (maxScroll + 6); 
  int startIdx = frame;
  
  if (startIdx < 2) startIdx = 0; 
  else startIdx -= 2; 
  
  if (startIdx > maxScroll) startIdx = maxScroll; 

  char view[LCD_COLUMNS + 1];
  strncpy(view, text + startIdx, LCD_COLUMNS);
  view[LCD_COLUMNS] = '\0';
  
  lcd.setCursor(0, row);
  lcd.print(view);
}

// --- NEW HELPER: Show Temporary Status Popup on LCD ---
void showPopup(const char* header, const char* body, unsigned long durationMs) {
  strncpy(customMsgHeader, header, 16);
  customMsgHeader[16] = '\0';
  strncpy(customMsgText, body, sizeof(customMsgText) - 1);
  customMsgText[sizeof(customMsgText) - 1] = '\0';
  isCustomMsgActive = true;
  customMsgStartTime = millis();
  customMsgDurationMs = durationMs;
}

void playWestminsterChime(int currentHour) {
  int melody[] = {
    NOTE_E4, NOTE_C4, NOTE_D4, NOTE_G3,
    NOTE_G3, NOTE_D4, NOTE_E4, NOTE_C4,
    NOTE_E4, NOTE_D4, NOTE_C4, NOTE_G3,
    NOTE_G3, NOTE_D4, NOTE_E4, NOTE_C4
  };

  for (int i = 0; i < 16; i++) {
    tone(BUZZER_PIN, melody[i]);
    delay(400);
    noTone(BUZZER_PIN);
    delay(100);
    if ((i + 1) % 4 == 0) delay(500);
  }

  delay(600);

  int strikes = currentHour % 12;
  if (strikes == 0) strikes = 12;

  for (int i = 0; i < strikes; i++) {
    tone(BUZZER_PIN, NOTE_C3);
    delay(800);
    noTone(BUZZER_PIN);
    delay(600);
  }
}

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  lcd.createChar(0, (uint8_t*)degreeSymbol);
  rtcWire.begin();
  
  pinMode(DIMMER_PIN, OUTPUT);
  analogWrite(DIMMER_PIN, DIMMER_LEVEL);
  pinMode(BUZZER_PIN, OUTPUT);
}

void loop() {
  // --- LISTEN FOR PC COMMANDS ---
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    // Time Sync: T,YYYY,MM,DD,DOW,HH,MM,SS
    if (command.startsWith("T,")) {
      DateTime newTime;
      int y, m, d, dow, h, min, s;
      sscanf(command.c_str(), "T,%d,%d,%d,%d,%d,%d,%d", &y, &m, &d, &dow, &h, &min, &s);
      newTime.year = y; newTime.month = m; newTime.day = d; newTime.dayOfWeek = dow;
      newTime.hour = h; newTime.minute = min; newTime.second = s;
      setRtcTime(newTime);
      Serial.println("OK_TIME");
    }
    // Reminder: R,HH,MM,Text
    else if (command.startsWith("R,")) {
      int comma1 = command.indexOf(',');
      int comma2 = command.indexOf(',', comma1 + 1);
      int comma3 = command.indexOf(',', comma2 + 1);
      
      reminderHour = command.substring(comma1 + 1, comma2).toInt();
      reminderMinute = command.substring(comma2 + 1, comma3).toInt();
      String remText = command.substring(comma3 + 1);
      
      remText.toCharArray(reminderText, sizeof(reminderText));
      isReminderActive = true;
      Serial.println("OK_REM");
    }
    else if (command == "CLEAR_REM") {
      isReminderActive = false;
      Serial.println("OK_CLEAR_REM");
    }
    // Alarm Set: A,HH,MM
    else if (command.startsWith("A,")) {
      int comma1 = command.indexOf(',');
      int comma2 = command.indexOf(',', comma1 + 1);
      alarmHour = command.substring(comma1 + 1, comma2).toInt();
      alarmMinute = command.substring(comma2 + 1).toInt();
      isAlarmActive = true;

      // Display 3-second confirmation on LCD
      char buf[17];
      snprintf(buf, sizeof(buf), " Set for %02d:%02d  ", alarmHour, alarmMinute);
      showPopup("  ** ALARM **   ", buf, 3000UL);

      Serial.println("OK_ALARM");
    }
    // Alarm Clear / Stop
    else if (command == "CLEAR_ALM") {
      isAlarmActive = false;
      noTone(BUZZER_PIN);

      // Display 3-second confirmation on LCD
      showPopup("  ** ALARM **   ", "   ALARM OFF    ", 3000UL);

      Serial.println("OK_CLEAR_ALM");
    }
    // Hourly Chime Toggle: C,1 or C,0
    else if (command.startsWith("C,")) {
      int state = command.substring(2).toInt();
      chimeEnabled = (state == 1);

      // Display 3-second confirmation on LCD
      if (chimeEnabled) {
        showPopup(" ** CHIME **    ", "   CHIME ON     ", 3000UL);
      } else {
        showPopup(" ** CHIME **    ", "   CHIME OFF    ", 3000UL);
      }

      Serial.println("OK_CHIME");
    }
    // Custom Message: M,Seconds,Text
    else if (command.startsWith("M,")) {
      int comma1 = command.indexOf(',');
      int comma2 = command.indexOf(',', comma1 + 1);
      
      int durationSec = command.substring(comma1 + 1, comma2).toInt();
      String msgText = command.substring(comma2 + 1);
      
      showPopup("  -- MESSAGE -- ", msgText.c_str(), durationSec * 1000UL);
      Serial.println("OK_MSG");
    }
  }

  if (millis() - lastDisplayRefreshMs < DISPLAY_REFRESH_MS) return; 
  lastDisplayRefreshMs = millis();

  DateTime dt = getRtcTime();

  // --- TRIGGER HOURLY CHIME (Only if enabled) ---
  if (chimeEnabled && dt.minute == 0 && dt.second == 0 && !hasChimedThisHour) {
    playWestminsterChime(dt.hour);
    hasChimedThisHour = true;
  } else if (dt.minute != 0) {
    hasChimedThisHour = false;
  }

  // --- PRIORITY 1: Custom Temporary Message or Status Popup ---
  if (isCustomMsgActive) {
    if (millis() - customMsgStartTime > customMsgDurationMs) {
      isCustomMsgActive = false;
    } else {
      lcd.setCursor(0, 0);
      printPadded(customMsgHeader);
      printScrollingLine(customMsgText, 1);
      return; 
    }
  }

  // --- PRIORITY 2: ALARM (Beeps every second during the alarm minute) ---
  if (isAlarmActive && dt.hour == alarmHour && dt.minute == alarmMinute) {
    lcd.setCursor(0, 0);
    printPadded("  ** ALARM **   ");
    lcd.setCursor(0, 1);
    char almLine[17];
    snprintf(almLine, sizeof(almLine), "  TIME: %02d:%02d   ", dt.hour, dt.minute);
    printPadded(almLine);

    if (dt.second != lastAlarmBeepSecond) {
      lastAlarmBeepSecond = dt.second;
      tone(BUZZER_PIN, NOTE_A5, 150);
      delay(200);
      tone(BUZZER_PIN, NOTE_A5, 150);
    }
    return;
  }

  // --- PRIORITY 3: Daily Reminder ---
  if (isReminderActive && dt.hour == reminderHour && dt.minute == reminderMinute) {
    lcd.setCursor(0, 0);
    printPadded(" !! REMINDER !! ");
    printScrollingLine(reminderText, 1);
    if (dt.second == 0) {
       tone(BUZZER_PIN, NOTE_E4, 200); 
    }
    return;
  }

  // --- PRIORITY 4: Normal Clock Display ---
  float temp = getRtcTemperature();

  lcd.setCursor(0, 0);
  printDigits(dt.hour); lcd.print(":"); printDigits(dt.minute); lcd.print(":"); printDigits(dt.second);
  lcd.print("  "); lcd.setCursor(10, 0); lcd.print(temp, 1); lcd.write(0); lcd.print("C");

  lcd.setCursor(0, 1);
  char line[LCD_COLUMNS + 1];
  snprintf(line, sizeof(line), "%s, %02d %s %04d", daysOfTheWeek[dt.dayOfWeek - 1], dt.day, monthsOfTheYear[dt.month - 1], dt.year);
  printPadded(line);
}