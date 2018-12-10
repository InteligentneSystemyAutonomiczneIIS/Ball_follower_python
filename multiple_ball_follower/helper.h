#pragma once

#include <Servo.h>
#include <elapsedMillis.h>
#include <tuple>

struct ballData
{
    int x = -100;
    int y = -100;
    int radius = 0;
};

struct ballPositionsPacket
{
    ballData redBall;
    ballData greenBall;
    ballData blueBall;
};

enum packetTypes
{
    invalidHeader,
    echoHeader,
    linesConfigHeader_left,
    linesConfigHeader_right,
    ballsPositionHeader
};


class Line
{
    public:
    // y = ax + b <==> (-a)x + 1*y + (-b) 
    float m_a = 0.0f;
    float m_b = 0.0f;

    public:
    Line(){
    }

    Line(float a, float b): m_a{a}, m_b{b}
    {

    }

    void calculate_a_b_using_two_points(int x_a, int y_a, int x_b, int y_b)
    {
        m_a = ((float)(y_a - y_b)/(float)(x_a - x_b));
        m_b = (float)y_a - (m_a * (float)x_a); 
    }


    /*
    OR (https://math.stackexchange.com/questions/274712/calculate-on-which-side-of-a-straight-line-is-a-given-point-located)
    To determine which side of the line from A=(x1,y1) to B=(x2,y2) a point P=(x,y) falls on you need to compute the value:-
    d=(x−x1)(y2−y1)−(y−y1)(x2−x1)
    If d<0 then the point lies on one side of the line, and if d>0 then it lies on the other side. If d=0
    then the point lies exactly line.
    To see whether points on the left side of the line are those with positive or negative values compute the value for d
    for a point you know is to the left of the line, such as (x1−1,y1) and then compare the sign with the point you are interested in.
    */

    std::tuple<bool, int> is_ball_left_of_line(ballData ball)
    {
        int distance = calculate_distance_point_to_line(ball.x, ball.y);
        bool isLeftOf = ((abs(distance) > ball.radius) && distance*((m_a > 0) - (m_a < 0)) > 0) ? true : false;
        int distance_ball_to_line = abs(distance) - ball.radius; 
        return std::make_tuple(isLeftOf, distance_ball_to_line);
    }


    std::tuple<bool, int> is_ball_right_of_line(ballData ball)
    {
        int distance = calculate_distance_point_to_line(ball.x, ball.y);
        bool isRightOf = ((abs(distance) > ball.radius) && distance*((m_a > 0) - (m_a < 0)) < 0) ? true : false;
        int distance_ball_to_line = abs(distance) - ball.radius; 
        return std::make_tuple(isRightOf, distance_ball_to_line);
    }

    private:

    int calculate_y(int x)
    {
        return (int)(m_a * x + m_b);
    }



    int calculate_distance_point_to_line(int x, int y)
    {

        float a = -m_a, b = 1.0, c = -m_b;
        float numerator = a*x + b*y + c;
        // numerator = abs(numerator);
        float denominator = sqrt(a*a + b*b);

        float distance = numerator/denominator;

        return (int)distance;
    }

};


int commandByte = 0;
String inString = "";
ballPositionsPacket currentBallPositions;
Line leftLine;
Line rightLine;




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


packetTypes resolveHeader(const char * header)
{
    int maxHeaderLength = 20;
    if (strncmp(header, "configL", maxHeaderLength) == 0) return packetTypes::linesConfigHeader_left;
    if (strncmp(header, "configR", maxHeaderLength) == 0) return packetTypes::linesConfigHeader_right;
    if (strncmp(header, "ballPositions", maxHeaderLength) == 0) return packetTypes::ballsPositionHeader;
    if (strncmp(header, "echoHeader", maxHeaderLength) == 0 ) return packetTypes::echoHeader;

    return packetTypes::invalidHeader;

}