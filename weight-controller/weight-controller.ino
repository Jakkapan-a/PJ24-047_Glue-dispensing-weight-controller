/*
Last update: 2025-10-29
Author: Jakkapan A.
Github: https://github.com/Jakkapan-a/PJ24-047_Glue-dispensing-weight-controller
Hardware: Arduino Mega 2560
Sensor: HX711
LCD: I2C 16x2
Relay: 2 Channel
Buzzer: Active
Button: 4 Button

- 4 Button
  - ESC
  - UP
  - DOWN
  - OK

- 2 Input
  - SENSOR
  - M/C RUN

- 3 Output
  - RELAY 1
  - RELAY 2
  - BUZZER

- 3 LED
  - RED
  - GREEN
  - BLUE

- 1 HX711
  - DATA
  - CLOCK
*/ 

#include <TcBUTTON.h>
#include <TcPINOUT.h>
#include "HX711.h"
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <Wire.h>

#define DEBUG 0
LiquidCrystal_I2C lcd(0x27, 16, 2);
HX711 scale;
const uint8_t dataPin = 6;
const uint8_t clockPin = 7;

#define BUTTON_PIN_ESC A0
#define BUTTON_PIN_UP A1
#define BUTTON_PIN_DOWN A2
#define BUTTON_PIN_OK A3

void btnEscOnEventChanged(bool state);
void btnUpOnEventChanged(bool state);
void btnDownOnEventChanged(bool state);
void btnOkOnEventChanged(bool state);

void btnUpHoldCallback();
void btnDownHoldCallback();
TcBUTTON btnEsc(BUTTON_PIN_ESC, btnEscOnEventChanged);
TcBUTTON btnUp(BUTTON_PIN_UP, btnUpOnEventChanged);
TcBUTTON btnDown(BUTTON_PIN_DOWN, btnDownOnEventChanged);
TcBUTTON btnOk(BUTTON_PIN_OK, btnOkOnEventChanged);

#define INPUT_PIN1 8  // SENSOR
#define INPUT_PIN2 9
#define INPUT_PIN3 10
#define INPUT_PIN4 11

void inputPin1OnEventChanged(bool state);
void inputPin2OnEventChanged(bool state);

TcBUTTON inputPin1(INPUT_PIN1, inputPin1OnEventChanged);
TcBUTTON inputPin2(INPUT_PIN2, inputPin2OnEventChanged);

// -------------- OUTPUT --------------

#define BUZZER_PIN 10
TcPINOUT buzzer(BUZZER_PIN);

#define RELAY_AIR1 4
#define RELAY_AIR2 5

TcPINOUT relayAir1(RELAY_AIR1);
TcPINOUT relayAir2(RELAY_AIR2);


#define LED_RED_PIN 13
#define LED_GREEN_PIN 12
#define LED_BLUE_PIN 11

enum LED_COLOR {
  RED,
  GREEN,
  BLUE,
  OFF
};

void setLedColor(LED_COLOR color) {
  switch (color) {
    case RED:
      digitalWrite(LED_RED_PIN, HIGH);
      digitalWrite(LED_GREEN_PIN, LOW);
      digitalWrite(LED_BLUE_PIN, LOW);
      break;
    case GREEN:
      digitalWrite(LED_RED_PIN, LOW);
      digitalWrite(LED_GREEN_PIN, HIGH);
      digitalWrite(LED_BLUE_PIN, LOW);
      break;
    case BLUE:
      digitalWrite(LED_RED_PIN, LOW);
      digitalWrite(LED_GREEN_PIN, LOW);
      digitalWrite(LED_BLUE_PIN, HIGH);
      break;
    default:
      digitalWrite(LED_RED_PIN, LOW);
      digitalWrite(LED_GREEN_PIN, LOW);
      digitalWrite(LED_BLUE_PIN, LOW);
      break;
  }
}

// -------------- HX711 --------------
float w1 = 0, w2 = 0, previous = 0;
const unsigned long interval = 100;  //
unsigned long lastReadTime = 0;
bool waitingForStability = false;  //

