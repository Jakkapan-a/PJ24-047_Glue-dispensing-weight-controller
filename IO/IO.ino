const int inputPin = 5;   // ขา Digital Pin ที่เชื่อมกับ EL817
int lastState = HIGH;     // เก็บสถานะก่อนหน้า (เริ่มต้นเป็น HIGH)

void setup() {
  pinMode(inputPin, INPUT_PULLUP);
  Serial.begin(9600);
}

void loop() {
  int currentState = digitalRead(inputPin);  // อ่านค่าจาก EL817

  // ตรวจจับการเปลี่ยนแปลงของสถานะ
  if (currentState != lastState) {
    Serial.print("State changed: ");
    Serial.println(currentState ? "HIGH" : "LOW");

    lastState = currentState;  // อัปเดตค่าสถานะล่าสุด
  }

  delay(1000);  // ลด noise ป้องกัน false trigger
}
