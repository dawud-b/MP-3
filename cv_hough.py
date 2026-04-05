import numpy as np
import cv2 as cv
import matplotlib.pyplot as plt

img = np.fromfile("C:\\Users\zfsalti\\488\MP-3\images\BIMG_010.BIN", dtype=np.uint16).reshape((1080, 1920))


raw_pixels = np.frombuffer(img, dtype=np.uint16)

y_channel = (raw_pixels & 0xff).astype(np.uint8)

new_img = y_channel.reshape((1080, 1920))


#blurred_img = cv.medianBlur(new_img, 21)
blurred_img = cv.GaussianBlur(new_img, (3, 3), 2)

circles = cv.HoughCircles(blurred_img, cv.HOUGH_GRADIENT, 1, 20, param1=50, param2=55, minRadius=0, maxRadius=150)

circles = np.uint16(np.around(circles))

for i in circles[0,:]:
    cv.circle(blurred_img, (i[0], i[1]), i[2], 0, 2)
    cv.circle(blurred_img, (i[0], i[1]), 2, 0, 2)

plt.imshow(blurred_img, cmap="gray", vmin=0, vmax=255)
plt.show()