// -------------- EEPROM --------------
template<typename T>
void SaveToEEPROM(int address, T value) {
  EEPROM.put(address, value);
}

template<typename T>
T ReadFromEEPROM(int address) {
  T value;
  EEPROM.get(address, value);
  return value;
}
int menuIndex = 0;
int subMenuIndex = 0;
float values[5] = { 0.0, 0.0, 0.0, 0.0, 0.0 };      // 0=min1, 1=max1, 2=min2, 3=max2, 4=cal weight
float valuesTemp[5] = { 0.0, 0.0, 0.0, 0.0, 0.0 };  // min1, max1, min2, max2, cal weight
// ---------- SETUP ------------
void setup() {
  Serial.begin(9600);
  scale.begin(dataPin, clockPin);
  float scale_factor = ReadFromEEPROM<float>(50);
  Serial.print("Scale factor: ");
  Serial.println(scale_factor);
  if (scale_factor != 0) {
    scale.set_scale(scale_factor);
  } else {
    scale.set_scale(406.472167);
  }
  scale.tare();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  // -- Read EEPROM to values
  for (int i = 0; i < 5; i++) {
    values[i] = ReadFromEEPROM<float>(i * sizeof(float));
    // Check if value is NaN and set to default 0
    if (isnan(values[i])) {
      values[i] = 0.0;
    }
  }
  // ----------------
  delay(100);
  String _w = String(w1, 1) + "g";
  _w = "Weight: " + _w;
  updateLCD("AUTO",_w.c_str());
  // -- LED --
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_BLUE_PIN, OUTPUT);
#if 0  // Test LED
  setLedColor(RED);
  delay(1000);
  setLedColor(GREEN);
  delay(1000);
  setLedColor(BLUE);
  delay(1000);
  setLedColor(OFF);
#endif
  String dot = ".";
  updateLCD("Loading", "");
  for (int i = 0; i < 10; i++) {
    updateLCD("Loading", dot.c_str());
    dot += ".";
    delay(50);
  }
  delay(300);
  // -- BUTTONS --
  btnUp.setOnHold(btnUpHoldCallback, 1000);
  btnDown.setOnHold(btnDownHoldCallback, 1000);
  inputPin1.isInvert = false;
  inputPin2.isInvert = true;
  // set debounce delay to 1000 milliseconds (1 second)
  inputPin1.setDebounceDelay(1000);
  // -- RELAY --  //
  relayAir1.off();
  relayAir2.off();
  // -- BUZZER -- //
  buzzer.toggleFor(2);
}
int menu = 0;
bool btnEscPressed = false;
bool btnUpPressed = false;
bool btnDownPressed = false;
bool btnOkPressed = false;
bool btnUpHold = false;
bool btnDownHold = false;
bool input1Sensor = false;
bool input1SensorState = false;
bool input2McRun = false;
bool input2McRunState = false;
bool resultNG = false;
const int _countDownStart = 3;
int countDownStart = 0;

