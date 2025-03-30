import numpy as np
import cv2
import math

# パラメータ
width = 256
height = 1024
range_m = 10.0
swat = 120

# 魚の位置（例）：正面方向3m
fish_angle_deg = 0
fish_distance = 3.0

beam_angles_deg = np.linspace(-swat / 2, swat / 2, width)
beam_angles_rad = np.radians(beam_angles_deg)
range_bins = np.linspace(0, range_m, height)

x_map = np.outer(range_bins, np.cos(beam_angles_rad))
y_map = np.outer(range_bins, np.sin(beam_angles_rad))

fx = fish_distance * math.cos(math.radians(fish_angle_deg))
fy = fish_distance * math.sin(math.radians(fish_angle_deg))

dx = x_map - fx
dy = y_map - fy
dist_map = np.sqrt(dx ** 2 + dy ** 2)

frame = np.zeros((height, width), dtype=np.uint8)
frame[dist_map < 0.1] = 255

# C言語用に配列を出力
with open("sonar_frame_data.h", "w") as f:
    f.write("unsigned char sonar_frame[1024][256] = {\n")
    for row in frame:
        f.write("  {" + ", ".join(map(str, row)) + "},\n")
    f.write("};\n")

print("✔️ 1フレーム目を C言語配列形式で 'sonar_frame_data.h' に出力しました。")

