#include <TcBUTTON.h>
#include <TcPINOUT.h>
#include "HX711.h"
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <Wire.h>

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

#define INPUT_PIN1 8 // SENSOR 
#define INPUT_PIN2 9 
#define INPUT_PIN3 10
#define INPUT_PIN4 11

void inputPin1OnEventChanged(bool state);
void inputPin2OnEventChanged(bool state);
// void inputPin3OnEventChanged(bool state);
// void inputPin4OnEventChanged(bool state);

TcPINOUT inputPin1(INPUT_PIN1, inputPin1OnEventChanged);
TcPINOUT inputPin2(INPUT_PIN2, inputPin2OnEventChanged);
// TcPINOUT inputPin3(INPUT_PIN3, inputPin3OnEventChanged);
// TcPINOUT inputPin4(INPUT_PIN4, inputPin4OnEventChanged);

// -------------- OUTPUT --------------
#define BUZZER_PIN 12
TcPINOUT buzzer(BUZZER_PIN);



// -------------- HX711 --------------
float w1 = 0, w2 = 0, previous = 0;
const unsigned long interval = 100;  //
unsigned long lastReadTime = 0;
bool waitingForStability = false;  //

// -------------- EEPROM --------------
template <typename T>
void SaveToEEPROM(int address, T value) {
  EEPROM.put(address, value);
}

template <typename T>
T ReadFromEEPROM(int address) {
  T value;
  EEPROM.get(address, value);
  return value;
}
int menuIndex = 0;
int subMenuIndex = 0;
int values[5] = {0, 0, 0, 0, 0}; // min1, max1, min2, max2, cal weight
int valuesTemp[5] = {0, 0, 0, 0, 0}; // min1, max1, min2, max2, cal weight
// ---------- SETUP ------------
void setup() {
  Serial.begin(115200);
  Serial.print("HX711_LIB_VERSION: ");
  Serial.println(HX711_LIB_VERSION);
  Serial.println();
  scale.begin(dataPin, clockPin);
  float scale_factor = ReadFromEEPROM<float>(50);
  Serial.print("Scale factor: ");
  Serial.println(scale_factor);
  
  if(scale_factor != 0){
    scale.set_scale(scale_factor);
  }else{
    scale.set_scale(406.472167);
  }

  scale.tare();

  lcd.begin();
  lcd.backlight();
  lcd.setCursor(0, 0);
  String dot = ".";
  updateLCD("Loading", "");
  for(int i = 0; i < 10; i++) {
    updateLCD("Loading", dot.c_str());
    dot += ".";
    delay(100);
  }

  // -- Read EEPROM to values
  for(int i = 0; i < 5; i++){
    values[i] = ReadFromEEPROM<int>(i * sizeof(int));
  }
  // ---------------- 
  delay(100);
  String _w = String(w1, 1) + "g";
  updateLCD("Weight", _w.c_str());

  // -- BUTTONS --
  btnUp.setOnHold(btnUpHoldCallback, 1000);
  btnDown.setOnHold(btnDownHoldCallback, 1000);
  buzzer.toggleFor(2, 100);
}
int menu = 0;
bool btnEscPressed = false;
bool btnUpPressed = false;
bool btnDownPressed = false;
bool btnOkPressed = false;
bool btnUpHold = false;
bool btnDownHold = false;
void loop() {
  btnEsc.update();
  btnUp.update();
  btnDown.update();
  btnOk.update();
  buzzer.update();
  if(menu == 0){
    updateWeight();
  }else{
   updateMenu();
  }
}


