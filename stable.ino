#include <SPI.h>
#include <DHT.h>
#include <TFT.h>
#include <GyverPower.h>
#include <RotaryEncoder.h>

#define DHTPIN 5 // пин DHT-датчика

#define CS 10 //пины дисплея
#define DC 9
#define RST 8

#define ENCSW 4 //пины энкодера
#define ENCDT 7
#define ENCCLK 12

#define INTPIN 2 //пин для прерывания

#define BLPIN 6 //пин подсветки дисплея
#define SCUP 10000 //частота обновления экрана в главном меню, в мс
#define AUSL 60000 //время бездействия до перехода в режим сна

#define POSMIN 0 //меню выбора настройки
#define POSMAX 2

#define POSMIN_2 0 //меню регулировки яркости
#define POSMAX_2 5

#define POSMIN_3 0 //меню выбора единицы измерения
#define POSMAX_3 2

DHT dht(DHTPIN, DHT11);
TFT TFTscreen = TFT(CS, DC, RST);
RotaryEncoder encoder(ENCDT, ENCCLK);

char sensorTPrintout[3];
char sensorHPrintout[3];
unsigned long scrUpd = 0; //отсчет времени обновления экрана
unsigned long autoSleep = 0; //отсчет времени бездействия до перехода в режим сна
volatile boolean flagForWake =  false, flagForSleep = true; //флаги прерывания
static volatile byte w = 5; //переменная для идентификации номера меню
byte lastPos = 1, newPos = 1; // значение энкодера
static byte brtl = 5; // уровень яркости экрана
static boolean unit = 0; //ед. изм., 0 - Цельсий, 1 - Фаренгейт
static int BRT = 168; // яркость экрана, в у.е. (мин. 0, макс. 168)

