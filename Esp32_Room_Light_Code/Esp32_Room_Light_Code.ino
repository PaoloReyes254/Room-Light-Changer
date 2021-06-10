#include <LiquidCrystal.h>
#include <WiFi.h>
#include "FirebaseESP32.h"
#include "time.h"

//Wifi
#define ssid "wifi martinez"
#define password "contrasena1"

//Firebase
#define host "https://room-light-changer-default-rtdb.firebaseio.com/"
#define auth "f3u1Q3x9vT7ktn4uBPAxoqfWt4pTy9gZn2oy1spm"

FirebaseData firebaseData;

//LCD
#define rs  22
#define en  21
#define d4  5
#define d5  18
#define d6  23
#define d7  19

LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//Time
#define ntpServer "pool.ntp.org"
#define gmtOffset_sec -21600
#define daylightOffset_sec 3600

//Global Variables
uint8_t previousEstado;
uint8_t estado;
uint8_t hora;
uint8_t minuto;
uint8_t segundo;
uint8_t horaGoal;
uint8_t minutoGoal;
uint8_t segundoGoal;
uint8_t previousPeople = 0;
uint8_t peopleBefore = 0;
uint8_t h;
uint8_t t;
uint8_t previousH;
uint8_t previousT;
uint8_t resetear;
volatile uint8_t currentImage = 0;

volatile int8_t people = 1;

int64_t secondDifference;

bool cambioEstado = false;
bool initialization = true;
bool enable = false;
volatile bool change;
volatile bool execute = false;

struct tm timeinfo;

hw_timer_t * timer = NULL;

byte wifiChar[] = {
  B00000,
  B01110,
  B10001,
  B00100,
  B01010,
  B00000,
  B00100,
  B00000
};

void IRAM_ATTR changeDisplay() {
  currentImage = !currentImage;
  change = true;
}

void IRAM_ATTR onTimer() {
  execute = true;
}

void setup() {
  Serial.begin(115200);
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);
  pinMode(26, INPUT_PULLUP);
  attachInterrupt(26, changeDisplay, FALLING);
  pinMode(33, INPUT_PULLUP);
  pinMode(14, OUTPUT);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_33, 0);
  touch_pad_intr_disable();
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("Welcome");
  lcd.setCursor(1, 1);
  lcd.print("By: Paolo Reyes");
  delay(2500);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting to:");
  lcd.setCursor(3, 1);
  lcd.print(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
  }
  delay(1000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connected");
  Serial.println("Connected");
  delay(1000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Setting time...");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Failed to config");
    lcd.setCursor(0, 1);
    lcd.print("Retrying...");
    while (!getLocalTime(&timeinfo)) {
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    }
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Time: ");
  lcd.setCursor(6, 0);
  lcd.print(&timeinfo, "%H:%M:%S");
  lcd.createChar(1, wifiChar);
  delay(1000);
  Firebase.begin(host, auth);
  Firebase.reconnectWiFi(true);
  Firebase.setInt(firebaseData, "/People", people);
  Firebase.setString(firebaseData, "/LightDependency", "Light depends on number of people");
  Firebase.setInt(firebaseData, "/Estado", 0);
  previousEstado = 0;
  getDifference();
  if (secondDifference > 0) {
    Firebase.setString(firebaseData, "/Status", "OFF, Paolo is sleeping");
    Firebase.setString(firebaseData, "/TH/H", "Unavailable");
    Firebase.setString(firebaseData, "/TH/T", "Unavailable");
    Firebase.setString(firebaseData, "/Light", "Unavailable");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    secondDifference = secondDifference * 1000000;
    if (secondDifference < 10440000000) {
      esp_sleep_enable_timer_wakeup(secondDifference);
    } else {
      esp_sleep_enable_timer_wakeup(10440000000);
    }
    digitalWrite(4, LOW);
    esp_deep_sleep_start();
  }
  getOutput();
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 1500000, true);
  timerAlarmEnable(timer);
}

