# parts of the code are based on https://www.pyimagesearch.com/2016/01/04/unifying-picamera-and-cv2-videocapture-into-a-single-class-with-opencv/
# before running the code install imutils for python3 (pip3 install imutils)
# parts of the code are based on https://github.com/HassamSheikh/multiple-color-tracking-opencv/blob/master/ball_tracking.py
# AND https://pastebin.com/WVhfmphS

import time
from imutils.video import VideoStream
import serial
# from picamera.array import PiRGBArray
# from picamera import PiCamera
import numpy as np
import cv2
import platform
import os


def translate(value, oldMin, oldMax, newMin=-100, newMax=100):
    # Figure out how 'wide' each range is
    oldRange = oldMax - oldMin
    newRange = newMax - newMin
    NewValue = (((value - oldMin) * newRange) / oldRange) + newMin
    return int(NewValue)

def updateColorRanges(chosenColor):
    # take circular roi into consideration
    H = np.multiply(roi[:, :, 0], cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (roiDiameter, roiDiameter)))
    S = np.multiply(roi[:, :, 1], cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (roiDiameter, roiDiameter)))
    V = np.multiply(roi[:, :, 2], cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (roiDiameter, roiDiameter)))

    colorRange[chosenColor]['lower'] = (int(np.min(H[np.nonzero(H)])), max(0, int(np.min(S[np.nonzero(S)])-20 )), max(0, int(np.min(V[np.nonzero(V)])-20)))
    colorRange[chosenColor]['upper'] = (int(np.max(H[np.nonzero(H)])), min(255, int(np.max(S[np.nonzero(S)])+20)), min(255, int(np.max(V[np.nonzero(V)])+20)))
    print("new color range {} {}".format(chosenColor, colorRange[chosenColor]))

usesPiCamera = False

if platform.system == 'Windows':
    usesPiCamera = False
elif platform.system == 'Linux':
    if os.uname()[4][:3] == 'arm':
        usesPiCamera = True
    else:
        usesPiCamera = False



# camera = PiCamera()
# camera.framerate = 60
cameraResolution = (640, 480)
# camera.resolution = cameraResolution

# # camera.awb_mode = 'tungsten'
# camera.vflip = camera.hflip = True
# camera.video_stabilization = True
# rawCapture = PiRGBArray(camera, size=cameraResolution)

# initialize the video stream and allow the cammera sensor to warmup
vs = VideoStream(usePiCamera=usesPiCamera, resolution=cameraResolution, framerate=30).start()
time.sleep(2.0)


colorTolerance = 3
paused = False
roiDiameter = 20
roiSize = (roiDiameter, roiDiameter) # roi size on the scaled down image (converted to HSV)

colorRange = {
    'red': {'lower': (144, 0, 0), 'upper': (170, 230, 230)}, 
    'green': {'lower': (89, 86, 6), 'upper': (109, 255, 255)}, 
    'blue' : {'lower': (106, 100, 50), 'upper': (115, 255, 255)}
}

circles = {
    'red': (-1, -1, 0), 'green': (-1, -1, 0), 'blue': (-1, -1, 0)
}

colors = {
    'red': (0, 0, 255), 'green': (0, 255, 0), 'blue': (255, 0, 0)
}

# experimental data - has to be changed for use with other wheeled platforms
leftLine = ((0, 380), (200, 210))
rightLine = ((639, 380), (439, 210))



# # initialize serial communication
ser = serial.Serial(port='COM5', baudrate=57600, timeout=0.05)

packet = "<configL, {}, {}, {}, {}>".format(leftLine[0][0], leftLine[0][1], 
                                            leftLine[1][0], leftLine[1][1])
print(packet)
ser.write(bytes(packet, 'utf-8'))
print("response: {}".format(ser.readline().decode('utf-8')))
packet = "<configR, {}, {}, {}, {}>".format(rightLine[0][0], rightLine[0][1], 
                                            rightLine[1][0], rightLine[1][1])
print(packet)
ser.write(bytes(packet, 'utf-8'))
print("response: {}".format(ser.readline().decode('utf-8')))


