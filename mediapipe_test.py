import mediapipe as mp
from mediapipe.tasks import python
from mediapipe.tasks.python import vision
from mediapipe.framework.formats import landmark_pb2
from mediapipe import solutions

import cv2
import time
import numpy as np

model_path = 'C:\\Users\\james\\Downloads\\pose_landmarker_full.task'

BaseOptions = mp.tasks.BaseOptions
PoseLandmarker = mp.tasks.vision.PoseLandmarker
PoseLandmarkerOptions = mp.tasks.vision.PoseLandmarkerOptions
PoseLandmarkerResult = mp.tasks.vision.PoseLandmarkerResult
VisionRunningMode = mp.tasks.vision.RunningMode

def draw_landmarks_on_image(rgb_image, detection_result):
    pose_landmarks_list = detection_result.pose_landmarks
    annotated_image = np.copy(rgb_image)

    # Loop through the detected poses to visualize.
    for idx in range(len(pose_landmarks_list)):
        pose_landmarks = pose_landmarks_list[idx]

        # Draw the pose landmarks.
        pose_landmarks_proto = landmark_pb2.NormalizedLandmarkList()
        pose_landmarks_proto.landmark.extend([
            landmark_pb2.NormalizedLandmark(x=landmark.x, y=landmark.y, z=landmark.z) for landmark in pose_landmarks
        ])
        solutions.drawing_utils.draw_landmarks(
            annotated_image,
            pose_landmarks_proto,
            solutions.pose.POSE_CONNECTIONS,
            solutions.drawing_styles.get_default_pose_landmarks_style())
    print("Returning Image")
    return annotated_image

image = None

# Create a pose landmarker instance with the live stream mode:
def print_result(result: PoseLandmarkerResult, output_image: mp.Image, timestamp_ms: int):
    print(f'pose received: {result}', output_image.numpy_view())
    image = output_image.numpy_view()
    if len(result.pose_landmarks) > 0:
        image = draw_landmarks_on_image(image, result)
        cv2.imwrite(f"C:\\Users\\james\\Downloads\\test_img\\test_{timestamp_ms}.png", image)
        print("Annotated image", image)
    # cv2.imshow(annotated_image)

options = PoseLandmarkerOptions(
    base_options=BaseOptions(model_asset_path=model_path),
    running_mode=VisionRunningMode.LIVE_STREAM,
    result_callback=print_result)


cam = cv2.VideoCapture(0)
cam_not_available = cam is None or not cam.isOpened()
if cam_not_available:
    print('Warning: unable to open video source: 0')
    cam = cv2.VideoCapture(1)
    cam_not_available = cam is None or not cam.isOpened()
    if cam_not_available:
       print('Warning: unable to open video source: 1')
       exit()

with PoseLandmarker.create_from_options(options) as landmarker:
    while True:
        print("Getting webcam image")
        check, frame = cam.read()

        mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=frame)
        print("Passing to pose detector")
        landmarker.detect_async(mp_image, int(time.time()*1000))
        time.sleep(10)
        # if image is not None:
        #     print("image exists")
        #     cv2.imshow(image)
        # else:
        #     print("image does not exist")

    cam.release()
    cv2.destroyAllWindows()
print("Impossible")
