#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

/* Constants */

const int sensor_id = 0x00;  // Identifier of the sensor

const char* ssid = "";  // SSID of the network

const char* password = ""; // Password of the network

const IPAddress host = {192, 168, 1, 117};

const int remote_port = 3000;  // Port for the initial connection

const int local_port = 9000;  // Local port for UDP communication

/* Buffers */

char buffer[100];  // Message buffer

char read_buffer[255];  // Buffer to read received messages

int generation = 0;  // Number of datapoints collected

/* Global Structs */

WiFiUDP Udp;

Adafruit_BNO055 bno = Adafruit_BNO055(55);

/* Packet structure for data transport
*   
*    ----------------------------------------------------------------------------------------------
*    | Sensor Identifier | AX | AY | AZ | GX | GY | GZ | MX | MY | MZ | Generation (packet number)|
*    ----------------------------------------------------------------------------------------------
*       The sensor identifier is a number which is assigned to the sensor in its
*       firmware. During the visualiation of the data, the sensor identifier is 
*        matched to the joint for which the sensor recorded the data.
*       
*        AX..MZ are the raw sensor readings
*
*        The Generation (or packet number) is used to determine the order of datapoints
*        since the order of arrival might not be chronological.
*
*/
struct sensor_packet_t {
    int id;
    float ax;
    float ay;
    float az;
    float gx;
    float gy;
    float gz;
    float mx;
    float my;
    float mz;
    int generation;
};

/* Helper function to push onto the message stack */
void push(int n, char* buf, int top) {
    if (n < 0) {
        n = n * -1;
        buf[top] = (n >> 24) & 0xFF;
        buf[top + 1] = (n >> 16) & 0xFF;
        buf[top + 2] = (n >> 8) & 0xFF;
        buf[top + 3] = n & 0xFF;
        buf[top + 4] = 1; // the sign flag        
    } else {
        buf[top] = (n >> 24) & 0xFF;
        buf[top + 1] = (n >> 16) & 0xFF;
        buf[top + 2] = (n >> 8) & 0xFF;
        buf[top + 3] = n & 0xFF;
        buf[top + 4] = 0;    
    }
}


/* Helper function to turn floating point numebrs into integers
*   Takes a float and the precison to round to
*   The point is shifted by the given precision
*   and rounded to the next integer. This integer
*   is then given returned.
*
*/
int round_to_int(float x, float prec) {
    x = x * pow(10, prec);
    x = round(x);
    return (int) x;
}


void setup(void) {

    // DEBUG:
    Serial.begin(115200);
    delay(100);

    Wire.pins(4, 5);  // Setup the IO pins

    if(!bno.begin()) {
        // DEBUG:
        Serial.println("Could not find BNO055");
    }
    Serial.print("Connecting to the network ");
    Serial.println(ssid);
    
    // Connect to the network
    WiFi.begin(ssid, password);

    while(WiFi.status() != WL_CONNECTED) {
        delay(500);
        // DEBUG:
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP Address:");
    Serial.println(WiFi.localIP());

    delay(1000);

    Udp.begin(local_port);

    // Tell the server this sensor is ready 
    // by sending the current orientation

    imu::Vector<3> euler = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
    int roll = round_to_int(euler.x(), 4.0);
    int pitch = round_to_int(euler.y(), 4.0);
    int yaw = round_to_int(euler.z(), 4.0);
    int top = 0;
    push(sensor_id, buffer, top);
    top = top + 5;
    push(roll, buffer, top);
    top = top + 5;
    push(pitch, buffer, top);
    top = top + 5;
    push(yaw, buffer, top);
    top = top + 5;

    Udp.beginPacket(host, remote_port);
    Udp.write(buffer, 100);
    Udp.endPacket();

    // Wait for the recording to begin

    while(Udp.parsePacket() < 1) {
        delay(1000);
    }

};

void loop(void) {
    imu::Vector<3> acc = bno.getVector(Adafruit_BNO055::VECTOR_ACCELEROMETER);  // m/s^2
    imu::Vector<3> gyr = bno.getVector(Adafruit_BNO055::VECTOR_GYROSCOPE);  // radians per second, rps
    imu::Vector<3> mag = bno.getVector(Adafruit_BNO055::VECTOR_MAGNETOMETER);  // micro Teslas, uT
    Serial.println(gyr.z());
    sensor_packet_t packet = {.id = sensor_id, .ax = round_to_int(acc.x(), 4.0), .ay = round_to_int(acc.y(), 4.0), .az = round_to_int(acc.z(), 4.0), .gx = round_to_int(gyr.x(), 4.0), .gy = round_to_int(gyr.y(), 4.0), .gz = round_to_int(gyr.z(), 4.0), .mx = round_to_int(mag.x(), 4.0), .my = round_to_int(mag.y(), 4.0), .mz = round_to_int(mag.z(), 4.0), .generation = generation};
    generation = generation + 1;
    int top = 0;
    Serial.println(packet.gz);
    push(packet.id, buffer, top);
    top = top + 5;
    push(packet.ax, buffer, top);
    top = top + 5;
    push(packet.ay, buffer, top);
    top = top + 5;
    push(packet.az, buffer, top);
    top = top + 5;
    push(packet.gx, buffer, top);
    top = top + 5;
    push(packet.gy, buffer, top);
    top = top + 5;
    push(packet.gz, buffer, top);
    top = top + 5;
    push(packet.mx, buffer, top);
    top = top + 5;            
    push(packet.my, buffer, top);
    top = top + 5;
    push(packet.mz, buffer, top);
    top = top + 5;
    push(packet.generation, buffer, top);

    Udp.beginPacket(host, remote_port);
    Udp.write(buffer, 100);
    Udp.endPacket();
    
    if(Udp.parsePacket() > 1) {  // If the server sends a byte, the sensor will end the recording i.e. shutdown
        exit(0);
    }

    delay(100);  // Set the delay to 100ms since the sensor is only capable of reading at 100Hz

};