import numpy as np
import cv2
import math

# パラメータ
width = 256
height = 1024
fps = 30
frames = 900
swat = 120
range_m = 10.0
pillar_angle_deg = -30
pillar_distance_m = 3.0
pillar_radius_m = 0.1

# 角度と距離のマップ作成（1回だけ）
i_idx = np.arange(width)
j_idx = np.arange(height)
beam_angles_deg = (i_idx / width) * swat - (swat / 2)
beam_angles_rad = np.radians(beam_angles_deg)
distances_m = (j_idx / height) * range_m

# 座標マップ作成
x_map = np.outer(distances_m, np.cos(beam_angles_rad))  # height x width
y_map = np.outer(distances_m, np.sin(beam_angles_rad))  # height x width

# 円柱の中心座標
cx = pillar_distance_m * math.cos(math.radians(pillar_angle_deg))
cy = pillar_distance_m * math.sin(math.radians(pillar_angle_deg))

# 中心からの距離マップ
dist_map = np.sqrt((x_map - cx)**2 + (y_map - cy)**2)

# 距離が柱の半径以内なら反射（白）
frame = np.zeros((height, width), dtype=np.uint8)
frame[dist_map <= pillar_radius_m] = 255

# 動画作成
fourcc = cv2.VideoWriter_fourcc(*'X264')
out = cv2.VideoWriter('sonar_simulated_fast.mkv', fourcc, fps, (width, height), isColor=False)

for _ in range(frames):
    out.write(frame)

out.release()
print("✔️ 高速版：動画ファイル 'sonar_simulated_fast.mkv' を出力しました。")

