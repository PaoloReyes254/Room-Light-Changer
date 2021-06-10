#include "DHT.h"

//DHT
#define DHTPIN 8
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

uint8_t sensor1[] = {2, 3};
uint8_t sensor2[] = {4, 5};
int8_t people = 1;
uint8_t previousPeople = 1;
uint8_t h;
uint8_t t;
uint8_t previousH;
uint8_t previousT;
uint8_t modeOfLight;
uint8_t previousMode;

uint32_t sensor1Val;
uint32_t sensor2Val;

uint8_t state[] = {0, 0};

bool firstTime = false;

uint64_t currentSeconds = 0;

ISR(PCINT2_vect) {
  if (PIND >> 7 & (1 << DDD7) >> 7) {
    people = 1;
    Serial.print("P:");
    Serial.println(people);
  }
}

void setup() {
  cli();
  pinMode(2, INPUT);
  pinMode(3, OUTPUT);
  pinMode(4, INPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, INPUT_PULLUP);
  pinMode(9, OUTPUT);
  pinMode(11, OUTPUT);
  PCICR |= B00000100;
  PCMSK2 |= B10000000;
  Serial.begin(115200);
  dht.begin();
  Serial.print("P:");
  Serial.println(people);
  sei();
}

void loop() {
  sensor1Val = measureDistance(sensor1);
  sensor2Val = measureDistance(sensor2);
  getPeople();
  if (modeOfLight != previousMode || people != previousPeople) {
    if (modeOfLight == 0) {
      if (people) {
        digitalWrite(9, 1);
        Serial.println("R: 1");
        digitalWrite(11, 0);
      } else {
        digitalWrite(9, 0);
        Serial.println("R: 0");
        digitalWrite(11, 1);
      }
    } else if (modeOfLight == 1) {
      digitalWrite(9, 0);
      Serial.println("R: 0");
      digitalWrite(11, 1);
    } else if (modeOfLight == 2) {
      digitalWrite(9, 1);
      Serial.println("R: 1");
      digitalWrite(11, 0);
    }
    previousMode = modeOfLight;
    previousPeople = people;
  }
  h = dht.readHumidity();
  t = dht.readTemperature();
  if (h != previousH) {
    Serial.print("H:");
    Serial.println(h);
    previousH = h;
  }
  if (t != previousT) {
    Serial.print("T:");
    Serial.println(t);
    previousT = t;
  }
  uint16_t reading = analogRead(A0);
  if (reading > 100 && reading < 450) {
    modeOfLight = 0;
  } else if (reading < 100) {
    modeOfLight = 1;
  } else if (reading > 450) {
    modeOfLight = 2;
  }
}

void getPeople() {
  if (sensor1Val < 45 && state[0] == 0 && state[1] == 0) {
    state[0] = 1;
    state[1] = 0;
  }
  if (sensor2Val < 45 && state[0] == 1 && state[1] == 0) {
    people++;
    digitalWrite(6, HIGH);
    delay(200);
    digitalWrite(6, LOW);
    Serial.print("P:");
    Serial.println(people);
    state[0] = 0;
    state[1] = 0;
    firstTime = true;
    delay(300);
  }
  sensor1Val = measureDistance(sensor1);
  sensor2Val = measureDistance(sensor2);
  if (sensor2Val < 45 && state[0] == 0 && state[1] == 0) {
    state[0] = 0;
    state[1] = 1;
  }
  if (sensor1Val < 45 && state[0] == 0 && state[1] == 1) {
    people--;
    if (people < 0) {
      people = 0;
    }
    digitalWrite(6, HIGH);
    delay(100);
    digitalWrite(6, LOW);
    delay(50);
    digitalWrite(6, HIGH);
    delay(100);
    digitalWrite(6, LOW);
    Serial.print("P:");
    Serial.println(people);
    state[0] = 0;
    state[1] = 0;
    firstTime = true;
    delay(250);
  }
  if (state[0] != 0 || state[1] != 0) {
    if (firstTime) {
      currentSeconds = round(millis() / 1000);
      firstTime = false;
    }
    if (round(millis() / 1000) - currentSeconds >= 1) {
      state[0] = 0;
      state[1] = 0;
      firstTime = true;
    }
  }
}

uint8_t measureDistance(uint8_t a[]) {
  digitalWrite(a[1], LOW);
  delayMicroseconds(2);
  digitalWrite(a[1], HIGH);
  delayMicroseconds(10);
  digitalWrite(a[1], LOW);
  long duration = pulseIn(a[0], HIGH, 100000);
  return duration / 29 / 2;
}