const int _countDownStop = 5; // 5 seconds countdown
int countDownStop = 0;
void loop() {
  btnEsc.update();
  inputPin1.update();
  inputPin2.update();
  buzzer.update();
  // relayAir1.update();
  relayAir2.update();
  if(countDownStart > 0)
  {
    //
    updateDispalyCountDown();
    return;
  }
  
  if (menu == 0) {
    updateWeight();
  } else {
    btnUp.update();
    btnDown.update();
    btnOk.update();
    updateMenu();
  }
}
const char *menuItems[] = { "Zero", "Min1", "NAN", "Min2", "Max2", "Cal" };
void updateMenu() {
  String _men1 = menuItems[menuIndex];
  _men1 = ">" + _men1;
  String _men2 = menuIndex >= 5 ? " " : menuItems[menuIndex + 1];
  _men2 = " " + _men2;
  updateLCD(_men1.c_str(), _men2.c_str());

  if (btnUpPressed) {
    menuIndex = (menuIndex > 0) ? menuIndex - 1 : 5;
    btnUpPressed = false;
  }
  if (btnDownPressed) {
    menuIndex = (menuIndex < 5) ? menuIndex + 1 : 0;
    btnDownPressed = false;
  }
  if (btnOkPressed) {
    if (menuIndex == 0) {
      scale.tare();
      updateLCD("Set zero", ".");
      String _dot = ".";
      for (int i = 0; i < 5; i++) {
        updateLCD("Set zero", _dot.c_str());
        _dot += ".";
        delay(50);
      }
      delay(100);
    } else if (menuIndex >= 1 && menuIndex <= 4) {
      // copy values to temp
      for (int i = 0; i < 5; i++) {
        valuesTemp[i] = values[i];
      }
      subMenuIndex = menuIndex - 1;  //
      btnOkPressed = false;
      while (true) {
        btnEsc.update();
        btnUp.update();
        btnDown.update();
        btnOk.update();
        String _menu = menuItems[menuIndex];
        _menu = "Set " + _menu + " :";
        updateLCD(_menu.c_str(), String(valuesTemp[subMenuIndex]).c_str());
        if (btnUpPressed || btnUpHold) {
          valuesTemp[subMenuIndex] += 0.1;
          btnUpPressed = false;
          if (btnUpHold) {
            delay(1);
          }
        }
        if (btnDownPressed || btnDownHold) {
          valuesTemp[subMenuIndex] -= 0.1;
          btnDownPressed = false;
          if (btnDownHold) {
            delay(1);
          }
        }
        if (btnOkPressed) {
          // save to eeprom
          SaveToEEPROM(subMenuIndex * sizeof(float), valuesTemp[subMenuIndex]);
          btnOkPressed = false;
          updateLCD("Saveing", ".");
          String _dot = ".";
          for (int i = 0; i < 5; i++) {
            updateLCD("Saveing", _dot.c_str());
            _dot += ".";
            delay(50);
          }
          // update values
          for (int i = 0; i < 5; i++) {
            values[i] = valuesTemp[i];
          }

          delay(100);
          break;
        }
        if (btnEscPressed) {
          btnEscPressed = false;
          break;
        }
      }
    } else if (menuIndex == 5) {
      // ------------ CALIBRATE ------------
      calibrate();
    }
    btnOkPressed = false;
  }
  if (btnEscPressed) {
    menuIndex = 0;
    btnEscPressed = false;
  }
}

void calibrate() {
  updateLCD("Please", "Remove all");
  btnOkPressed = false;
  while (true) {
    btnEsc.update();
    btnOk.update();

    if (btnOkPressed) {
      scale.tare(20);
      btnOkPressed = false;
      float wegiht = 0;
      int32_t offset = scale.get_offset();
      updateLCD("Zero offset", String(offset).c_str());
      delay(900);
      updateLCD("Put weight", "and press OK");
      delay(1000);
      while (true) {
        btnEsc.update();
        btnUp.update();
        btnDown.update();
        btnOk.update();
        if (btnUpPressed || btnUpHold) {
          wegiht++;
          if (wegiht > 1000) {
            wegiht = 1000;
          }
          btnUpPressed = false;        
          if (btnUpHold) {
            delay(1);
          }
        }
        if (btnDownPressed || btnDownHold) {
          wegiht--;
          if (wegiht < 0) {
            wegiht = 0;
          }
          btnDownPressed = false;
          if (btnDownHold) {
            delay(1);
          }
        }

        String _w = String(wegiht) + "g";
        updateLCD("Put weight", _w.c_str());
        if (btnOkPressed) {
          updateLCD("Are you sure", "No: ESC, Yes: OK");
          btnOkPressed = false;
          while (true) {
            btnEsc.update();
            btnOk.update();
            if (btnOkPressed) {
              updateLCD("Calibrate", "...");
              scale.calibrate_scale(wegiht, 20);
              updateLCD("Calibrate", "Done");
              delay(1000);
              float scale_factor = scale.get_scale();
              SaveToEEPROM(70, scale_factor);
              btnOkPressed = false;
              return;
            }
            if (btnEscPressed) {
              btnEscPressed = false;
              return;
            }
          }
          break;
        }
        if (btnEscPressed) {
          btnEscPressed = false;
          break;
        }
      }
      break;
    }
    if (btnEscPressed) {
      btnEscPressed = false;
      return;
    }
  }
}
bool testing, testStamp = false;
float weightStamp = 0;

