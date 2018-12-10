/*
This code is based on the examples at http://forum.arduino.cc/index.php?topic=396450
*/


// Example 5 - parsing text and numbers with start and end markers in the stream

#include "helper.h"

const byte numChars = 128;
char receivedChars[numChars];
char tempChars[numChars];        // temporary array for use when parsing

bool leftLineConfigured = false, rightLineConfigured = false;
bool configurationComplete = false;


boolean newData = false;

//============

void setup() {
    Serial.begin(57600);
    delay(1000);
    Serial.println("Enter data in this style: <echoHeader, 12, 24>  ");
    Serial.println();
}

//============

void loop() {
    recvWithStartEndMarkers();
    if (newData == true) 
    {
        // this temporary copy is necessary to protect the original data
        // because strtok() used in parseData() replaces the commas with \0
        strcpy(tempChars, receivedChars);
        parseData();
        newData = false;

        if (configurationComplete)
        {
            //RED ball
            if (currentBallPositions.redBall.x >= 0 && currentBallPositions.redBall.y >= 0 && currentBallPositions.redBall.radius != 0)
            {
                //SIMPLIFIED CONDITION - BE CAREFUL WHEN USING !
                std::tuple<bool, int> isLeftOf = leftLine.is_ball_left_of_line(currentBallPositions.redBall);
                std::tuple<bool, int> isRightOf = rightLine.is_ball_right_of_line(currentBallPositions.redBall);

                bool isOnTheLeft = std::get<0>(isLeftOf);
                int distanceToLeft = std::get<1>(isLeftOf);
                bool isOnTheRight = std::get<0>(isRightOf);
                int distanceToRight = std::get<1>(isRightOf); 
                if (isOnTheLeft == false && distanceToLeft > 0 && isOnTheRight == false && distanceToRight > 0)
                {
                    Serial.println("Red ball is fully between lines");
                }

            }
            else 
            {
                Serial.println("Red ball not currently tracked");
            }

            //GREEN ball
            if (currentBallPositions.greenBall.x >= 0 && currentBallPositions.greenBall.y >= 0 && currentBallPositions.greenBall.radius != 0)
            {
                //SIMPLIFIED CONDITION - BE CAREFUL WHEN USING !
                std::tuple<bool, int> isLeftOf = leftLine.is_ball_left_of_line(currentBallPositions.greenBall);
                std::tuple<bool, int> isRightOf = rightLine.is_ball_right_of_line(currentBallPositions.greenBall);

                bool isOnTheLeft = std::get<0>(isLeftOf);
                int distanceToLeft = std::get<1>(isLeftOf);
                bool isOnTheRight = std::get<0>(isRightOf);
                int distanceToRight = std::get<1>(isRightOf); 
                if (isOnTheLeft == false && distanceToLeft > 0 && isOnTheRight == false && distanceToRight > 0) 
                {
                    Serial.println("Green ball is fully between lines");
                }

            }
            else 
            {
                Serial.println("Green ball not currently tracked");
            }

            //BLUE ball
            if (currentBallPositions.blueBall.x >= 0 && currentBallPositions.blueBall.y >= 0 && currentBallPositions.blueBall.radius != 0)
            {
                //SIMPLIFIED CONDITION - BE CAREFUL WHEN USING !
                std::tuple<bool, int> isLeftOf = leftLine.is_ball_left_of_line(currentBallPositions.blueBall);
                std::tuple<bool, int> isRightOf = rightLine.is_ball_right_of_line(currentBallPositions.blueBall);

                bool isOnTheLeft = std::get<0>(isLeftOf);
                int distanceToLeft = std::get<1>(isLeftOf);
                bool isOnTheRight = std::get<0>(isRightOf);
                int distanceToRight = std::get<1>(isRightOf);  
                if (isOnTheLeft == false && distanceToLeft > 0 && isOnTheRight == false && distanceToRight > 0)
                {
                    Serial.println("Blue ball is fully between lines");
                }

            }
            else 
            {
                Serial.println("Blue ball not currently tracked");
            }
            
        }
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

void parseData() {      // split the data into its parts

    char header[64] = {0};
    char * strtokIndx; // this is used by strtok() as an index

    int x_a = 0, x_b = 0, y_a = 0, y_b = 0;

    std::tuple <bool, int> result;

    strtokIndx = strtok(tempChars,",");      // get the first part - the string
    strcpy(header, strtokIndx); 

    switch(resolveHeader(header))
    {
        case packetTypes::echoHeader:
            Serial.println("Echo reply");
            break;
        case packetTypes::linesConfigHeader_left:
            strtokIndx = strtok(NULL, ",");
            x_a = atoi(strtokIndx);
            strtokIndx = strtok(NULL, ",");
            y_a = atoi(strtokIndx);
            strtokIndx = strtok(NULL, ",");
            x_b = atoi(strtokIndx);
            strtokIndx = strtok(NULL, ",");
            y_b = atoi(strtokIndx);
            leftLine.calculate_a_b_using_two_points(x_a, y_a, x_b, y_b);
            Serial.println(String("Left Line params: ") + leftLine.m_a + "; " + leftLine.m_b);
            leftLineConfigured = true;
            if (leftLineConfigured && rightLineConfigured) configurationComplete = true;

            break;
        case packetTypes::linesConfigHeader_right:
            strtokIndx = strtok(NULL, ",");
            x_a = atoi(strtokIndx);
            strtokIndx = strtok(NULL, ",");
            y_a = atoi(strtokIndx);
            strtokIndx = strtok(NULL, ",");
            x_b = atoi(strtokIndx);
            strtokIndx = strtok(NULL, ",");
            y_b = atoi(strtokIndx);
            rightLine.calculate_a_b_using_two_points(x_a, y_a, x_b, y_b);
            Serial.println(String("Right Line params: ") + rightLine.m_a + "; " + rightLine.m_b);
            rightLineConfigured = true;
            if (leftLineConfigured && rightLineConfigured) configurationComplete = true;

            break;
        case packetTypes::ballsPositionHeader:
            //RED
            strtokIndx = strtok(NULL,","); //skip color header for now
            strtokIndx = strtok(NULL, ",");
            currentBallPositions.redBall.x = atoi(strtokIndx);
            strtokIndx = strtok(NULL, ",");
            currentBallPositions.redBall.y = atoi(strtokIndx);
            strtokIndx = strtok(NULL, ",");
            currentBallPositions.redBall.radius = atoi(strtokIndx);
            
            //GREEN
            strtokIndx = strtok(NULL,","); //skip color header for now
            strtokIndx = strtok(NULL, ",");
            currentBallPositions.greenBall.x = atoi(strtokIndx);
            strtokIndx = strtok(NULL, ",");
            currentBallPositions.greenBall.y = atoi(strtokIndx);
            strtokIndx = strtok(NULL, ",");
            currentBallPositions.greenBall.radius = atoi(strtokIndx);

            //BLUE
            strtokIndx = strtok(NULL,","); //skip color header for now
            strtokIndx = strtok(NULL, ",");
            currentBallPositions.blueBall.x = atoi(strtokIndx);
            strtokIndx = strtok(NULL, ",");
            currentBallPositions.blueBall.y = atoi(strtokIndx);
            strtokIndx = strtok(NULL, ",");
            currentBallPositions.blueBall.radius = atoi(strtokIndx);
            break;
        default:
            Serial.println("Invalid header");
            break;
    }
}
