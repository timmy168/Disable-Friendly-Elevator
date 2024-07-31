import cv2
import mediapipe as mp
import math
import os
import subprocess
import time

mp_drawing = mp.solutions.drawing_utils
mp_drawing_styles = mp.solutions.drawing_styles
mp_hands = mp.solutions.hands


def vector_2d_angle(v1, v2):
    v1_x = v1[0]
    v1_y = v1[1]
    v2_x = v2[0]
    v2_y = v2[1]
    try:
        angle_ = math.degrees(math.acos((v1_x * v2_x + v1_y * v2_y) / (((v1_x * 2 + v1_y * 2) * 0.5) * ((v2_x * 2 + v2_y * 2) * 0.5))))
    except:
        angle_ = 180
    return angle_


def hand_angle(hand_):
    angle_list = []
    angle_ = vector_2d_angle(
        ((int(hand_[0][0]) - int(hand_[2][0])), (int(hand_[0][1]) - int(hand_[2][1]))),
        ((int(hand_[3][0]) - int(hand_[4][0])), (int(hand_[3][1]) - int(hand_[4][1])))
    )
    angle_list.append(angle_)
    angle_ = vector_2d_angle(
        ((int(hand_[0][0]) - int(hand_[6][0])), (int(hand_[0][1]) - int(hand_[6][1]))),
        ((int(hand_[7][0]) - int(hand_[8][0])), (int(hand_[7][1]) - int(hand_[8][1])))
    )
    angle_list.append(angle_)
    angle_ = vector_2d_angle(
        ((int(hand_[0][0]) - int(hand_[10][0])), (int(hand_[0][1]) - int(hand_[10][1]))),
        ((int(hand_[11][0]) - int(hand_[12][0])), (int(hand_[11][1]) - int(hand_[12][1])))
    )
    angle_list.append(angle_)
    angle_ = vector_2d_angle(
        ((int(hand_[0][0]) - int(hand_[14][0])), (int(hand_[0][1]) - int(hand_[14][1]))),
        ((int(hand_[15][0]) - int(hand_[16][0])), (int(hand_[15][1]) - int(hand_[16][1])))
    )
    angle_list.append(angle_)
    angle_ = vector_2d_angle(
        ((int(hand_[0][0]) - int(hand_[18][0])), (int(hand_[0][1]) - int(hand_[18][1]))),
        ((int(hand_[19][0]) - int(hand_[20][0])), (int(hand_[19][1]) - int(hand_[20][1])))
    )
    angle_list.append(angle_)
    return angle_list


def hand_pos(finger_angle, finger_points):
    thumb_tip = finger_points[4]  
    thumb_base = finger_points[0]  

    if thumb_tip[1] < thumb_base[1]:  
        return 'up'
    else:
        return 'down'

cap = cv2.VideoCapture(0)            
fontFace = cv2.FONT_HERSHEY_SIMPLEX  
lineType = cv2.LINE_AA               


up_count = 0
down_count = 0


with mp_hands.Hands(
    model_complexity=1,
    min_detection_confidence=0.3,
    min_tracking_confidence=0.3) as hands:

    if not cap.isOpened():
        print("Cannot open camera")
        exit()
    w, h = 800, 600                                

    while True:
        ret, img = cap.read()
        img = cv2.resize(img, (w,h))         
        if not ret:
            print("Cannot receive frame")
            break
        img2 = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)  
        results = hands.process(img2)                
        if results.multi_hand_landmarks:
            for hand_landmarks in results.multi_hand_landmarks:
                finger_points = []                   
                for i in hand_landmarks.landmark:
                    x = i.x*w
                    y = i.y*h
                    finger_points.append((x,y))
                if finger_points:
                    finger_angle = hand_angle(finger_points) 
                    text = hand_pos(finger_angle, finger_points)  
                    if text == 'up':
                        up_count += 1
                    else:
                        down_count += 1
                    
                    cv2.putText(img, f"Up: Confidence:{up_count}%, Down: Confidence:{down_count}%", (30, 30), fontFace, 1, (255, 255, 255), 2, lineType)
                    
                    if up_count > 100 or down_count > 100:
                        path = "/home/timmy/Elevator_project/elevator_simulator"  
                        direction = 'up' if up_count > 100 else 'down'
                        command = f"./client 192.168.50.171 8888<<EOF\nname Disable1 type 1 loc 3 des 9 direc {direction}\nEOF"
                        subprocess.run(command, shell=True, cwd=path)
                        break   

        cv2.imshow('oxxostudio', img)
        if cv2.waitKey(5) == ord('q'):
            break

cap.release()
cv2.destroyAllWindows()
