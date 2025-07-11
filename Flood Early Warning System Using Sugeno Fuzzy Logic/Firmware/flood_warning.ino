#include <Adafruit_LiquidCrystal.h>
Adafruit_LiquidCrystal lcd(0);

// === PIN SETUP ===
#define trigpin 3
#define echopin 4
int ledlow = 5;
int ledmiddle = 6;
int ledhigh = 7;
int buzzer = 9;
int dangerlamp = 8;

// === TIMER LCD ===
unsigned long previousMillis = 0;
int screenMode = 0;
const unsigned long intervalJarak = 3000;
const unsigned long intervalSuhu = 3000;
const unsigned long intervalCahaya = 3000;

// === INPUT SUHU & CAHAYA ===
#define tmp36Pin A0
#define lightSensorPin A1

// === TONE BUZZER NON-BLOCKING ===
int tone1 = 0, dur1 = 0, tone2 = 0, dur2 = 0;
unsigned long patternStart = 0;
bool tonePhase = true;

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  pinMode(trigpin, OUTPUT);
  pinMode(echopin, INPUT);
  pinMode(ledlow, OUTPUT);
  pinMode(ledmiddle, OUTPUT);
  pinMode(ledhigh, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(dangerlamp, OUTPUT);
}

void loop() {
  long distance = getDistance();
  float suhu = getSuhu();
  int cahaya = analogRead(lightSensorPin);

  float skor = fuzzySugeno(distance);
  updateIndikator(skor);

  showLCD(distance, suhu, cahaya);
  delay(50);

  updateTone(); // Pastikan buzzer tetap berjalan non-blocking
}

// === AMBIL JARAK ULTRASONIK ===
long getDistance() {
  digitalWrite(trigpin, LOW); delayMicroseconds(2);
  digitalWrite(trigpin, HIGH); delayMicroseconds(10);
  digitalWrite(trigpin, LOW);
  long duration = pulseIn(echopin, HIGH, 30000);
  if (duration == 0) return 999; // error sensor
  return (duration / 2) / 29.1;
}

// === SUHU TMP36 ===
float getSuhu() {
  int analogValue = analogRead(tmp36Pin);
  float voltage = analogValue * (5.0 / 1024.0);
  return (voltage - 0.5) * 100.0;
}

// === FUZZY SUGENO DARI JARAK  ===
float fuzzySugeno(long jarak) {
  if (jarak == 999) return 0; 
  float mu[6] = {0};
  float z[6]  = {100, 80, 60, 40, 20, 10}; // Output untuk masing-masing fuzzy set

  // Pendek
  if (jarak <= 0) mu[0] = 1;
  else if (jarak > 0 && jarak < 50) mu[0] = (50 - jarak) / 50.0;
  else mu[0] = 0;

  // Cukup Dekat
  if (jarak >= 50 && jarak < 75) mu[1] = (jarak - 50) / 25.0;
  else if (jarak >= 75 && jarak < 100) mu[1] = (100 - jarak) / 25.0;
  else mu[1] = 0;

  // Sedang
  if (jarak >= 100 && jarak < 125) mu[2] = (jarak - 100) / 25.0;
  else if (jarak >= 125 && jarak < 150) mu[2] = (150 - jarak) / 25.0;
  else mu[2] = 0;

  // Agak Jauh
  if (jarak >= 150 && jarak < 175) mu[3] = (jarak - 150) / 25.0;
  else if (jarak >= 175 && jarak < 200) mu[3] = (200 - jarak) / 25.0;
  else mu[3] = 0;

  // Jauh
  if (jarak >= 200 && jarak < 240) mu[4] = (jarak - 200) / 40.0;
  else if (jarak >= 240 && jarak < 280) mu[4] = (280 - jarak) / 40.0;
  else mu[4] = 0;

  // Sangat Jauh
  if (jarak >= 280 && jarak < 305) mu[5] = (jarak - 280) / 25.0;
  else if (jarak >= 305) mu[5] = 1;
  else mu[5] = 0;

  float atas = 0, bawah = 0;
  for (int i = 0; i < 6; i++) {
    atas += mu[i] * z[i];
    bawah += mu[i];
  }
  if (bawah == 0) return 0;
  return atas / bawah;
}

