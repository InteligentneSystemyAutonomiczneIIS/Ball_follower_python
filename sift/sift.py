import time
import cv2
import numpy as np
from imutils.video import VideoStream

usesPiCamera = True 
cameraResolution = (640, 480)
framerate = 60

img = cv2.imread('cam.jpg')
img = cv2.resize(img, (320, 240))
reference_gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)

# SIFT - patented
# sift = cv2.xfeatures2d.SIFT_create()
# kp = sift.detect(gray, None)

# img2 = cv2.drawKeypoints(gray, kp, outImage=np.array([]))

# cv2.imwrite('sift_keypoints.jpg', img2)


orb = cv2.ORB_create(nfeatures=1000)
keypoints_orig, descriptors_orig = orb.detectAndCompute(reference_gray, None)
img = cv2.drawKeypoints(reference_gray, keypoints_orig, outImage=np.array([]))


cv2.imshow('reference', img)


vs = VideoStream(usePiCamera=usesPiCamera, resolution=cameraResolution, framerate=framerate).start()
time.sleep(1.0)

while True:
    loopStart = time.time()
    frame = vs.read()
    height, width = frame.shape[0:2]
    scaleFactor = 2
    newWidth, newHeight = width//scaleFactor, height//scaleFactor
    resizedColor = cv2.resize(frame, (newWidth, newHeight), interpolation=cv2.INTER_CUBIC)
    gray = cv2.cvtColor(resizedColor, cv2.COLOR_BGR2GRAY)


    orb = cv2.ORB_create(nfeatures=500)
    keypoints, descriptors = orb.detectAndCompute(gray, None)
    img2 = cv2.drawKeypoints(gray, keypoints, outImage=np.array([]))


    bf = cv2.BFMatcher(cv2.NORM_HAMMING2, crossCheck=True)
    matches = bf.match(descriptors_orig, descriptors)
    print(len(matches))
    matches = sorted(matches, key=lambda x: x.distance)

    matching_result = cv2.drawMatches(reference_gray, keypoints_orig, gray, keypoints, matches[:20], None, flags=2)

    cv2.imshow("video", img2)
    cv2.imshow("Matching result", matching_result)

    key = cv2.waitKey(1)
    if key == ord('q'):
        break

    loopEnd = time.time()
    print("loop execution took {:3.2f}ms".format((loopEnd - loopStart)*1000))

cv2.destroyAllWindows()
cv2.imwrite('orb_keypoints.jpg', img2)
