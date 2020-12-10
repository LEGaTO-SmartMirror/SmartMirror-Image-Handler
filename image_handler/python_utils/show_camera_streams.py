#!/usr/bin/env python3
# -*- coding: utf-8 -*-


import cv2
import sys
import time


def to_node(type, message):
	# convert to json and print (node helper will read from stdout)
	try:
		print(json.dumps({type: message}))
	except Exception:
		pass
	# stdout has to be flushed manually to prevent delays in the node helper communication
	sys.stdout.flush()


IMAGE_HEIGHT = 416 #1080
IMAGE_WIDTH = 416 #1920
IMAGE_STREAM_PATH = "/dev/shm/camera_small"

FPS = int (sys.argv[1])


cap = cv2.VideoCapture("shmsrc socket-path=" + str(IMAGE_STREAM_PATH) + " ! video/x-raw, format=BGR, width=" + str(IMAGE_WIDTH) + ", height=" + str(IMAGE_HEIGHT) + ", framerate=30/1 ! queue max-size-buffers=5  leaky=2 ! videoconvert ! video/x-raw, format=BGR ! appsink drop=true", cv2.CAP_GSTREAMER)

# ! queue max-size-time=100 leaky=2 !


cv2.namedWindow("small image - " + str(FPS) + " FPS", cv2.WINDOW_NORMAL)




while True:

	start_time = time.time()

	ret, frame = cap.read()
	if ret is False:
		to_node("status", "ret was false..")
		continue


	cv2.imshow("small image - " + str(FPS) + " FPS", frame)
	cv2.waitKey(33)

	delta = time.time() - start_time


	if (1.0 / FPS) - delta > 0:
		time.sleep((1.0 / FPS) - delta)