// === OUTPUT BERDASARKAN SKOR ===
void updateIndikator(float skor) {
  digitalWrite(ledlow, LOW);
  digitalWrite(ledmiddle, LOW);
  digitalWrite(ledhigh, LOW);
  noTone(buzzer);
  tone1 = tone2 = dur1 = dur2 = 0; // Reset pola buzzer
  digitalWrite(dangerlamp, LOW);

  if (skor >= 90) {
    digitalWrite(dangerlamp, HIGH); 
    tonePattern(1000, 100, 1500, 100);
  } else if (skor >= 70) {
    digitalWrite(ledhigh, HIGH);
    tonePattern(1000, 300, 0, 50);
  } else if (skor >= 60) {
    digitalWrite(ledhigh, HIGH);
    tonePattern(800, 300, 0, 50);
  } else if (skor >= 35) {
    digitalWrite(ledmiddle, HIGH);
  } else {
    digitalWrite(ledlow, HIGH);
  }
}

// === NON-BLOCKING BUZZER PATTERN ===
void tonePattern(int t1, int d1, int t2, int d2) {
  tone1 = t1; dur1 = d1;
  tone2 = t2; dur2 = d2;
  patternStart = millis();
  tonePhase = true;
  if (t1 > 0) tone(buzzer, t1);
  else noTone(buzzer);
}

void updateTone() {
  if (tone1 == 0 && tone2 == 0) return;
  unsigned long now = millis();
  if (tonePhase && now - patternStart >= dur1) {
    patternStart = now;
    tonePhase = false;
    if (tone2 > 0) tone(buzzer, tone2);
    else noTone(buzzer);
  }
  else if (!tonePhase && now - patternStart >= dur2) {
    patternStart = now;
    tonePhase = true;
    if (tone1 > 0) tone(buzzer, tone1);
    else noTone(buzzer);
  }
}

// === LCD TAMPIKAN NILAI DAN STATUS ===
void showLCD(long distance, float suhu, int cahaya) {
  unsigned long now = millis();
  unsigned long interval = (screenMode == 0) ? intervalJarak :
                           (screenMode == 1) ? intervalSuhu :
                           intervalCahaya;

  if (now - previousMillis >= interval) {
    previousMillis = now;
    screenMode = (screenMode + 1) % 3;
    lcd.clear();

    if (screenMode == 0) {
      lcd.setCursor(0, 0);
      lcd.print("Jarak: ");
      if (distance == 999) lcd.print("Err");
      else lcd.print(distance);
      lcd.print("cm");

      lcd.setCursor(0, 1);
      if (distance <= 50) lcd.print("PENUH!! BAHAYA!!");
      else if (distance <= 100) lcd.print("Tinggi - 100%");
      else if (distance <= 150) lcd.print("Tinggi - 80%");
      else if (distance <= 200) lcd.print("Sedang - 60%");
      else if (distance <= 280) lcd.print("Rendah - 40%");
      else lcd.print("Rendah - 20%");
    } 
    else if (screenMode == 1) {
      lcd.setCursor(0, 0);
      lcd.print("Suhu: ");
      lcd.print(suhu, 1);
      lcd.print(" C");
    } 
    else if (screenMode == 2) {
      int persenCahaya = map(cahaya, 0, 1023, 0, 100);
      lcd.setCursor(0, 0);
      lcd.print("Cahaya: ");
      lcd.print(persenCahaya);
      lcd.print("%");

      lcd.setCursor(0, 1);
      if (persenCahaya > 50) lcd.print("(Terang)");
      else lcd.print("(Gelap)");
    }
  }
}
