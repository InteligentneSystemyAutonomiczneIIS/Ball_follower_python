#pragma once

#include <Servo.h>
#include <elapsedMillis.h>


struct dataPacket 
{
    char message[64] = {0};
    int packet_int = 0;
    float packet_float = 0.0f;

};


int commandByte = 0;
String inString = "";

//constants for servos
//left-right
Servo servoYaw;
// VarSpeedServo servoYaw;
int yawMin = 45, yawMax = 135;
int yawCurrent = 90;
int horizontalFOV = 50;
int yawVectorMin = -100;
int yawVectorMax = 100;

//up-down
Servo servoPitch;
// VarSpeedServo servoPitch;
int pitchMin = 55, pitchMax = 125;
int pitchCurrent = 90;
int verticalFOV = 40;
int pitchVectorMin = -100;
int pitchVectorMax = 100;

int deadZone = 10;
int deadZoneDegrees = 5;
bool isPaused = true;
int servoMoveSpeed = 10; //miliseconds per degree of rotation


//Initialization functions
void initServos()
{
    servoYaw.attach(20);
    servoPitch.attach(21);
    servoYaw.write(yawCurrent);
    servoPitch.write(pitchCurrent);
}

int read_number()
{
    inString = Serial.readStringUntil('\n');
    return inString.toInt();
}

int yawVectorToDegrees(int x)
{
    int out_max = int(horizontalFOV/2);
    int out_min = -int(horizontalFOV/2);
    return (x - yawVectorMin) * (out_max - out_min) / (yawVectorMax - yawVectorMin) + out_min;
}

int pitchVectorToDegrees(int x)
{
    int out_max = int(verticalFOV/2);
    int out_min = -int(verticalFOV/2);
    return (x - pitchVectorMin) * (out_max - out_min) / (pitchVectorMax - pitchVectorMin) + out_min;
}
