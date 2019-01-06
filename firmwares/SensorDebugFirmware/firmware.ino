#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>

Adafruit_BNO055 bno = Adafruit_BNO055(55);

void setup(void) {

    Serial.begin(115200);
    delay(100);

    Wire.pins(4, 5);

    if(!bno.begin()) {
        Serial.println("Could not find BNO055");
    }
}

void loop(void) {
    imu::Quaternion quat = bno.getQuat();
    Serial.print(quat.w());
    Serial.print(",");
    Serial.print(quat.x());
    Serial.print(",");
    Serial.print(quat.y());
    Serial.print(",");
    Serial.print(quat.z());
    Serial.println(",");
    delay(100);
}