const char *menuItems[] = {"Zero", "Min1", "Max1", "Min2", "Max2", "Cal"};
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
  if (btnDownPressed)
  {
    menuIndex = (menuIndex < 5) ? menuIndex + 1 : 0;
    btnDownPressed = false;
  }
  if (btnOkPressed) 
  {
    if(menuIndex == 0){
      scale.tare();
      updateLCD("Set zero", ".");
      String _dot = ".";
      for(int i = 0; i < 5; i++){
        updateLCD("Set zero", _dot.c_str());
        _dot += ".";
        delay(50);
      }
      delay(100);
    }else if(menuIndex >= 1 && menuIndex <=4){
      // ------------ SET MIN, MAX
      // copy values to temp
      for(int i = 0; i < 5; i++){
        valuesTemp[i] = values[i];
      }      
      subMenuIndex = menuIndex - 1; //
      btnOkPressed = false;
      while (true) {
        btnEsc.update();
        btnUp.update();
        btnDown.update();
        btnOk.update();
        String _menu = menuItems[menuIndex];
        _menu ="Set "+_menu+ " :";
        updateLCD(_menu.c_str(), String(valuesTemp[subMenuIndex]).c_str());
        if (btnUpPressed || btnUpHold) 
        {
          valuesTemp[subMenuIndex]++;
          btnUpPressed = false;
          if(btnUpHold){
            delay(100);
          }
        }
        if (btnDownPressed || btnDownHold)
        {
          valuesTemp[subMenuIndex]--;
          btnDownPressed = false;
          if(btnDownHold){
            delay(100);
          }
        }
        if (btnOkPressed) {
          // save to eeprom
          SaveToEEPROM(subMenuIndex * sizeof(int), valuesTemp[subMenuIndex]);
          btnOkPressed = false;
          updateLCD("Saveing", ".");
          String _dot = ".";
          for(int i = 0; i < 5; i++){
            updateLCD("Saveing", _dot.c_str());
            _dot += ".";
            delay(100);
          }
          
          // update values
          for(int i = 0; i < 5; i++){
            values[i] = valuesTemp[i];
          }

          delay(1000);
          break;
        }
        if (btnEscPressed) {
          btnEscPressed = false;
          break;
        }
      }
    }else if(menuIndex == 5){
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


void calibrate(){
  updateLCD("Please", "Remove all");
  btnOkPressed = false;
  while (true)
  {
    btnEsc.update();
    btnOk.update();

    if(btnOkPressed){
      scale.tare(20);
      btnOkPressed = false;
      uint32_t wegiht = 0;
      int32_t offset = scale.get_offset();
      updateLCD("Zero offset", String(offset).c_str());
      delay(900);
      updateLCD("Put weight", "and press OK");
      delay(1000);
      while (true)
      {
        btnEsc.update();
        btnUp.update();
        btnDown.update();
        btnOk.update();

        if(btnUpPressed || btnUpHold)
        {
          wegiht++;
          if(wegiht > 1000){
            wegiht = 1000;
          }
          btnUpPressed = false;
          if(btnUpHold){
            delay(100);
          }
        }
        if(btnDownPressed || btnDownHold)
        {
          wegiht--;
          if(wegiht < 0){
            wegiht = 0;
          }
          btnDownPressed = false;
          if(btnDownHold){
            delay(100);
          }
        }

        updateLCD("Put weight", String(wegiht).c_str());
        if(btnOkPressed){
          updateLCD("Are you sure", "Yes: OK, No: ESC");
          btnOkPressed = false;
          while (true)
          {
            btnEsc.update();
            btnOk.update();
            if(btnOkPressed){
              updateLCD("Calibrate", "...");
              scale.calibrate_scale(wegiht, 20);
              updateLCD("Calibrate", "Done");
              delay(1000);
              float scale_factor = scale.get_scale();
              SaveToEEPROM(50, scale_factor);
              btnOkPressed = false;
              return;
            }
            if(btnEscPressed){
              btnEscPressed = false;
              return;
            }
          }
          break;
        }

        if(btnEscPressed){
          btnEscPressed = false;
          break;
        }
      }
      break;
    }
  }
}

void updateWeight() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastReadTime >= interval) {
    lastReadTime = currentMillis;
    float newWeight = scale.get_units(); 
    if (abs(newWeight - w1) > 0.9) 
    {
      w1 = newWeight; 
      if(w1 > -1 && w1 < 0.9)
      {
        w1 = 0;
      }
      String _w = String(w1, 1) + "g";
      updateLCD("Weight", _w.c_str());
    }
  } else if (currentMillis < lastReadTime) {
    lastReadTime = currentMillis;  // Overflow
  }
}

void btnEscOnEventChanged(bool state) {
  if (state) {
    Serial.println("ESC");
    if(menu == 0){
      menu = 1;
      menuIndex = 0;
      subMenuIndex = 0;

    }else if(menu == 1 && menuIndex == 0)
    {
      menu = 0;
      updateLCD("Weight", String(w1, 1).c_str());
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
  if (state) {
    Serial.println("INPUT_PIN1");
  }
}

// M/C RUN
void inputPin2OnEventChanged(bool state) {
  if (state) {
    Serial.println("INPUT_PIN2");
  }
}

// M/C AIR
void inputPin3OnEventChanged(bool state) {
  if (state) {
    Serial.println("INPUT_PIN3");
  }
}

// Not used
void inputPin4OnEventChanged(bool state) {
  if (state) {
    Serial.println("INPUT_PIN4");
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
