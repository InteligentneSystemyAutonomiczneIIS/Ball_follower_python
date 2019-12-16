/*
This code is based on the examples at http://forum.arduino.cc/index.php?topic=396450
*/


// Example 5 - parsing text and numbers with start and end markers in the stream

#include "variables.h"
#include "functions.hpp"
// #include "helper.h"

const byte numChars = 64;
char receivedChars[numChars];
char tempChars[numChars];        // temporary array for use when parsing

dataPacket packet;


boolean newData = false;


int yawRequested = 0;
int pitchRequested = 0;
//============

void setup() {

    initSerial(57600);
    Serial.println("This demo expects 3 pieces of data - text, an integer and a floating point value");
    Serial.println("Enter data in this style <HelloWorld, 12, 24.7>  ");
    Serial.println();



    initMotors();

    // Each platform has to do this independently, checked manually
    calibrateServo(ServoSelector::Yaw, 80);
    calibrateServo(ServoSelector::Pitch, 58);

    initServos();
    centerServos();

    initESP826();
    // initLED();
    Brake();
    delay(500);

    Serial.println("Initalization ended");

}

//============


void loop() {

    // Check if servos finished movement
    if (isYawServoMoving == true )
    {
        if (pitchMoveTimer > servoWaitTimePitch)
        {
            // calculate how long the move will take
            float yawActualRequested = yawRequested * damping;
            float yawReal = yawCurrent - yawActualRequested;

            yawRequested -= yawActualRequested;

            if (yawRequested < abs(deadZone))
            {
                Serial.println("yaw deadzone reached");
                isYawServoMoving = false;
            }

            servoWaitTimeYaw = yawActualRequested * servoMillisecondsPerDegree;
            
            // move servo
            moveServo(ServoSelector::Yaw, (int)yawReal);
            
            //set timing variables and block movement
            yawMoveTimer = 0;
        }
    }
    if (isPitchServoMoving == true)
    {
        if (yawMoveTimer > servoWaitTimeYaw)
        {
            // calculate how long the move will take
            float pitchActualRequested = pitchRequested * damping;
            float pitchReal = pitchCurrent - pitchActualRequested;

            pitchRequested -= pitchActualRequested;

            if (pitchRequested < abs(deadZone))
            {
                Serial.println("pitch deadzone reached");
                isPitchServoMoving = false;
            }

            servoWaitTimePitch = pitchActualRequested * servoMillisecondsPerDegree;
            
            // move servo
            moveServo(ServoSelector::Pitch, (int)pitchReal);
            
            //set timing variables and block movement
            pitchMoveTimer = 0;
        }
        
    }


    // parse input data
    recvWithStartEndMarkers();

    if (newData == true) {
        strcpy(tempChars, receivedChars);
            // this temporary copy is necessary to protect the original data
            //   because strtok() used in parseData() replaces the commas with \0
        packet = parseData();
        showParsedData(packet);

        if (strcmp(packet.message, "servo") == 0)
        {
            if (isStopped == false)
            {

                if (isYawServoMoving == false)
                {
                    yawRequested = packet.first;
                    isYawServoMoving = true;     

                }
                if (isPitchServoMoving == false)
                {
                    pitchRequested = packet.second;
                    isPitchServoMoving = true;

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