#define NUM_SAMPLES 4  // Number of samples for moving average
float weightBuffer[NUM_SAMPLES] = {0}; // Buffer for moving average
int bufferIndex = 0;

float getFilteredWeight() {
  float sum = 0.0;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    sum += weightBuffer[i];
  }
  return sum / NUM_SAMPLES;  // คำนวณค่าเฉลี่ยของค่าที่อ่านได้
}

float customRound(float value) {
  int intPart = (int)value;
  float decimalPart = value - intPart;

  if (decimalPart <= 0.4) {
    return intPart * decimalPart;
  } else if (decimalPart <= 0.6) {
    return intPart + decimalPart;
  } else {
    return intPart + decimalPart;
  }
}


void updateWeightI(){
  float newWeight = scale.get_units();
  weightBuffer[bufferIndex] = newWeight;
  bufferIndex = (bufferIndex + 1) % NUM_SAMPLES;  
  float filteredWeight = getFilteredWeight();

    if (abs(filteredWeight - w1) > 0.1) 
    {
      w1 = round(filteredWeight * 10) / 10.0;  // Fix to 1 decimal place
      if (w1 > -1 && w1 < 0.7) {
        w1 = 0.0;
      }
      // Fix to 1 decimal place
      w1 = customRound(w1);
    }
}

void updateDispalyCountDown(){
  unsigned long currentMillis = millis();
  if (currentMillis - lastReadTime >= 700)
  {
    lastReadTime = currentMillis;
    String line1 = "START IN";
    String line2 = String(countDownStart);
    updateLCD(line1.c_str(), line2.c_str());

    countDownStart--;
    if(countDownStart == 1)
    { 
      scale.tare();
      updateWeightI();
    }
    if(countDownStart == 0)
    {
      countDownStart = 0;
      // Zero weight offset
      relayAir2.onFor(500);
      delay(100);
    }

    // Count down stop for after run
    if(countDownStop > 0)
    {
      countDownStop--;
     if(countDownStop == 0)
     {
      countDownStop = 0;
      input2McRunState = false;
     }
    }

  } else if (currentMillis < lastReadTime) {
    lastReadTime = currentMillis;  // Overflow
  }
}
bool isRalayIsOn = false;
void updateWeight() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastReadTime >= interval) {
    lastReadTime = currentMillis;
    updateWeightI();
    String _w = String(w1, 1) + "g";
    // PROCESS TEST HERE
    if (input1SensorState) {
      String line1 = "";
      String line2 = "";
      line1 = "Running";
      line2 = "W:" + _w;
      if (input1Sensor) {
        input1Sensor = false;
        testStamp = false;
        resultNG = false;

        buzzer.toggleFor(1);
        setLedColor(BLUE);
      }

      if (input2McRunState) {
        testStamp = true;
        if (input2McRun) {
          input2McRun = false;
          isRalayIsOn = true;
          scale.tare();
          testing = true;
          updateWeightI();
        }
        line1 = "Testing";
        if (w1 > values[0] && isRalayIsOn) {
          line2 = "W:" + _w + " >" + String(values[0]) + " OK";
          relayAir1.on();
          // relayAir2.on();
          isRalayIsOn = false;
        }else if(isRalayIsOn)
        {
          line2 = "W:" + _w + " >" + String(values[0]) + " ...";
          relayAir1.off();
          // relayAir2.off();
        }else
        {
          line2 = "W:" + _w + " >" + String(values[0]) + " ---";
        }

      } else {
        if (testStamp) {
          line1 = "Validate";
          if(resultNG){
            line1 += " NG";
          }else{
            line1 += " OK";
          }
          _w = String(weightStamp, 1) + "g";
          line2 = "W:" + _w + " >" + String(values[2]) + "|" + String(values[3]) + "<";
          if(testing){
            testing = false;
            if (w1 >= values[2] && w1 <= values[3]) {
              buzzer.toggleFor(2);
              line1 += " OK";
              setLedColor(GREEN);
            } else {
              setLedColor(RED);
              line1 += " NG";
              resultNG = true;
            }

            weightStamp = w1;
          }          
        }
      }
      updateLCD(line1.c_str(), line2.c_str());
    } else {
      if (input1Sensor) {
        input1Sensor = false;
        buzzer.stopToggle();
        buzzer.off();
        scale.tare();
        setLedColor(OFF);
      }

      _w = "Weight: " + _w;
      updateLCD("AUTO", _w.c_str());
    }
    if(resultNG)
    {
      setLedColor(RED);
      buzzer.toggleFor(15);
    }
  } else if (currentMillis < lastReadTime) {
    lastReadTime = currentMillis;  // Overflow
  }
}

