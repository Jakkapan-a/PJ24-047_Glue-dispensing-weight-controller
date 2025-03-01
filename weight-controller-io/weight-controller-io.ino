#include <TcBUTTON.h>
#include <TcPINOUT.h>
#include <SoftwareSerial.h>

const byte rxPin = 2;
const byte txPin = 3;

// Set up a new SoftwareSerial object
SoftwareSerial mySerial(rxPin, txPin);

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



#define BUZZER_PIN 10
TcPINOUT buzzer(BUZZER_PIN);

#define RELAY_AIR1 4
#define RELAY_AIR2 5

TcPINOUT relayAir1(RELAY_AIR1);
TcPINOUT relayAir2(RELAY_AIR2);


#define BUFFER_SIZE_DATA 128
uint32_t timeStamp = 0;
bool endReceived = false;
char inputString[BUFFER_SIZE_DATA];
int inputStringLength = 0;



void serialEventRead() {
  while (mySerial.available())
  {
    char inChar = (char)mySerial.read();

    if (inChar == '\n' || inChar == '#') {
      endReceived = true;
    } else {
      if(inChar == '$'){
        inputStringLength = 0;
        continue;
      }else if(inChar == '#'){
        continue;
      }

      if (inputStringLength < BUFFER_SIZE_DATA - 1) {
        inputString[inputStringLength] = inChar;
        inputStringLength++;
      } else {
        inputStringLength = 0;
      }
      timeStamp = millis();
    }
  }
}

bool resultNG = false;
void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);
  relayAir1.off();
  relayAir2.off();
  buzzer.off();
}

void loop() {
  serialEventRead();
  buzzer.update();
  uint32_t currentTime = millis();
  manageSerialInput(currentTime);

  if(resultNG){
    buzzer.toggleFor(15);
  }
}

void manageSerialInput(uint32_t currentTime) {
  // -------------- Serial -------------- //
  if (endReceived) {
    inputString[inputStringLength] = '\0';
    parseData(inputString);

    memset(inputString, 0, sizeof(inputString));
    inputStringLength = 0;
    endReceived = false;
    Serial.println("$REV#");
  } else if (currentTime - timeStamp > 1000 && inputStringLength > 0) {
    endReceived = true;
  }
}

void parseData(String data) 
{
  Serial.print("Received: ");
  Serial.println(data);

  if(data == "R1"){
    relayAir1.on();
    relayAir1.on();
  }else if(data == "R0"){
    relayAir1.off();
    relayAir1.off();
  }else if(data == "BOK"){
    buzzer.toggleFor(2);
    resultNG = false;
  }else if(data == "BNG"){
    resultNG = true;
  }else if(data == "B0"){
    buzzer.stopToggle();
    buzzer.off();
  }else if(data == "B1"){
    buzzer.toggleFor(1);
  }else if(data == "RST"){
    resultNG = false;
    relayAir1.off();
    relayAir2.off();
    buzzer.stopToggle();
    buzzer.off();
  }
}



void btnEscOnEventChanged(bool state) {
  if (state) {
    Serial.println("ESC");
    // if (menu == 0) {
    //   menu = 1;
    //   menuIndex = 0;
    //   subMenuIndex = 0;

    // } else if (menu == 1 && menuIndex == 0) {
    //   menu = 0;
    //   updateLCD("Weight", String(w1, 1).c_str());
    // }
    // btnEscPressed = true;
  }
}

void btnUpOnEventChanged(bool state) {
  if (state) {
    Serial.println("UP");
    // btnUpPressed = true;
  }

  // btnUpHold = false;
}

void btnUpHoldCallback() {
  Serial.println("UP_HOLD");
  // btnUpHold = true;
}

void btnDownOnEventChanged(bool state) {
  if (state) {
    Serial.println("DOWN");
    // btnDownPressed = true;
  }

  // btnDownHold = false;
}

void btnDownHoldCallback() {
  Serial.println("DOWN_HOLD");
  // btnDownHold = true;
}

void btnOkOnEventChanged(bool state) {
  if (state) {
    Serial.println("OK");
    // btnOkPressed = true;
  }
}