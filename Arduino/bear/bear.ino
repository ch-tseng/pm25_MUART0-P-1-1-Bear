#include <SoftwareSerial.h>
SoftwareSerial Serial1(2, 3); // RX, TX
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
//LiquidCrystal_I2C lcd(0x27, 16, 2);
LiquidCrystal_I2C lcd(0x3f, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

int pm25_onoffThreshold = 20;
int accuThresholdCount = 3;  //重複超過幾次才啟動
boolean disablePIR = 1; //PIR是否作用? 若無作用則LCD的背光持續開啟

byte pinLEDred = 12;
byte pinBlue = 10;
byte pinGreen = 11;
byte pinRelay = 9;
byte pinPIR = 8;
byte pinBtnUp = 7;
byte pinBtnDown = 6;
byte pinRelayStatus = 5;  //接到IO port OUT#1, 接收來自relay的status
//byte pinIOreceive_1 = 4;
//byte pinIOsend_1 = 9;

long pmcf10 = 0;
long pmcf25 = 0;
long pmcf100 = 0;
long pmat10 = 0;
long pmat25 = 0;
long pmat100 = 0;
unsigned int temperature = 0;
unsigned int humandity = 0;

char buf[50];
boolean deviceOnOffStatus = 0;
boolean pm25Over = 0;
boolean last_pm25Over = 0;
String displayOnOff;
int countOver = 0;  //累計for accuThresholdCount

void displayLCD(int yPosition, String txtMSG) {
  int xPos = (16 - txtMSG.length()) / 2;
  if (xPos < 0) xPos = 0;
  lcd.setCursor(xPos, yPosition);
  lcd.print(txtMSG);
}

void onoffDevice(boolean command) {
  deviceOnOffStatus = command;
  if (command == 0) {
    digitalWrite(pinRelay, LOW);
  } else {
    digitalWrite(pinRelay, HIGH);
  }
  delay(300);
}

void adjustThreshold(boolean upStatus, boolean downStatus) {
  if(upStatus==1 && downStatus==1) {
    lcd.backlight();
    if(disablePIR==0) {
      disablePIR = 1;
      displayLCD(0, "PIR Configuration");
      displayLCD(1, "  PIR disabled  ");

    }else{
      disablePIR = 0;
      displayLCD(0, "[Configuration] ");
      displayLCD(1, "  PIR enabled   ");

    }
    delay(600);
    //if(disablePIR==0) lcd.noBacklight();
  }

  if(upStatus==1 && downStatus==0) {  
    lcd.backlight();
    pm25_onoffThreshold -= 10;
    if(pm25_onoffThreshold<0) pm25_onoffThreshold=300;  
    displayLCD(0, "PM2.5 threshold ");
    displayLCD(1, "                ");
    displayLCD(1, String(pm25_onoffThreshold));
    delay(600);
    //if(disablePIR==0) lcd.noBacklight();
  }

  if(upStatus==0 && downStatus==1) {  
    lcd.backlight();
    pm25_onoffThreshold += 10;
    if(pm25_onoffThreshold>300) pm25_onoffThreshold=0;  
    displayLCD(0, "PM2.5 threshold ");
    displayLCD(1, "                ");
    displayLCD(1, String(pm25_onoffThreshold));
    delay(600);
    //if(disablePIR==0) lcd.noBacklight();
  }
}

void setup() {
  pinMode(pinLEDred, OUTPUT);
  pinMode(pinBlue, OUTPUT);
  pinMode(pinGreen, OUTPUT);
  pinMode(pinPIR, INPUT);
  pinMode(pinRelay, OUTPUT);
  pinMode(pinRelayStatus, INPUT);
  digitalWrite(pinRelay, HIGH);
  pinMode(pinBtnUp, INPUT);
  pinMode(pinBtnDown, INPUT);

  digitalWrite(pinLEDred, LOW);
  digitalWrite(pinBlue, LOW);
  digitalWrite(pinGreen, LOW);
  digitalWrite(pinLEDred, HIGH);

  Serial.begin(9600);
  Serial1.begin(9600);
  Serial.println("Setup");
  //lcd.init();
  lcd.begin(16, 2);
  //lcd.backlight();

  onoffDevice(0);
}

void loop() { // run over and over
  digitalWrite(pinLEDred, HIGH);
  digitalWrite(pinBlue, HIGH);
  digitalWrite(pinGreen, HIGH);

  boolean pirStatus = digitalRead(pinPIR);
  boolean btnUp_status = digitalRead(pinBtnUp);
  boolean btnDown_status = digitalRead(pinBtnDown);
  boolean relay_status = digitalRead(pinRelayStatus);

  if(btnUp_status==1 or btnDown_status==1) {
    adjustThreshold(btnUp_status, btnDown_status);
    long timeStart = millis();
    while ((millis()-timeStart) <5000) {
      btnUp_status = digitalRead(pinBtnUp);
      btnDown_status = digitalRead(pinBtnDown);
        if(btnUp_status==1 or btnDown_status==1) {
          timeStart = millis();
          adjustThreshold(btnUp_status, btnDown_status);
        }
    }
  }

  // put your main code here, to run repeatedly:
  int count = 0;
  unsigned char c;
  unsigned char high;

  Serial.println("read air data.");
  delay(500);

  while (Serial1.available()) {
    digitalWrite(pinLEDred, HIGH);
    digitalWrite(pinBlue, LOW);
    digitalWrite(pinGreen, LOW);

    c = Serial1.read();
    if ((count == 0 && c != 0x42) || (count == 1 && c != 0x4d)) {
      Serial.println("check failed");
      break;
    }
    if (count > 27) {
      Serial.println("complete");
      break;
    }
    else if ( count == 6 || count == 24|| count == 26) {
      high = c;
    }
    else if (count == 7) {
      pmcf25 = 256 * high + c;
      //Serial.print("CF=1, PM2.5=");
      //Serial.print(pmcf25);
      //Serial.println(" ug/m3");
    }
    else if (count == 25) {
      temperature = (256 * high + c)/10 ;
      Serial.print("temperature=");
      Serial.print(temperature);
      Serial.println(" C");
    }
    else if (count == 27) {
      humandity = (256 * high + c)/10 ;
      Serial.print("humandity=");
      Serial.print(humandity);
      Serial.println(" %");
    }
    count++;
  }
  //Serial.print("PM25="); Serial.println(pmcf25);
  last_pm25Over = pm25Over;
  if (pmcf25 > pm25_onoffThreshold) {
    Serial.println("TEST: PIR:" + String(pirStatus) + " deviceOnOffStatus=" + String(deviceOnOffStatus) + " IO1_pin=" + String(digitalRead(pinRelay)) + " pmcf25=" + String(pmcf25) + " pm25_onoffThreshold=" + String(pm25_onoffThreshold) + " btnUP:" + String(btnUp_status) + " btnDown:" + String(btnDown_status));
    pm25Over = 1;
  } else {
    pm25Over = 0;
  }

  if (last_pm25Over == pm25Over) {
    countOver += 1;
  } else {
    countOver = 0;
  }

  if (countOver > accuThresholdCount) {
    Serial.println("pmcf25=" + String(pmcf25) + " pm25_onoffThreshold=" + String(pm25_onoffThreshold) + " / pm25Ov=" + String(pmcf25) + " ,pm25Over=" + String(pm25Over));
    if (pm25Over == 1) {
      
      if (deviceOnOffStatus == 0) {
        onoffDevice(1);
        Serial.println("deviceOnOffStatus is  Off (0) now, run onoffDevice(1) ");
      }
    } else {
      if (deviceOnOffStatus == 1){
        onoffDevice(0);
        Serial.println("deviceOnOffStatus is  On (1) now, run onoffDevice(0) ");
      }
    }
    countOver = 0;
  }

  if (deviceOnOffStatus == relay_status) {  
    if(deviceOnOffStatus==0) {
      displayOnOff = "Off";
    } else {
      displayOnOff = "On";
    }
  }else{
    displayOnOff = "Err";
    //Try to fix the error
    onoffDevice(deviceOnOffStatus);
  }
  Serial.println("relay_status="+String(relay_status) + " / deviceOnOffStatus=" + String(deviceOnOffStatus));

  //Serial.println("countOver=" + String( countOver) + " pm25Over=" + String(pm25Over));

  if(disablePIR==0) {
    if (pirStatus == 1) {
      lcd.backlight();
    } else {
      lcd.noBacklight();
    }
  }
  //displayLCD(0, "                ");
  Serial.println("[" + String(pm25_onoffThreshold) + ":" + displayOnOff + "] " + String(temperature) +  "C " + String(humandity) + "%");
  displayLCD(0, " [" + String(pm25_onoffThreshold) + ":" + displayOnOff + "]" + String(temperature) +  "C " + String(humandity) + "% ");
  //displayLCD(1, "                ");
  displayLCD(1, " PM2.5:" + String(pmcf25) + " ug/m3 ");


  while (Serial1.available()) Serial1.read();
  Serial.println();
  digitalWrite(pinLEDred, HIGH);
  digitalWrite(pinBlue, HIGH);
  digitalWrite(pinGreen, HIGH);
  delay(500);
}


