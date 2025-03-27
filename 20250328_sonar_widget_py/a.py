import sys
import numpy as np
from PySide2.QtWidgets import QApplication, QWidget
from PySide2.QtCore import Qt, QTimer, QPoint, QRectF
from PySide2.QtGui import QPainter, QColor, QImage, QFont
import math


def interpolate_color(val, min_val, max_val, min_color, max_color):
    ratio = (val - min_val) / max(1, (max_val - min_val))
    ratio = max(0.0, min(1.0, ratio))
    r = int(min_color.red() * (1 - ratio) + max_color.red() * ratio)
    g = int(min_color.green() * (1 - ratio) + max_color.green() * ratio)
    b = int(min_color.blue() * (1 - ratio) + max_color.blue() * ratio)
    return QColor(r, g, b)


class SonarWidget(QWidget):
    def __init__(self, nRange=1024, nBeam=512, nSwat=120, bit_depth=8, min_intensity=0, max_intensity=255):
        super().__init__()
        self.nRange = nRange
        self.nBeam = nBeam
        self.nSwat = nSwat  # in degrees
        self.bit_depth = bit_depth
        self.min_intensity = min_intensity
        self.max_intensity = max_intensity
        self.min_color = QColor(255, 255, 0)  # 黄色
        self.max_color = QColor(0, 0, 0)      # 黒
        self.setMinimumSize(600, 600)
        self.image = QImage(self.size(), QImage.Format_RGB32)
        self.image.fill(Qt.black)
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.update_data)
        self.timer.start(100)  # update every 100 ms

    def update_data(self):
        # ダミーデータ生成（ランダム）
        if self.bit_depth == 8:
            data = np.random.randint(0, 256, (self.nBeam, self.nRange), dtype=np.uint8)
        else:
            data = np.random.randint(0, 65536, (self.nBeam, self.nRange), dtype=np.uint16)
        self.update_image(data)
        self.update()

    def update_image(self, data):
        """センサデータ（2D numpy array）を扇形のQImageに変換"""
        self.image.fill(Qt.black)
        center = self.rect().center()
        painter = QPainter(self.image)
        painter.setRenderHint(QPainter.Antialiasing, False)

        angle_start = -self.nSwat / 2
        angle_step = self.nSwat / self.nBeam

        scale = min(self.width(), self.height()) / (2 * self.nRange)

        for i in range(self.nBeam):
            angle_deg = angle_start + i * angle_step
            angle_rad = math.radians(angle_deg)

            for j in range(self.nRange):
                strength = data[i, j]
                if self.bit_depth == 16:
                    strength = strength * 255 // 65535  # normalize to 0–255

                color_val = max(0, min(255, (strength - self.min_intensity) * 255 //
                                (self.max_intensity - self.min_intensity + 1)))
                color = QColor(color_val, color_val, color_val)  # grayscale

                r = j * scale
                x = center.x() + r * math.sin(angle_rad)
                y = center.y() - r * math.cos(angle_rad)

                color_val = max(0, min(255, (strength - self.min_intensity) * 255 //
                                (self.max_intensity - self.min_intensity + 1)))
                color = interpolate_color(color_val, 0, 255, self.min_color, self.max_color)
                self.image.setPixel(int(x), int(y), color.rgb())

        painter.end()

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.drawImage(0, 0, self.image)

        center = self.rect().center()
        radius_max = min(self.width(), self.height()) / 2
        scale = radius_max / self.nRange

        # --- 距離目盛の円弧を描画 ---
        step = self.nRange // 5  # 目盛り間隔（例：1024なら5分割）
        for r in range(step, self.nRange + 1, step):
            radius = r * scale
            rect = QRectF(center.x() - radius, center.y() - radius, 2 * radius, 2 * radius)
            start_angle = int((-self.nSwat / 2 + 90) * 16)  # 上向き起点に調整
            span_angle = int(self.nSwat * 16)
            painter.setPen(Qt.gray)
            painter.drawArc(rect, start_angle, span_angle)

            # 円弧の左右端に文字（距離）を表示
            angle_left = -self.nSwat / 2
            angle_right = self.nSwat / 2
            for angle in [angle_left, angle_right]:
                angle_rad = math.radians(angle)
                x = center.x() + radius * math.sin(angle_rad)
                y = center.y() - radius * math.cos(angle_rad)
                painter.drawText(int(x), int(y), f"{r}m")

        # --- 扇形の放射状ライン（角度目盛） ---
        for i in range(5):
            angle = -self.nSwat / 2 + i * (self.nSwat / 4)
            angle_rad = math.radians(angle)
            x = center.x() + radius_max * math.sin(angle_rad)
            y = center.y() - radius_max * math.cos(angle_rad)
            painter.setPen(QColor(128, 128, 128))  # background line
            painter.drawLine(center.x(), center.y(), int(x), int(y))
            painter.setPen(Qt.white)               # foreground line
            painter.drawLine(center.x(), center.y(), int(x), int(y))

        # --- 距離目盛の円弧 ---
        for r in range(step, self.nRange + 1, step):
            radius = r * scale
            rect = QRectF(center.x() - radius, center.y() - radius, 2 * radius, 2 * radius)
            start_angle = int((-self.nSwat / 2 + 90) * 16)
            span_angle = int(self.nSwat * 16)

            painter.setPen(QColor(128, 128, 128))  # background arc
            painter.drawArc(rect, start_angle, span_angle)
            painter.setPen(Qt.white)               # foreground arc
            painter.drawArc(rect, start_angle, span_angle)

            # 数値の描画（両端）
            for angle in [-self.nSwat / 2, self.nSwat / 2]:
                angle_rad = math.radians(angle)
                x = center.x() + radius * math.sin(angle_rad)
                y = center.y() - radius * math.cos(angle_rad)
                painter.setPen(Qt.white)
                painter.drawText(int(x), int(y), f"{r}m")


if __name__ == "__main__":
    app = QApplication(sys.argv)
    widget = SonarWidget(
        nRange=1024,
        nBeam=512,
        nSwat=120,
        bit_depth=8,
        min_intensity=0,
        max_intensity=255
    )
    widget.show()
    sys.exit(app.exec_())