while True:
# for cameraFrame in camera.capture_continuous(rawCapture, format="bgr", use_video_port=True):
    loopStart = time.time()
    if not paused:

        frame = vs.read()
        if usesPiCamera is True:
            frame = cv2.flip(frame, flipCode=-1)
        else:
            frame = cv2.flip(frame, flipCode=1)
        height, width = frame.shape[0:2]
        scaleFactor = 4
        newWidth, newHeight = width//scaleFactor, height//scaleFactor

        resizedColor = cv2.resize(frame, (newWidth, newHeight), interpolation=cv2.INTER_CUBIC)
        resizedColor_blurred = cv2.GaussianBlur(resizedColor, (5, 5), 0)

        resizedHSV = cv2.cvtColor(resizedColor_blurred, cv2.COLOR_BGR2HSV)

        roi = resizedHSV[newHeight//2 - roiSize[0]//2 : newHeight //2 + roiSize[0]//2, newWidth//2 - roiSize[1]//2 : newWidth//2 + roiSize[1]//2, :]

        upscaledColor = cv2.resize(resizedColor, (width, height), interpolation=cv2.INTER_NEAREST)
        
        # draw ROI on upscaled image
        xROI, yROI = width//2 - roiSize[1]//2 * scaleFactor, height//2 - roiSize[0]//2 * scaleFactor
        cv2.circle(upscaledColor,(width//2, height//2), roiDiameter//2*scaleFactor, (0, 0, 0), thickness=3)

        # detect objects
        for color in colorRange:
            colorLower = colorRange[color]['lower']
            colorUpper = colorRange[color]['upper']

            colorLowerWithTolerance = (colorLower[0] - colorTolerance,) + colorLower[1:]
            colorUpperWithTolerance = (colorUpper[0] + colorTolerance,) + colorUpper[1:]

            
            mask = cv2.inRange(resizedHSV, colorLowerWithTolerance, colorUpperWithTolerance)
            
            # # EITHER
            # cv2.erode(mask, None, iterations=5)
            # cv2.dilate(mask, None, iterations=5)
            # OR  
            kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (5, 5))
            mask = cv2.morphologyEx(mask, cv2.MORPH_OPEN, kernel)
            mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, kernel)
            # mask = cv2.morphologyEx(mask, cv2.MORPH_GRADIENT, kernel)
            cv2.imshow(color, mask)
            
            (_,contours, hierarchy) = cv2.findContours(mask, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)
            circles[color] = (-1, -1, 0)
            if contours:
                largestContour = max(contours, key=cv2.contourArea)
                if cv2.contourArea(largestContour) > ((newWidth * newHeight)/800) :
                    (x, y), r = cv2.minEnclosingCircle(largestContour)
                    circles[color] = (int(x), int(y), int(r))
                
        # draw lines
        cv2.line(upscaledColor, leftLine[0], leftLine[1], colors['green'], thickness=5)
        cv2.line(upscaledColor, rightLine[0], rightLine[1], colors['red'], thickness=5)

        # draw objects
        for color in colorRange:
            x,y,r = circles[color]
            objectCenter = x*scaleFactor, y*scaleFactor
            cv2.circle(upscaledColor, objectCenter, r*scaleFactor, colors[color], thickness=2)


        # send data to arduino
        packet = "<ballPositions,"
        for color in colorRange:
            x,y,radius = circles[color]
            objectCenter = x*scaleFactor, y*scaleFactor
            packet += " {}, {}, {}, {},".format(color,objectCenter[0], objectCenter[1], radius*scaleFactor)

        packet += ">"
        print(packet)
        ser.write(bytes(packet, 'utf-8'))
        print("response: {}".format(ser.readall().decode('utf-8')))

        cv2.imshow("video", upscaledColor)
        # cv2.imshow("roi", cv2.resize(roi, (roiDiameter*scaleFactor, roiDiameter*scaleFactor)))

        # Show actual circular ROI 
        H = np.multiply(roi[:, :, 0], cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (roiDiameter, roiDiameter)))
        S = np.multiply(roi[:, :, 1], cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (roiDiameter, roiDiameter)))
        V = np.multiply(roi[:, :, 2], cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (roiDiameter, roiDiameter)))
        roiCircular = cv2.merge([H, S, V])
        cv2.imshow("roi circular", cv2.resize(roiCircular, (roiDiameter*scaleFactor, roiDiameter*scaleFactor)) )
        
        modTolerances = False

    # handle keys 
    key = cv2.waitKey(1) & 0xFF
    if key == ord('q'):
        break
   
    elif key == ord('r'):
        updateColorRanges('red')

    elif key == ord('g'):
        updateColorRanges('green')
    
    elif key == ord('b'):
        updateColorRanges('blue')

    elif key == ord('w'):
        colorTolerance = min(colorTolerance + 1, 50)
        print("New color range: {}".format(colorTolerance))
    elif key == ord('s'):
        colorTolerance = max(colorTolerance - 1, 0)
        print("New color range: {}".format(colorTolerance))
    elif key == ord('p'):
        paused = not paused
    elif key == ord('d'):
        # pause/unpause arduino camera movement
        # ser.write(bytes('d', 'utf-8'))
        pass

    # rawCapture.truncate(0)
    loopEnd = time.time()
    # print("loop execution took {:3.2f}ms".format((loopEnd - loopStart)*1000))
    
# cleanup
cv2.destroyAllWindows()
vs.stop()
