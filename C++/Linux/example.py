# please find `phonecam.*.so` in your build directory and copy it here

from phonecam import PhoneCamera
import cv2


pc = PhoneCamera(8888)
while pc.isOpened():
    succeed, im = pc.read()
    if not succeed:
        break
    print('timestamp:', pc.get(cv2.CAP_PROP_POS_MSEC))
    cv2.imshow('camera', im)
    cv2.waitKey(1)
