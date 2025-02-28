#include "HX711.h"

HX711 scale;

const uint8_t dataPin = 6;
const uint8_t clockPin = 7;

float w1 = 0, w2 = 0, previous = 0;
const unsigned long interval = 100;  // เวลาระหว่างการอ่านค่า
unsigned long lastReadTime = 0;
bool waitingForStability = false;  // ใช้เช็คว่ากำลังตรวจสอบค่าคงที่หรือไม่

void setup()
{
    Serial.begin(115200);
    Serial.print("HX711_LIB_VERSION: ");
    Serial.println(HX711_LIB_VERSION);
    Serial.println();

    scale.begin(dataPin, clockPin);

    Serial.print("UNITS: ");
    Serial.println(scale.get_units(10));

    // load cell factor 20 KG
    // scale.set_scale(127.15);
    // load cell factor 5 KG
    // scale.set_scale(420.52);
    // scale.set_scale(375.149810);
    scale.set_scale(406.472167);
    scale.tare();

    Serial.print("UNITS: ");
    Serial.println(scale.get_units(10));
}

void loop()
{
    unsigned long currentMillis = millis();

    if (!waitingForStability && (currentMillis - lastReadTime >= interval))
    {
        lastReadTime = currentMillis;  // บันทึกเวลาล่าสุด

        w1 = scale.get_units(10);
        waitingForStability = true;  // เริ่มตรวจสอบค่าคงที่
    }

    if (waitingForStability && (currentMillis - lastReadTime >= interval))
    {
        lastReadTime = currentMillis;

        w2 = scale.get_units();

        if (abs(w1 - w2) > 10)
        {
            w1 = w2;  // อัปเดตค่า w1 และรออีกครั้ง
        }
        else
        {
            // ค่าเสถียรแล้ว แสดงผล
            Serial.print("UNITS: ");
            Serial.print(w1);
            Serial.print(" g");

            if (w1 == 0)
            {
                Serial.println();
            }
            else
            {
                Serial.print("\t\tDELTA: ");
                Serial.println(w1 - previous);
                previous = w1;
            }

            waitingForStability = false;  // รีเซ็ตเพื่อลูปใหม่
        }
    }
}