void btnEscOnEventChanged(bool state) {
  if (state) {
    Serial.println("ESC");
    if (menu == 0) {
      menu = 1;
      menuIndex = 0;
      subMenuIndex = 0;

    } else if (menu == 1 && menuIndex == 0) {
      menu = 0;
      String _w = String(w1, 1) + "g";
      updateLCD("AUTO", _w.c_str());
    }
    btnEscPressed = true;
  }
}

void btnUpOnEventChanged(bool state) {
  if (state) {
    Serial.println("UP");
    btnUpPressed = true;
  }

  btnUpHold = false;
}

void btnUpHoldCallback() {
  Serial.println("UP_HOLD");
  btnUpHold = true;
}

void btnDownOnEventChanged(bool state) {
  if (state) {
    Serial.println("DOWN");
    btnDownPressed = true;
  }

  btnDownHold = false;
}

void btnDownHoldCallback() {
  Serial.println("DOWN_HOLD");
  btnDownHold = true;
}

void btnOkOnEventChanged(bool state) {
  if (state) {
    Serial.println("OK");
    btnOkPressed = true;
  }
}

// SENSOR 
void inputPin1OnEventChanged(bool state) {
  input1SensorState = state;
  input1Sensor = true;

  resultNG = false;
  if (state) {
    countDownStart = _countDownStart;
    lastReadTime = 0;
    Serial.println("INPUT_PIN1");
  } else {
    // END TEST
    relayAir1.off();
    relayAir2.off();
    resultNG = false;
  }
}

// M/C RUN
void inputPin2OnEventChanged(bool state) {
  // input2McRunState = state;
  if (state) {
    Serial.println("INPUT_PIN2");
    input2McRun = true;
    input2McRunState = true;
    countDownStop = 0;
  } else {
    countDownStop = _countDownStop; // 5 seconds countdown
  }
}

// -------------- LCD --------------
char currentLine1[17] = "                ";  // 16 characters + null terminator
char currentLine2[17] = "                ";  // 16 characters + null terminator

void updateLCD(const String newDataLine1, const String newDataLine2) {
  updateLCD(newDataLine1.c_str(), newDataLine2.c_str());
}

void updateLCD(const char *newDataLine1, const char *newDataLine2) {
  updateLCDLine(newDataLine1, currentLine1, 0);
  updateLCDLine(newDataLine2, currentLine2, 1);
}

void updateLCDLine(const char *newData, char (&currentLine)[17], int row) {
  int i;
  // Update characters as long as they are different or until newData ends
  for (i = 0; i < 16 && newData[i]; i++) {
    if (newData[i] != currentLine[i]) {
      lcd.setCursor(i, row);
      lcd.print(newData[i]);
      currentLine[i] = newData[i];
    }
  }
  // Clear any remaining characters from the previous display
  for (; i < 16; i++) {
    if (currentLine[i] != ' ') {
      lcd.setCursor(i, row);
      lcd.print(' ');
      currentLine[i] = ' ';
    }
  }
}
// --------------- END LCD ---------------
