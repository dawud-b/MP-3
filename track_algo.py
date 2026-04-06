import numpy as np
import cv2
import matplotlib.pyplot as plt
import time


def contour_center_if_circular(cnt, circular_threshold=0.7):
    area = cv2.contourArea(cnt)
    if area < 100: return None
    perimeter = cv2.arcLength(cnt, True)
    if perimeter == 0: return None
    circularity = (4 * np.pi * area) / (perimeter ** 2)
    if circularity <= circular_threshold: return None
    M = cv2.moments(cnt)
    if M['m00'] == 0: return None
    return (M['m10'] / M['m00'], M['m01'] / M['m00'])

frame = np.fromfile("images/BIMG_010.BIN", dtype=np.uint16).reshape((1080, 1920))

start = time.perf_counter_ns()

# view it as strided image to reduce resolution and only care about Y value of YCbCr
# in reality, this likely does a index calculation whenever frame_strided is indexed
# so in c++ implementation, just do a smart copy to a buffer from the frame buffer that only stores Y while also downscaling resolution
frame_u8 = frame.view(np.uint8)
frame_Ys_u8 = frame_u8[:, 0::2]
frame_strided = frame_Ys_u8[::4, ::4]

blurred_img = cv2.GaussianBlur(frame_strided, (3, 3), 1)
_, thresh = cv2.threshold(blurred_img, 0, 255, cv2.THRESH_BINARY + cv2.THRESH_OTSU)

# plt.imshow(thresh)
# plt.show()

# RETR_TREE captures the hierarchy which naturally fits with our goal of finding nested rings (what our bullseye looks like)
cnts, hierarchy = cv2.findContours(thresh, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)
hierarchy = hierarchy[0]


props = []
for cnt in cnts:
    circular_center = contour_center_if_circular(cnt)
    props.append(circular_center)


def chain_depth(i):
    if i == -1 or props[i] is None:
        return 0
    return 1 + chain_depth(hierarchy[i][2])

idx_with_top_depth = -1
top_depth = 0
for i in range(len(cnts)):
    if props[i] is None:
        continue
    depth = chain_depth(i)
    # print(f"depth: {depth} and {props[i]}")
    if depth > top_depth:
        top_depth = depth
        idx_with_top_depth = i

# typically we get a depth of 7
center = None
if top_depth >= 4: center = props[idx_with_top_depth]

end = time.perf_counter_ns()
print(f"{(end - start) * 10.0 ** -6} ms")

print(f"depth: {top_depth}")
print(f"center: {center}")
# it may be more accurate to use the center of the most nested inner ring, but there doesn't seem to be much difference between the outmost ring center and the innermost ring center


plt.imshow(blurred_img, cmap='gray', vmin=0, vmax=255)
plt.show()

