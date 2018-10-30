import time
import serial
from picamera.array import PiRGBArray
from picamera import PiCamera
import numpy as np
import cv2
# import numpy as np
# import typing


def translate(value, oldMin, oldMax, newMin=-100, newMax=100):
    # Figure out how 'wide' each range is
    oldRange = oldMax - oldMin
    newRange = newMax - newMin

    NewValue = (((value - oldMin) * newRange) / oldRange) + newMin

    return int(NewValue)


camera = PiCamera()
camera.framerate = 60
cameraResolution = (640, 480)
camera.resolution = cameraResolution

# camera.awb_mode = 'tungsten'
camera.vflip = camera.hflip = True
camera.video_stabilization = True
rawCapture = PiRGBArray(camera, size=cameraResolution)

# cap = cv2.VideoCapture(0)
blueLower = (0, 100, 50)
blueUpper = (100, 255, 255)
colorTolerance = 10
paused = False
roiSize = (6, 6) # roi size on the scaled down image (converted to HSV)


# initialize serial communication
ser = serial.Serial(port='/dev/ttyACM0', baudrate=57600, timeout=0.05)

# while True:
for cameraFrame in camera.capture_continuous(rawCapture, format="bgr", use_video_port=True):
    loopStart = time.time()
    if not paused:
        # ret, frame = cap.read()
        # frame = cv2.flip(frame, flipCode=-1)
        frame = cameraFrame.array
        
        height, width = frame.shape[0:2]
        scaleFactor = 4
        newWidth, newHeight = width//scaleFactor, height//scaleFactor


        resizedColor = cv2.resize(frame, (newWidth, newHeight), interpolation=cv2.INTER_CUBIC)
        resizedColor_blurred = cv2.GaussianBlur(resizedColor, (5, 5), 0)

        # resizedHSV = cv2.cvtColor(resizedColor, cv2.COLOR_BGR2HSV)
        resizedHSV = cv2.cvtColor(resizedColor_blurred, cv2.COLOR_BGR2HSV)

        roi = resizedHSV[newHeight//2 - roiSize[0]//2 : newHeight //2 + roiSize[0]//2, newWidth//2 - roiSize[1]//2 : newWidth//2 + roiSize[1]//2, :]
        # roi = resizedHSV[10*newHeight//20 : 12*newHeight//20, 10*newWidth//20 : 12*newWidth // 20, :]
        
        blueLowerWithTolerance = (blueLower[0] - colorTolerance,) + blueLower[1:]
        blueUpperWithTolerance = (blueUpper[0] + colorTolerance,) + blueUpper[1:]

        mask = cv2.inRange(resizedHSV, blueLowerWithTolerance, blueUpperWithTolerance)
        cv2.erode(mask, None, iterations=5)
        cv2.dilate(mask, None, iterations=5)

        (_,contours, hierarchy) = cv2.findContours(mask, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)

        boundingBoxes = []
        biggestObject_BoundingBox = None
        biggestObjectMiddle = None
        if contours:
            largestContour = max(contours, key=cv2.contourArea)
            biggestObject_BoundingBox = cv2.boundingRect(largestContour)
            
            for i, contour in enumerate(contours):
                area = cv2.contourArea(contour)
                if area > ((newWidth * newHeight)/256):
                    x,y,w,h = cv2.boundingRect(contour)
                    boundingBoxes.append((x,y,w,h))
                    # print("Object {}: ({},{}); {}x{}; area: {}".format(i, x,y,w,h, area))
        else:
            pass

        upscaledColor = cv2.resize(resizedColor, (width, height), interpolation=cv2.INTER_NEAREST)
        # draw ROI on upscaled image
        xROI, yROI = width//2 - roiSize[1]//2 * scaleFactor, height//2 - roiSize[0]//2 * scaleFactor
        cv2.rectangle(upscaledColor, (xROI, yROI), (xROI + roiSize[0]*scaleFactor, yROI + roiSize[1]*scaleFactor), (0, 0, 0), thickness=3)

        for boundingBox in boundingBoxes:
            x,y,w,h = boundingBox
            cv2.rectangle(resizedColor, (x, y), (x+w, y+h), (255, 255, 0), thickness=1)
            cv2.rectangle(upscaledColor, (x*scaleFactor, y*scaleFactor),
                        ((x+w)*scaleFactor, (y+h)*scaleFactor), (255, 255, 0), thickness=2)
        
        if biggestObject_BoundingBox:
            x, y, w, h = biggestObject_BoundingBox
            biggestObjectMiddle = ((x+ w//2)*scaleFactor, (y + h//2)*scaleFactor)
            cv2.rectangle(resizedColor, (x, y), (x+w, y+h), (0, 0, 255), thickness=2)
            cv2.rectangle(upscaledColor, (x*scaleFactor, y*scaleFactor),
                            ((x+w)*scaleFactor, (y+h)*scaleFactor), (0, 0, 255), thickness=3)
            cv2.circle(upscaledColor, biggestObjectMiddle, 2, (255, 0, 0), thickness=2)
            screenMiddle = width//2, height//2
            distanceVector = tuple(map(lambda x, y: x - y, biggestObjectMiddle, screenMiddle))
            # print("Vector: {}".format(distanceVector))
            scaled = (translate(distanceVector[0], -width//2, width//2), translate(distanceVector[1], -height//2, height//2) )
            # print("Vector scaled: {}".format(scaled))
            cv2.line(upscaledColor, screenMiddle, biggestObjectMiddle, (0, 0, 255))
            yaw = 'y {}\n'.format(scaled[0])
            b_yaw = bytes(yaw, 'utf-8') # or 'ascii'
            pitch = 'p {}\n'.format(scaled[1])
            b_pitch = bytes(pitch, 'utf-8') # or 'ascii'
            ser.write(b_yaw)
            # print(ser.read_all())
            ser.write(b_pitch)
            # print(ser.read_all())
            

        cv2.imshow("video", upscaledColor)
        cv2.imshow("roi", roi)
        cv2.imshow("mask", mask)

        modTolerances = False

    # handle keys 
    key = cv2.waitKey(1) & 0xFF
    if key == ord('q'):
        break
    elif key == ord('a'):
        avg_h = 0
        avg_s = 0
        avg_v = 0
        i = 0
        for _, row in enumerate(roi):
            avg = np.average(row, 0)
            avg_h += avg[0]
            avg_s += avg[1]
            avg_v += avg[2]
            i+=1

        avg_h /= i
        avg_s /= i
        avg_v /= i
        print("HUE:{}, SAT:{}, VAL:{}".format(avg_h, avg_s, avg_v))
        blueLower = (max(0,avg_h), max(0, avg_s - 50), max(0,avg_v - 50))
        blueUpper = (min (255, avg_h), min(255, avg_s + 50), min(255, avg_v + 50))
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
        ser.write(bytes('d', 'utf-8'))
    
    rawCapture.truncate(0)
    loopEnd= time.time()
    print("loop execution took {:3.2f}ms".format((loopEnd - loopStart)*1000))
    
cv2.destroyAllWindows()
