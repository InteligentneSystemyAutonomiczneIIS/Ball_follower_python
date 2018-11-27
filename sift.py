import cv2
import numpy as np

img = cv2.imread('cam.jpg')
img = cv2.resize(img, (320, 240))
gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)

sift = cv2.xfeatures2d.SIFT_create()
kp = sift.detect(gray, None)

img2 = cv2.drawKeypoints(gray, kp, outImage=np.array([]))

cv2.imwrite('sift_keypoints.jpg', img2)
