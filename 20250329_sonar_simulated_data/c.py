import numpy as np
import cv2
import math

# パラメータ設定
width = 256
height = 1024
fps = 30
frames = 900
swat = 120  # スキャン角度
range_m = 10.0

# 魚の移動パラメータ
start_angle_deg = -30
end_angle_deg = 30
start_distance = 3.0
end_distance = 7.0

# ソナーのマッピング用
beam_angles_deg = np.linspace(-swat / 2, swat / 2, width)
beam_angles_rad = np.radians(beam_angles_deg)
range_bins = np.linspace(0, range_m, height)

x_map = np.outer(range_bins, np.cos(beam_angles_rad))  # height x width
y_map = np.outer(range_bins, np.sin(beam_angles_rad))  # height x width

# 動画ライター初期化
fourcc = cv2.VideoWriter_fourcc(*'X264')
out = cv2.VideoWriter('sonar_simulated_fish.mkv', fourcc, fps, (width, height), isColor=False)

for frame_idx in range(frames):
    t = frame_idx / (frames - 1)
    angle = math.radians(start_angle_deg + (end_angle_deg - start_angle_deg) * t)
    distance = start_distance + (end_distance - start_distance) * t

    fx = distance * math.cos(angle)
    fy = distance * math.sin(angle)

    dx = x_map - fx
    dy = y_map - fy
    dist_map = np.sqrt(dx ** 2 + dy ** 2)

    # 魚の近くにある点を白くする（0.05m以内）
    frame = np.zeros((height, width), dtype=np.uint8)
    frame[dist_map < 0.05] = 255

    out.write(frame)

out.release()
print("✔️ 魚の移動を描いたMKVファイル 'sonar_simulated_fish.mkv' を出力しました。")
