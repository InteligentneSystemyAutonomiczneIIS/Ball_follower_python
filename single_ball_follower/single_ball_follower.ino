/*
This code is based on the examples at http://forum.arduino.cc/index.php?topic=396450
*/


// Example 5 - parsing text and numbers with start and end markers in the stream

#include "variables.h"
#include "functions.hpp"
// #include "helper.h"

float YawCalibrationCenter = 80.0f;
float PitchCalibrationCenter = 58.0f;
float HorizontalFOV = 62.0f;
float VerticalFOV = 37.0f;

const byte numChars = 64;
char receivedChars[numChars];
char tempChars[numChars];        // temporary array for use when parsing

dataPacket packet;


boolean newData = false;


int yawRequested = 0;
int pitchRequested = 0;


float yawErrorAccumulated = 0;
float pitchErrorAccumulated = 0;
//============

void setup() {

    initSerial(115200);
    Serial.println("This demo expects 3 pieces of data - text, an integer and a floating point value");
    Serial.println("Enter data in this style <HelloWorld, 12, 24.7>  ");
    Serial.println();

    initMotors();

    // Each platform has to do this independently, checked manually
    calibrateServo(ServoSelector::Yaw, (int)YawCalibrationCenter);
    calibrateServo(ServoSelector::Pitch, (int)PitchCalibrationCenter);

    initServos();
    centerServos();

    initESP826();
    // initLED();
    Brake();
    delay(500);

    // Serial.println("Initalization ended");

}

//============



void loop() {
      
    // parse input data
    recvWithStartEndMarkers();

    if (newData == true) {
        strcpy(tempChars, receivedChars);
            // this temporary copy is necessary to protect the original data
            //   because strtok() used in parseData() replaces the commas with \0
        packet = parseData();
        // showParsedData(packet);

        if (strcmp(packet.message, "servo") == 0)
        {
            if (isStopped == false)
            {
                yawRequested = packet.first;
                pitchRequested = packet.second;

                if (yawRequested != 0)
                {
                    float yawError = -(yawRequested / (HorizontalFOV/2) );
                    float Kp = 25.0f;
                    float Ki = 4.0f;

                    float output = Kp * yawError + Ki * yawErrorAccumulated;
                    yawErrorAccumulated += yawError;
                    
                    moveServo(ServoSelector::Yaw, (int)(YawCalibrationCenter + output));

                }
                if (pitchRequested != 0)
                {
                    // calculate how long the move will take
                    float pitchError = -(pitchRequested / (VerticalFOV/2));
                    float Kp = 15.0f;
                    float Ki = 3.0f;

                    float output = Kp * pitchError + Ki * pitchErrorAccumulated;
                    pitchErrorAccumulated += pitchError;
                    
                    // move servo
                    moveServo(ServoSelector::Pitch,  (int)(PitchCalibrationCenter + output));
                }
                 
            }

        }

        if (strcmp(packet.message, "stop") == 0)
        {
            isStopped = true;
        }

        if (strcmp(packet.message, "start") == 0)
        {
            isStopped = false;
        }

        newData = false;
    }
}

//============

void recvWithStartEndMarkers() {
    static boolean recvInProgress = false;
    static byte ndx = 0;
    char startMarker = '<';
    char endMarker = '>';
    char rc;

    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();

        if (recvInProgress == true) {
            if (rc != endMarker) {
                receivedChars[ndx] = rc;
                ndx++;
                if (ndx >= numChars) {
                    ndx = numChars - 1;
                }
            }
            else {
                receivedChars[ndx] = '\0'; // terminate the string
                recvInProgress = false;
                ndx = 0;
                newData = true;
            }
        }

        else if (rc == startMarker) {
            recvInProgress = true;
        }
    }
}

//============

dataPacket parseData() {      // split the data into its parts

    dataPacket tmpPacket;

    char * strtokIndx; // this is used by strtok() as an index

    strtokIndx = strtok(tempChars,",");      // get the first part - the string
    strcpy(tmpPacket.message, strtokIndx); // copy it to messageFromPC
 
    strtokIndx = strtok(NULL, ","); // this continues where the previous call left off
    tmpPacket.first = atoi(strtokIndx);     // convert this part to an integer

    strtokIndx = strtok(NULL, ",");
    tmpPacket.second = atof(strtokIndx);     // convert this part to a float

    return tmpPacket;
}


void showParsedData(dataPacket packet) {
    Serial.print("Message ");
    Serial.println(packet.message);
    Serial.print("Yaw ");
    Serial.println(packet.first);
    Serial.print("Pitch ");
    Serial.println(packet.second);
}