void setup()
{
  dht.begin();
  TFTscreen.begin();
  pinMode(INTPIN, INPUT_PULLUP);
  pinMode(BLPIN, OUTPUT);
  pinMode(ENCSW, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(INTPIN), sleepControl, FALLING);
  sleepMode();
}
void loop()
{
  if (millis() - autoSleep > AUSL)
  {
    flagForWake = false;
    flagForSleep = true;
  }
  if (flagForSleep)
  {
    delay(50);
    sleepMode();
  }
  if (flagForWake)
  {
    delay(50);
    analogWrite(BLPIN, BRT);
    wakeMode();
  }
}
void sleepControl()
{
  flagForWake = !flagForWake;
  flagForSleep = !flagForSleep;
  autoSleep = millis();
}
void sleepMode()
{
  digitalWrite(BLPIN, LOW);
  power.setSleepMode(POWERDOWN_SLEEP);
  power.sleep(SLEEP_FOREVER);
}
void wakeMode()
{
  if (w == 5)
  {
    analogWrite(BLPIN, BRT);
    outputData();
    w = 0;
  }
  if (w == 0)
  {
    mainScreen();
  }
  if (w == 1)
  {
    menuOne();
  }
  if (w == 2)
  {
    menuTwo();
  }
  if (w == 3)
  {
    menuThree();
  }
}
void outputData()
{
  TFTscreen.background(0, 0, 0);
  float temperature = dht.readTemperature();
  if (temperature < 17)
    TFTscreen.stroke(112, 188, 255);
  else if (temperature > 17 && temperature < 30)
    TFTscreen.stroke(0, 128, 0);
  else if (temperature > 30 && temperature < 40)
    TFTscreen.stroke(255, 216, 31);
  else if (temperature > 40)
    TFTscreen.stroke(250, 48, 22);
  if (!unit)
  {
    String sensorTVal = String(temperature);
    sensorTVal.toCharArray(sensorTPrintout, 3);
    TFTscreen.setTextSize(5);
    TFTscreen.text(sensorTPrintout, 65, 43);
    TFTscreen.circle(128, 44, 2);
    TFTscreen.setTextSize(2);
    TFTscreen.text(" C", 120, 63);
  }
  else if (unit)
  {
    temperature = (temperature * 9.0 / 5.0) + 32;
    String sensorTVal = String(temperature);
    sensorTVal.toCharArray(sensorTPrintout, 3);
    TFTscreen.setTextSize(5);
    TFTscreen.text(sensorTPrintout, 65, 43);
    TFTscreen.circle(128, 44, 2);
    TFTscreen.setTextSize(2);
    TFTscreen.text(" F", 120, 63);
  }
  float humidity = dht.readHumidity();
  TFTscreen.stroke(112, 188, 255);
  String sensorHVal = String(humidity);
  sensorHVal.toCharArray(sensorHPrintout, 3);
  TFTscreen.text(sensorHPrintout, 100, 90);
  TFTscreen.text(" %", 115, 90);
  TFTscreen.stroke(38, 114, 51);
  TFTscreen.rect(38, 15, 122, 102);
  TFTscreen.setTextSize(1);
  TFTscreen.text("SETTINGS", 38, 4);
  TFTscreen.text("MeteoStation X 1.0", 43, 120);
}
void mainScreen()
{
  if (millis() - scrUpd > SCUP)
  {
    outputData();
    scrUpd = millis();
  }
  if (!digitalRead(ENCSW))
  {
    TFTscreen.fill(38, 114, 51);
    TFTscreen.rect(38, 4, 47, 9);
    TFTscreen.noFill();
    TFTscreen.stroke(0, 0, 0);
    TFTscreen.text("SETTINGS", 38, 4);
    delay(400);
    TFTscreen.background(0, 0, 0);
    w = 1;
    encoder.setPosition(1);
    lastPos, newPos = 1;
    autoSleep = millis();
  }
}
void menuOne()
{
  encoder.tick();
  TFTscreen.stroke(38, 114, 51);
  TFTscreen.setTextSize(1);
  TFTscreen.text("BACK", 38, 4);
  TFTscreen.stroke(38, 114, 51);
  TFTscreen.rect(38, 15, 122, 102);
  TFTscreen.stroke(0, 128, 0);
  TFTscreen.setTextSize(2);
  TFTscreen.text("BRT", 70, 45);
  TFTscreen.text("UNIT", 70, 75);
  newPos = encoder.getPosition();
  if (newPos < POSMIN)
  {
    encoder.setPosition(POSMIN);
    newPos = POSMIN;
  }
  else if (newPos > POSMAX)
  {
    encoder.setPosition(POSMAX);
    newPos = POSMAX;
  }
  if (lastPos != newPos)
  {
    lastPos = newPos;
    TFTscreen.background(0, 0, 0);
    autoSleep = millis();
  }
  encoder.tick();
  switch (newPos)
  {
    case 0:
      TFTscreen.stroke(38, 114, 51);
      TFTscreen.rect(38, 4, 25, 9);
      break;
    case 1:
      TFTscreen.stroke(38, 114, 51);
      TFTscreen.circle(62, 50, 4);
      break;
    case 2:
      TFTscreen.stroke(38, 114, 51);
      TFTscreen.circle(62, 80, 4);
      break;
  }
  if (newPos == 0 && !digitalRead(ENCSW))
  {
    TFTscreen.fill(38, 114, 51);
    TFTscreen.rect(38, 4, 25, 9);
    TFTscreen.noFill();
    TFTscreen.setTextSize(1);
    TFTscreen.stroke(0, 0, 0);
    TFTscreen.text("BACK", 38, 4);
    w = 0;
    autoSleep = millis();
    delay (400);
  }
  if (newPos == 1 && !digitalRead(ENCSW))
  {
    TFTscreen.fill(38, 114, 51);
    TFTscreen.circle(62, 50, 4);
    TFTscreen.noFill();
    w = 2;
    encoder.setPosition(brtl);
    autoSleep = millis();
    delay(400);
  }
  if (newPos == 2 && !digitalRead(ENCSW))
  {
    TFTscreen.fill(38, 114, 51);
    TFTscreen.circle(62, 80, 4);
    TFTscreen.noFill();
    w = 3;
    encoder.setPosition(1);
    autoSleep = millis();
    delay(400);
  }
}
void menuTwo()
{
  encoder.tick();
  TFTscreen.stroke(38, 114, 51);
  TFTscreen.rect(38, 15, 122, 102);
  TFTscreen.setTextSize(1);
  TFTscreen.text("BACK", 38, 4);
  TFTscreen.stroke(0, 128, 0);
  TFTscreen.setTextSize(2);
  TFTscreen.text("BRT:", 80, 40);
  TFTscreen.stroke(0, 128, 0);
  TFTscreen.rect(55, 70, 90, 15);
  newPos = encoder.getPosition();
  if (newPos < POSMIN_2)
  {
    encoder.setPosition(POSMIN_2);
    newPos = POSMIN_2;
  }
  else if (newPos > POSMAX_2)
  {
    encoder.setPosition(POSMAX_2);
    newPos = POSMAX_2;
  }
  if (lastPos != newPos)
  {
    lastPos = newPos;
    TFTscreen.background(0, 0, 0);
    autoSleep = millis();
  }
  encoder.tick();
  switch (newPos)
  {
    case 0:
      TFTscreen.fill(0, 128, 0);
      TFTscreen.rect(55, 70, 5, 15);
      TFTscreen.noFill();
      BRT = 10;
      brtl = 0;
      analogWrite(BLPIN, BRT);
      break;
    case 1:
      TFTscreen.fill(0, 128, 0);
      TFTscreen.rect(55, 70, 18, 15);
      TFTscreen.noFill();
      BRT = 33;
      brtl = 1;
      analogWrite(BLPIN, BRT);
      break;
    case 2:
      TFTscreen.fill(0, 128, 0);
      TFTscreen.rect(55, 70, 36, 15);
      TFTscreen.noFill();
      BRT = 66;
      brtl = 2;
      analogWrite(BLPIN, BRT);
      break;
    case 3:
      TFTscreen.fill(0, 128, 0);
      TFTscreen.rect(55, 70, 54, 15);
      TFTscreen.noFill();
      BRT = 99;
      brtl = 3;
      analogWrite(BLPIN, BRT);
      break;
    case 4:
      TFTscreen.fill(0, 128, 0);
      TFTscreen.rect(55, 70, 72, 15);
      TFTscreen.noFill();
      BRT = 132;
      brtl = 4;
      analogWrite(BLPIN, BRT);
      break;
    case 5:
      TFTscreen.fill(0, 128, 0);
      TFTscreen.rect(55, 70, 90, 15);
      TFTscreen.noFill();
      BRT = 168;
      brtl = 5;
      analogWrite(BLPIN, BRT);
      break;
  }
  if (!digitalRead(ENCSW))
  {
    TFTscreen.fill(38, 114, 51);
    TFTscreen.rect(38, 4, 25, 9);
    TFTscreen.noFill();
    TFTscreen.setTextSize(1);
    TFTscreen.stroke(0, 0, 0);
    TFTscreen.text("BACK", 38, 4);
    w = 1;
    encoder.setPosition(1);
    lastPos, newPos = 1;
    autoSleep = millis();
    delay (400);
  }
}
void menuThree()
{
  encoder.tick();
  TFTscreen.stroke(38, 114, 51);
  TFTscreen.rect(38, 15, 122, 102);
  TFTscreen.setTextSize(1);
  TFTscreen.text("BACK", 38, 4);
  TFTscreen.stroke(0, 128, 0);
  TFTscreen.setTextSize(2);
  TFTscreen.text("C", 80, 45);
  TFTscreen.circle(75, 45, 2);
  TFTscreen.text("F", 80, 75);
  TFTscreen.circle(75, 75, 2);
  newPos = encoder.getPosition();
  if (newPos < POSMIN_3)
  {
    encoder.setPosition(POSMIN_3);
    newPos = POSMIN_3;
  }
  else if (newPos > POSMAX_3)
  {
    encoder.setPosition(POSMAX_3);
    newPos = POSMAX_3;
  }
  if (lastPos != newPos)
  {
    lastPos = newPos;
    TFTscreen.background(0, 0, 0);
    autoSleep = millis();
  }
  encoder.tick();
  switch (newPos)
  {
    case 0:
      TFTscreen.stroke(38, 114, 51);
      TFTscreen.rect(38, 4, 25, 9);
      break;
    case 1:
      TFTscreen.stroke(38, 114, 51);
      TFTscreen.circle(62, 50, 4);
      break;
    case 2:
      TFTscreen.stroke(38, 114, 51);
      TFTscreen.circle(62, 80, 4);
      break;
  }
  if (newPos == 0 && !digitalRead(ENCSW))
  {
    TFTscreen.fill(38, 114, 51);
    TFTscreen.rect(38, 4, 25, 9);
    TFTscreen.noFill();
    TFTscreen.setTextSize(1);
    TFTscreen.stroke(0, 0, 0);
    TFTscreen.text("BACK", 38, 4);
    w = 1;
    encoder.setPosition(1);
    lastPos, newPos = 1;
    autoSleep = millis();
    delay (400);
  }
  if (newPos == 1 && !digitalRead(ENCSW))
  {
    TFTscreen.fill(38, 114, 51);
    TFTscreen.circle(62, 50, 4);
    TFTscreen.noFill();
    TFTscreen.fill(38, 114, 51);
    TFTscreen.rect(38, 4, 25, 9);
    TFTscreen.noFill();
    TFTscreen.setTextSize(1);
    TFTscreen.stroke(0, 0, 0);
    TFTscreen.text("BACK", 38, 4);
    unit = 0;
    w = 0;
    autoSleep = millis();
    delay(400);
  }
  if (newPos == 2 && !digitalRead(ENCSW))
  {
    TFTscreen.fill(38, 114, 51);
    TFTscreen.circle(62, 80, 4);
    TFTscreen.noFill();
    TFTscreen.fill(38, 114, 51);
    TFTscreen.rect(38, 4, 25, 9);
    TFTscreen.noFill();
    TFTscreen.setTextSize(1);
    TFTscreen.stroke(0, 0, 0);
    TFTscreen.text("BACK", 38, 4);
    unit = 1;
    w = 0;
    autoSleep = millis();
    delay(400);
  }
}
