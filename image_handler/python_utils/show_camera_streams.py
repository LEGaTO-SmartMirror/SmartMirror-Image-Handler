#!/usr/bin/env python3
# -*- coding: utf-8 -*-


import cv2
import sys



def to_node(type, message):
	# convert to json and print (node helper will read from stdout)
	try:
		print(json.dumps({type: message}))
	except Exception:
		pass
	# stdout has to be flushed manually to prevent delays in the node helper communication
	sys.stdout.flush()


IMAGE_HEIGHT = 720
IMAGE_WIDTH = 1280
IMAGE_STREAM_PATH = "/dev/shm/camera_1m_720p"

cap = cv2.VideoCapture("shmsrc socket-path=" + str(IMAGE_STREAM_PATH) + " ! video/x-raw, format=BGR, width=" + str(IMAGE_WIDTH) + ", height=" + str(IMAGE_HEIGHT) + ", framerate=30/1 ! videoconvert ! video/x-raw, format=BGR ! appsink drop=true", cv2.CAP_GSTREAMER)

cv2.namedWindow("small image", cv2.WINDOW_NORMAL)

while True:

	ret, frame = cap.read()
	if ret is False:
		to_node("status", "ret was false..")
		continue


	cv2.imshow("small image", frame)
	cv2.waitKey(33)
