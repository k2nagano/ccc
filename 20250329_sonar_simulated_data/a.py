import numpy as np
import cv2
import math

# パラメータ設定
width = 256               # beam数
height = 1024             # range分割数
fps = 30
frames = 900              # 30秒
swat = 120                # スキャン角度（度）
range_m = 10.0            # 最大レンジ（メートル）
pillar_angle_deg = -30    # 左30度（左がマイナス）
pillar_distance_m = 3.0
pillar_radius_m = 0.1     # 半径10cm

# 角度 → ビーム番号変換
def angle_to_beam(angle_deg):
    return int((angle_deg + swat / 2) / swat * width)

# 距離 → range index変換
def distance_to_index(distance_m):
    return int(distance_m / range_m * height)

# 円をソナー画像上に描画
def draw_pillar(image):
    for i in range(width):
        for j in range(height):
            angle = (i / width) * swat - (swat / 2)
            beam_rad = math.radians(angle)
            distance = (j / height) * range_m
            x = distance * math.cos(beam_rad)
            y = distance * math.sin(beam_rad)

            # 円柱に近ければ強反射（白）
            dx = x - pillar_distance_m * math.cos(math.radians(pillar_angle_deg))
            dy = y - pillar_distance_m * math.sin(math.radians(pillar_angle_deg))
            dist = np.sqrt(dx**2 + dy**2)
            if dist <= pillar_radius_m:
                image[j, i] = 255  # 強反射
    return image

# 動画ライター初期化
fourcc = cv2.VideoWriter_fourcc(*'X264')
out = cv2.VideoWriter('sonar_simulated.mkv', fourcc, fps, (width, height), isColor=False)

# フレーム生成
for _ in range(frames):
    frame = np.zeros((height, width), dtype=np.uint8)
    frame = draw_pillar(frame)
    out.write(frame)

out.release()
print("動画ファイル 'sonar_simulated.mkv' を出力しました。")