void loop() {
  if (secondDifference <= 0) {
    if (initialization) {
      enable = true;
      Firebase.setString(firebaseData, "/Status", "ON");
      initialization = false;
    }
  } else {
    if (!estado) {
      enable = false;
      Firebase.setString(firebaseData, "/Status", "OFF, ESP32 is sleeping and will wake up at alarm time");
      Firebase.setString(firebaseData, "/TH/H", "Unavailable");
      Firebase.setString(firebaseData, "/TH/T", "Unavailable");
      Firebase.setString(firebaseData, "/Light", "Unavailable");
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      //Serial.println("Wifi OFF");
      secondDifference = secondDifference * 1000000;
      if (secondDifference < 10440000000) {
        esp_sleep_enable_timer_wakeup(secondDifference);
      } else {
        esp_sleep_enable_timer_wakeup(10440000000);
      }
      digitalWrite(4, LOW);
      esp_deep_sleep_start();
    } else {
      Firebase.setString(firebaseData, "/Status", "Waiting for Paolo to turn OFF the light to go sleep");
      getOutput();
      getLocalTime(&timeinfo);
      hora = timeinfo.tm_hour;
      minuto = timeinfo.tm_min;
      segundo = timeinfo.tm_sec;
      secondDifference = ((horaGoal * 3600) + (minutoGoal * 60) + segundoGoal) - ((hora * 3600) + (minuto * 60) + segundo);
      initialization = true;
    }
  }
  if (enable) {
    if (!currentImage && change) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("People Inside:");
      lcd.setCursor(15, 0);
      lcd.print(people);
      lcd.setCursor(0, 1);
      lcd.print(ssid);
      lcd.setCursor(14, 1);
      lcd.write(byte(1));
      previousPeople = people;
      change = false;
    } else  if (currentImage && change) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Temperature: ");
      lcd.print(t);
      lcd.print("C");
      lcd.setCursor(0, 1);
      lcd.print("Humidity: ");
      lcd.print(h);
      lcd.print("%");
      previousT = t;
      previousH = h;
      change = false;
    }
  }
  if (execute) {
    getOutput();
    getDifference();
    execute = false;
  }
  if (Serial.available() > 0) {
    String dataReceived;
    dataReceived = Serial.readStringUntil('\n');
    if (dataReceived.startsWith("P")) {
      people = dataReceived.substring(2, dataReceived.length()).toInt();
      Firebase.setInt(firebaseData, "/People", people);
      change = true;
    } else if (dataReceived.startsWith("T")) {
      t = dataReceived.substring(2, dataReceived.length()).toInt();
      Firebase.setInt(firebaseData, "/TH/T", t);
      change = true;
    } else if (dataReceived.startsWith("H")) {
      h = dataReceived.substring(2, dataReceived.length()).toInt();
      Firebase.setString(firebaseData, "/TH/H", String(h) + "%");
      change = true;
    } else if (dataReceived.startsWith("R")) {
      uint8_t value = dataReceived.substring(2, dataReceived.length()).toInt();
      digitalWrite(14, value);
    }
  }
}

void getDifference() {
  getLocalTime(&timeinfo);
  hora = timeinfo.tm_hour;
  minuto = timeinfo.tm_min;
  segundo = timeinfo.tm_sec;
  Firebase.getInt(firebaseData, "/Fecha/Hora");
  horaGoal = firebaseData.intData();
  Firebase.getInt(firebaseData, "/Fecha/Minuto");
  minutoGoal = firebaseData.intData();
  Firebase.getInt(firebaseData, "/Fecha/Segundo");
  segundoGoal = firebaseData.intData();
  /*Serial.print("Hora alarma: \t");
    Serial.print(horaGoal);
    Serial.print(":");
    Serial.print(minutoGoal);
    Serial.print(":");
    Serial.println(segundoGoal);*/
  secondDifference = ((horaGoal * 3600) + (minutoGoal * 60) + segundoGoal) - ((hora * 3600) + (minuto * 60) + segundo);
}

void getOutput() {
  Firebase.getInt(firebaseData, "/Estado");
  estado = firebaseData.intData();
  uint8_t tempEstado;
  Firebase.getInt(firebaseData, "/Estado");
  tempEstado = firebaseData.intData();
  while (estado != tempEstado) {
    Firebase.getInt(firebaseData, "/Estado");
    estado = firebaseData.intData();
    Firebase.getInt(firebaseData, "/Estado");
    tempEstado = firebaseData.intData();
  }
  Firebase.getInt(firebaseData, "/Reset");
  resetear = firebaseData.intData();
  if (estado != previousEstado) {
    if (estado) {
      Firebase.setString(firebaseData, "/Light", "ON");
      Firebase.setString(firebaseData, "/LightDependency", "Light depends on ON/OFF buttons");
      dacWrite(25, 255);
    } else {
      Firebase.setString(firebaseData, "/Light", "OFF");
      Firebase.setString(firebaseData, "/LightDependency", "Light depends on ON/OFF buttons");
      dacWrite(25, 0);
    }
    cambioEstado = true;
  }
  if (resetear) {
    cambioEstado = false;
    Firebase.setInt(firebaseData, "/Reset", 0);
    Firebase.setString(firebaseData, "/LightDependency", "Light depends on number of people");
    if (people) {
      Firebase.setString(firebaseData, "/Light", "ON");
    } else {
      Firebase.setString(firebaseData, "/Light", "OFF");
    }
    dacWrite(25, 127);
  }
  if (!cambioEstado) {
    if (people) {
      if (peopleBefore == 0) {
        estado = 1;
        Firebase.setString(firebaseData, "/Light", "ON");
        Firebase.setInt(firebaseData, "/Estado", 1);
        dacWrite(25, 127);
      }
    } else {
      if (peopleBefore > 0) {
        estado = 0;
        Firebase.setString(firebaseData, "/Light", "OFF");
        Firebase.setInt(firebaseData, "/Estado", 0);
        dacWrite(25, 127);
      }
    }
    peopleBefore = people;
  }
  previousEstado = estado;
}
