#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

/* Constants */

const int sensor_id = 0000;  // Identifier of the sensor

const char* ssid = "";  // SSID of the network

const char* password = ""; // Password of the network

const IPAddress host = {192, 168, 1, 117};

const int remote_port = 3000;  // Port for the initial connection

const int local_port = 9000;  // Local port for UDP communication

/* Buffers */

char buffer[30];  // Message buffer

char read_buffer[255];  // Buffer to read received messages

int generation = 0;  // Number of datapoints collected

/* Global Structs */

WiFiUDP Udp;

Adafruit_BNO055 bno = Adafruit_BNO055(55);

/* Packet structure for data transport
*   
*    ----------------------------------------------------------------------
*    | Sensor Idenitifier | Q0 | Q1 | Q2 | Q3 | Generation (packet number)|
*    ----------------------------------------------------------------------
*       The sensor identifier is a number which is assigned to the sensor in its
*       firmware. During the visualiation of the data, the sensor identifier is 
*        matched to the joint for which the sensor recorded the data.
*       
*        Q0-Q3 are the parts of the orientation quaternions supplied by the sensor
*
*        The Generation (or packet number) is used to determine the order of datapoints
*        since the order of arrival might not be chronological.
*
*/
struct sensor_packet_t {
    int id;
    float q0;
    float q1;
    float q2;
    float q3;
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
    imu::Quaternion quat = bno.getQuat(); // Get the absolute sensor position as a quaternion
    Serial.print(quat.w());
    Serial.print(quat.x());
    Serial.print(quat.y());
    Serial.print(quat.z());
    sensor_packet_t packet = {.id = sensor_id, .q0 = round_to_int(quat.w(), 4.0), .q1 = round_to_int(quat.x(), 4.0), .q2 = round_to_int(quat.y(), 4.0), .q3 = round_to_int(quat.z(), 4.0), .generation = generation};
    generation = generation + 1;
    int top = 0;
    push(packet.id, buffer, top);
    top = top + 5;
    push(packet.q0, buffer, top);
    top = top + 5;
    push(packet.q1, buffer, top);
    top = top + 5;
    push(packet.q2, buffer, top);
    top = top + 5;
    push(packet.q3, buffer, top);
    top = top + 5;
    push(packet.generation, buffer, top);
    Serial.println(packet.q1);

    Udp.beginPacket(host, remote_port);
    Udp.write(buffer, 30);
    Udp.endPacket();
    
    if(Udp.parsePacket() > 1) {  // If the server sends a byte, the sensor will end the recording i.e. shutdown
        exit(0);
    }

    delay(100);  // Set the delay to 100ms since the sensor is only capable of reading at 100Hz

};