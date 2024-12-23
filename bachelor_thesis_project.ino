//libraries
#include <HX711_ADC.h>
#if defined(ESP8266)|| defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif

#include <Stepper.h>

#include <LiquidCrystal.h>

//pins for Hx711:
const int HX711_dout = 5; //mcu > HX711 dout pin
const int HX711_sck = 6; //mcu > HX711 sck pin

//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);

//values for stepper motor
int StepsPerRev = 2048;
int MotSpeed = 10;
int steps_required = 171;
int stop_pos = 0;
int SwitchState1;
int SwitchState2;
int SwitchState3;
int SwitchState4;
int SwitchState5;
int SwitchState6;
int SwitchState7;

//Stepper constructor
Stepper Stepper1(StepsPerRev, 22,9,8,10);

//load cell values
const int calVal_eepromAdress = 0;
unsigned long t = 0;

//values read from serial communication
String read_val;
String read_val2;
float read_val_mass;
int read_val_mass_int;
int read_val_mass_int_val;

//values for container
String pretinac1;
String pretinac2;
String pretinac1_kraj = "a";
String pretinac2_kraj = "b";
float pretinac1_int;
float pretinac2_int;
int Start = 0;
int Stop = 0;
int Continue = 0;

//pins for LCD
int RS = 44;
int EN = 4;
int DB4 = 7;
int DB5 = 40;
int DB6 = 3;
int DB7 = 42;

//LCD constructor
LiquidCrystal LCD (RS, EN, DB4, DB5, DB6, DB7);

//counters
int count_container = 1;
int count_nextStep = 1;
int count_delay = 1;


//void setup()
void setup() {
  
  Serial.begin(9600); 
  Serial1.begin(9600);
  Serial2.begin(9600);
  
  LCD.begin (16,2);

  pinMode(2, INPUT_PULLUP);
  pinMode(48, INPUT_PULLUP);
  pinMode(50, INPUT_PULLUP);
  pinMode(52, INPUT_PULLUP);

  Stepper1.setSpeed(MotSpeed);

  attachInterrupt(digitalPinToInterrupt(2), EmergencyStop, LOW);

  LoadCell.begin();
  float calibrationValue; 
  calibrationValue = -451.01; 
#if defined(ESP8266)|| defined(ESP32)
  EEPROM.begin(512); 
#endif
  EEPROM.get(calVal_eepromAdress, calibrationValue); 

  unsigned long stabilizingtime = 2000; 
  boolean _tare = true; 
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  }
  else {
    LoadCell.setCalFactor(calibrationValue); 
    Serial.println("Startup is complete");
  }
}

void loop() {
  
  static boolean newDataReady = 0;
  const int serialPrintInterval = 1000; 
  const int serial1PrintInterval = 1000;

  if (LoadCell.update()) newDataReady = true;

  if (newDataReady) {
    if (millis() > t + serialPrintInterval) {
      read_val_mass = LoadCell.getData();
      read_val_mass_int = round(read_val_mass);
      Serial.print("Load_cell output val: ");
      Serial.println(read_val_mass_int);
      newDataReady = 0;
      t = millis();
    }
  }

  if (Serial2.available() > 0) {
    char inByte = Serial2.read();
    if (inByte == 't') LoadCell.tareNoDelay();
  }

  if (LoadCell.getTareStatus() == true) {
    Serial.println("Tare complete");
  }


  LCD.setCursor(0,0);
  LCD.print(read_val_mass_int);
  LCD.setCursor(0,2);
  LCD.print("[gram]");
  //delay(500);
  LCD.clear();
  
  SwitchState1 = digitalRead(48);
  SwitchState2 = digitalRead(50);
  SwitchState3 = digitalRead(52);
  
  if (Serial1.available() > 0) {  
    read_val = Serial1.readString();
    //Serial.println(read_val);

    if (read_val.endsWith(pretinac1_kraj)) { // ako procitana varijabla zavrsava na "a", znaci da se radi o pretincu 1
      pretinac1 = read_val; // dodijelimo ocitanu vrijednost varijabli pretinac1
      pretinac1.remove(pretinac1.length()-1); //micemo slovo "a"
      pretinac1_int = pretinac1.toInt();
      Serial.println(pretinac1_int);
    }

    if (read_val.endsWith(pretinac2_kraj)) { // ako procitana varijabla zavrsava na "b", znaci da se radi o pretincu 2
      pretinac2 = read_val; // dodijelimo ocitanu vrijednost varijabli pretinac2
      pretinac2.remove(pretinac2.length()-1); //micemo slovo "b"
      pretinac2_int = pretinac2.toInt();
      Serial.println(pretinac2_int);
    }

    if (read_val == "start") { // ako kliknemo na start
      Start = 1;
      Serial.println("Start");
    }

    if (read_val == "stop") { // ako kliknemo na stop
      Stop = 1;
      Serial.println("Stop");
    }

    if (read_val == "continue") { // ako kliknemo na continue
      Continue = 1;
      Start = 1;
      Serial.println("Start");
    }
  }
  
  if (Start == 1) {
   if (count_container == 1) {
    if (count_delay == 1) {
      delay(5000);
      count_delay = 0;
    }
    if (count_nextStep == 1) {
      MyStepper1.step(steps_required);
      if (SwitchState1 == LOW && SwitchState2 == HIGH) {
        MyStepper1.step(stop_pos);
        count_nextStep++;
      }
    }
      
   if (read_val_mass_int >= pretinac1_int && read_val_mass_int != 0 && count_nextStep == 2) {
    MyStepper1.step(-steps_required);
    if (millis() > t + serial1PrintInterval) {
      Serial.println("Dostignuta je masa.");
      t = millis();
   }
   if (SwitchState2 == LOW && SwitchState1 == HIGH) {
    MyStepper1.step(stop_pos);
    count_container = 2;
    count_nextStep = 1;
    count_delay = 1;
   }
  }
 }

   if (count_container == 2) {
    if (count_delay == 1) {
      delay(2000);
      count_delay = 0;
    }
    if (count_nextStep == 1) {
      MyStepper1.step(-steps_required);
      if (SwitchState3 == LOW && SwitchState2 == HIGH) {
        MyStepper1.step(stop_pos);
        count_nextStep++;
      }
    }

    if (read_val_mass_int >= (pretinac2_int + pretinac1_int) && read_val_mass_int != 0 && count_nextStep == 2) {
      MyStepper1.step(steps_required);
      if (millis() > t + serial1PrintInterval) {
        Serial.println("Dostignuta je masa.");
        t = millis();
    }
    if (SwitchState2 == LOW && SwitchState3 == HIGH) {
      MyStepper1.step(stop_pos);
      if (Stop == 0) {
        count_container = 1;
        count_nextStep = 1;
        count_delay = 1;
      }
      if (Stop == 1) {
        Start = 0;
      }
      if (Continue == 1) {
        Start = 1;
        Stop = 0;
      }
    }
   }
  }
 }  
}

void EmergencyStop() {
  if (SwitchState4 == LOW) {
    Start = 0;
  }
}
