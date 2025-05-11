import sys
import os
import re
import cv2
import numpy as np
from datetime import datetime
import sys
import time
import argparse
from datetime import datetime, timedelta
from PySide2.QtWidgets import (
    QApplication, QWidget, QVBoxLayout, QHBoxLayout, QColorDialog,
    QPushButton, QDoubleSpinBox, QLineEdit, QSlider, QFileDialog, QLabel, QStyle
)
from PySide2.QtCore import QThread, Signal, Slot, Qt, QRectF, QPointF, QTimer
from PySide2.QtGui import QPainter, QColor, QFont, QImage, QPen, QPainterPath


def parse_args():
    parser = argparse.ArgumentParser(description="Sonar MKV Player")
    parser.add_argument('-m', '--mkv', type=str, help='Sonar data MKV file path')
    parser.add_argument('-s', '--swath', type=float, default=120.0, help='Fan opening angle [deg]')
    parser.add_argument('-r', '--range', type=float, default=30.0, help='Sonar exploration range (m)')
    parser.add_argument('-i', '--min-intensity', type=float, default=0.0, help='Minimum intensity for display')
    parser.add_argument('-x', '--max-intensity', type=float, default=255.0, help='Maximum intensity for display')
    return parser.parse_args()


def set_button_colors(button, bg_color: QColor):
    r, g, b = bg_color.red(), bg_color.green(), bg_color.blue()
    brightness = 0.299 * r + 0.587 * g + 0.114 * b
    text_color = '#FFFFFF' if brightness < 128 else '#000000'
    button.setStyleSheet(f"background-color: {bg_color.name()}; color: {text_color};")


class SonarThread(QThread):
    frame_ready    = Signal(np.ndarray, float)
    position_changed = Signal(int)

    def __init__(self, video_path=None, parent=None):
        super().__init__(parent)
        self.video_path = video_path
        self.capture = None
        self._running = False
        self._mode = 'play'  # 'play', 'pause', 'fast_forward', 'rewind'
        self._frame_idx = 0

    def set_file(self, path: str):
        self.video_path = path

    def run(self):
        # Open video and determine FPS
        self.capture = cv2.VideoCapture(self.video_path)
        fps = self.capture.get(cv2.CAP_PROP_FPS) or 30
        self._running = True

        while self._running:
            # Determine skip offset based on mode
            if self._mode == 'fast_forward':
                offset = 30
            elif self._mode == 'rewind':
                offset = -30
            elif self._mode == 'play':
                offset = 1
            else:  # pause
                self.msleep(100)
                continue

            # Seek by offset (0 <= frame < total)
            curr = int(self.capture.get(cv2.CAP_PROP_POS_FRAMES))
            total = int(self.capture.get(cv2.CAP_PROP_FRAME_COUNT))
            new_pos = min(max(curr + offset, 0), total - 1)
            self.capture.set(cv2.CAP_PROP_POS_FRAMES, new_pos)

            # Read one frame
            ret, frame = self.capture.read()
            if not ret:
                break

            self._frame_idx = new_pos
            self.position_changed.emit(new_pos)

            # Extract intensity and timestamp
            intensity = frame[..., 0] if frame.ndim == 3 else frame
            timestamp = self.capture.get(cv2.CAP_PROP_POS_MSEC) / 1000.0
            # Emit to UI
            self.frame_ready.emit(intensity, timestamp)

            # Sleep to pace playback (only in normal play)
            if self._mode == 'play':
                self.msleep(int(1000 / fps))

            if self._mode in ('play', 'fast_forward') and new_pos >= total - 1:
                self._mode = 'pause'
            if self._mode  == 'rewind' and new_pos <= 0:
                self._mode = 'pause'

    def stop(self):
        self._running = False
        self.wait()

    def play(self):
        self._mode = 'play'

    def pause(self):
        self._mode = 'pause'

    def fast_forward(self):
        self._mode = 'fast_forward'

    def rewind(self):
        self._mode = 'rewind'


class SonarWidget(QWidget):
    def __init__(self):
        super().__init__()
        self.frame = None
        self.base_timestamp = 0.0
        self.timestamp = 0.0
        self.swath = 120.0
        self.range = 30.0
        self.min_intensity = 0.0
        self.max_intensity = 255.0
        self.min_color = QColor(255, 255, 0)
        self.max_color = QColor(255, 0, 0)
        self.foreground_color = QColor(0, 0, 0)
        self.background_color = QColor(255, 255, 255)
        self.top_margin = 30
        self.center = None
        self.radius = 0
        self.image = None
        self.clip_path = None

        self.update_timer = QTimer(self)
        self.update_timer.setSingleShot(True)
        self.update_timer.timeout.connect(self.handle_update_timeout)

    def set_base_timestamp(self, base_ts):
        self.base_timestamp = base_ts
    def set_swath(self, swath):
        self.swath = swath
        self.update2()
    def set_range(self, range):
        self.range = range
        self.update2()
    def set_min_intensity(self, min_intensity):
        self.min_intensity = min_intensity
        self.update2()
    def set_max_intensity(self, max_intensity):
        self.max_intensity = max_intensity
        self.update2()
    def set_min_color(self, min_color):
        self.min_color = min_color
        self.update2()
    def set_max_color(self, max_color):
        self.max_color = max_color
        self.update2()
    def set_foreground_color(self, foreground_color):
        self.foreground_color = foreground_color
        self.update2()
    def set_background_color(self, background_color):
        self.background_color = background_color
        self.update2()
    def update2(self):
        self.update_image()
        self.update()

    @Slot(np.ndarray, float)
    def update_frame(self, frame: np.ndarray, timestamp: float):
        self.frame = frame
        self.timestamp = self.base_timestamp + timestamp
        if not self.update_timer.isActive():
            self.update_timer.start(200)

    def update_image(self):
        start_time = time.time()

        if self.frame is None:
            return

        # widget size
        w, h = self.width(), self.height()
        if w <= 0 or h <= 0:
            return

        # compute center and pixel‐radius so the fan fits in the widget
        cx, cy = w * 0.5, h
        radius = min(cx, h) * 0.9
        self.center = QPointF(cx, cy)
        self.radius = radius

        # prepare an empty ARGB image
        self.image = QImage(w, h, QImage.Format_ARGB32)
        self.image.fill(Qt.transparent)

        # build our mapping grid only once (you can cache this if you want speed)
        # meshgrid of destination pixel coords
        jj, ii = np.meshgrid(np.arange(w), np.arange(h))
        dx = jj - cx
        dy = cy - ii

        # polar coords in pixel‐space
        r = np.hypot(dx, dy)
        theta = np.arctan2(dx, dy)   # radians, 0 = straight up

        # limits of fan
        half_sw_rad = np.deg2rad(self.swath * 0.5)
        valid = (r <= radius) & (theta >= -half_sw_rad) & (theta <= half_sw_rad)

        # normalize into your frame’s index space
        # 1) unpack correctly
        n_range, n_beam = self.frame.shape   # rows=range, cols=beams

        # 2) compute map_x (columns) from theta → beam index
        map_x = ((theta + half_sw_rad) / (2*half_sw_rad) * (n_beam - 1)).astype(np.float32)

        # 3) compute map_y (rows) from r → range index
        map_y = (r / radius * (n_range - 1)).astype(np.float32)

        # remap with linear interpolation in C++
        intensity = cv2.remap(
            self.frame.astype(np.float32),
            map_x, map_y,
            interpolation=cv2.INTER_LINEAR,
            borderMode=cv2.BORDER_CONSTANT,
            borderValue=0
        )

        # clamp to your intensity window
        intensity = np.clip(intensity, self.min_intensity, self.max_intensity)
        norm = (intensity - self.min_intensity) / max(self.max_intensity - self.min_intensity, 1e-6)

        # build the color gradient
        min_rgb = np.array([self.min_color.red(),   self.min_color.green(),   self.min_color.blue()])
        max_rgb = np.array([self.max_color.red(),   self.max_color.green(),   self.max_color.blue()])
        color_array = (min_rgb + (max_rgb - min_rgb) * norm[..., None]).astype(np.uint8)

        # copy into the QImage buffer (BGRA layout)
        ptr = self.image.bits()
        arr = np.frombuffer(ptr, dtype=np.uint8).reshape((h, w, 4))
        arr[..., 0:3] = color_array[..., ::-1]       # B, G, R
        arr[..., 3]   = np.where(valid, 255, 0)      # alpha mask
        
        elapsed_ms = (time.time() - start_time) * 1000
        print(f"update_image elapsed time: {elapsed_ms:.2f} ms")

    def resizeEvent(self, event):
        if not self.update_timer.isActive():
            self.update_timer.start(200)

    def handle_update_timeout(self):
        self.update2()

    def paintEvent(self, event):
        if self.image is None:
            return

        start_time = time.time()

        painter = QPainter(self)
        painter.fillRect(self.rect(), self.background_color)
        painter.setRenderHint(QPainter.Antialiasing)

        if self.clip_path:
            painter.setClipPath(self.clip_path)

        if self.image:
            painter.drawImage(0, 0, self.image)

        painter.setClipping(False)

        # 円弧ガイドと目盛り描画
        pen = QPen(self.foreground_color)
        pen.setWidth(1)
        painter.setPen(pen)
        font = QFont()
        font.setPointSize(10)
        painter.setFont(font)

        # 目盛り間隔設定
        if self.range < 10:
            step = 1
        elif self.range < 20:
            step = 2
        elif self.range < 30:
            step = 5
        else:
            step = 10

        if not hasattr(self, 'center') or self.center is None:
            # fall back to computing it here
            cx, cy = self.width()*0.5, self.height()
            self.center = QPointF(cx, cy)
            self.radius = min(cx, self.height()) * 0.9

        for r_m in range(step, int(self.range)+1, step):
            r_pix = r_m / self.range * self.radius
            rect = QRectF(self.center.x() - r_pix, self.center.y() - r_pix, r_pix*2, r_pix*2)
            painter.drawArc(rect, (90 - self.swath/2)*16, self.swath*16)

            # テキスト描画
            for angle in [-self.swath/2, self.swath/2]:
                angle_rad = np.radians(angle)
                x = self.center.x() + r_pix * np.sin(angle_rad)

                y = self.center.y() - r_pix * np.cos(angle_rad)
                text = f"{r_m}m"
                fm = painter.fontMetrics()
                text_width = fm.horizontalAdvance(text)

                painter.save()
                painter.translate(x, y)
                painter.rotate(angle)
                if angle < 0:
                    painter.drawText(-text_width, 0, text)  # 左側は右端揃え
                else:
                    painter.drawText(0, 0, text)  # 右側は左端揃え
                painter.restore()

        # 広がり角度のガイド線
        if self.swath < 120:
            angles = [-self.swath/2, 0, self.swath/2]
        else:
            angles = [-self.swath/2, -60, -30, 0, 30, 60, self.swath/2]

        for angle in angles:
            angle_rad = np.radians(angle)
            x = self.center.x() + self.radius * np.sin(angle_rad)
            y = self.center.y() - self.radius * np.cos(angle_rad)
            painter.drawLine(self.center, QPointF(x, y))

        # Header: datetime (left), range (center), intensities (right)
        painter.setPen(self.foreground_color)
        painter.setFont(QFont('Arial', 10))
        # datetime
        dt = datetime.fromtimestamp(self.timestamp)
        dt_str = dt.strftime('%Y/%m/%d %H:%M:%S') + f'.{dt.microsecond//1000:03d}'
        painter.drawText(10, 20, dt_str)
        # range
        range_str = f"Range={self.range:.0f}m"
        fm = painter.fontMetrics()
        tw = fm.horizontalAdvance(range_str)
        w = self.width()
        painter.drawText((w - tw)/2, 20, range_str)
        # intensities
        min_str = f"MinIntensity={self.min_intensity:.0f}"
        max_str = f"MaxIntensity={self.max_intensity:.0f}"
        x_min = w - 10 - fm.horizontalAdvance(min_str)
        painter.drawText(x_min, 20, min_str)
        x_max = w - 10 - fm.horizontalAdvance(max_str)
        painter.drawText(x_max, 40, max_str)

        painter.end()

        elapsed_ms = (time.time() - start_time) * 1000
        print(f"paintEvent elapsed time: {elapsed_ms:.2f} ms")


class SonarPlayer(QWidget):
    def __init__(self, args):
        super().__init__()
        self.setWindowTitle("Sonar Player")
        self.setAcceptDrops(True)
        self.thread = SonarThread()
        self.widget = SonarWidget()
        # apply parameters
        self.widget.swath = args.swath
        self.widget.range = args.range
        self.widget.min_intensity = args.min_intensity
        self.widget.max_intensity = args.max_intensity

        # layout
        vlay = QVBoxLayout(self)
        vlay.addWidget(self.widget, stretch=3)
        # controls
        hlay = QHBoxLayout(); hlay.setContentsMargins(0,0,0,0); hlay.setSpacing(5)
        style = QApplication.style()
        icons = [style.standardIcon(QStyle.SP_MediaPlay), style.standardIcon(QStyle.SP_MediaPause), style.standardIcon(QStyle.SP_MediaStop),
                 style.standardIcon(QStyle.SP_MediaSeekForward), style.standardIcon(QStyle.SP_MediaSeekBackward)]
        self.start_btn = QPushButton(); self.start_btn.setIcon(icons[0]); self.start_btn.setFixedWidth(40)
        self.pause_btn = QPushButton(); self.pause_btn.setIcon(icons[1]); self.pause_btn.setFixedWidth(40)
        self.stop_btn = QPushButton(); self.stop_btn.setIcon(icons[2]); self.stop_btn.setFixedWidth(40)
        self.ff_btn = QPushButton(); self.ff_btn.setIcon(icons[3]); self.ff_btn.setFixedWidth(40)
        self.rew_btn = QPushButton(); self.rew_btn.setIcon(icons[4]); self.rew_btn.setFixedWidth(40)
        for btn in (self.start_btn, self.pause_btn, self.stop_btn, self.ff_btn, self.rew_btn):
            hlay.addWidget(btn)
        # spinboxes and labels
        self.swath_spin = QDoubleSpinBox(); self.swath_spin.setRange(1, 360); self.swath_spin.setValue(args.swath); self.swath_spin.setMaximumWidth(80)
        self.range_spin = QDoubleSpinBox(); self.range_spin.setRange(0, 100); self.range_spin.setValue(args.range); self.range_spin.setMaximumWidth(80)
        self.min_spin   = QDoubleSpinBox(); self.min_spin.setRange(0, 65535); self.min_spin.setValue(args.min_intensity); self.min_spin.setMaximumWidth(80)
        self.max_spin   = QDoubleSpinBox(); self.max_spin.setRange(0, 65535); self.max_spin.setValue(args.max_intensity); self.max_spin.setMaximumWidth(80)
        for label, spin in [("Swath[deg]",self.swath_spin),("Range[m]",self.range_spin),("MinIntensity",self.min_spin),("MaxIntensity",self.max_spin)]:
            lbl = QLabel(label); lbl.setAlignment(Qt.AlignRight|Qt.AlignVCenter)
            hlay.addWidget(lbl); hlay.addWidget(spin)
        vlay.addLayout(hlay)
        # color buttons
        col = QHBoxLayout(); col.setSpacing(5)
        self.fg_btn = QPushButton('Foreground'); self.bg_btn = QPushButton('Background')
        self.min_col_btn = QPushButton('MinIntensity'); self.max_col_btn = QPushButton('MaxIntensity')
        for b in (self.fg_btn, self.bg_btn, self.min_col_btn, self.max_col_btn): col.addWidget(b)
        vlay.addLayout(col)
        set_button_colors(self.fg_btn, self.widget.foreground_color)
        set_button_colors(self.bg_btn, self.widget.background_color)
        set_button_colors(self.min_col_btn, self.widget.min_color)
        set_button_colors(self.max_col_btn, self.widget.max_color)
        # slider and status
        self.slider = QSlider(Qt.Horizontal); vlay.addWidget(self.slider)
        self.line = QLineEdit(); self.line.setReadOnly(True)
        bg = self.palette().color(self.backgroundRole()).name(); self.line.setStyleSheet(f"background-color: {bg};")
        vlay.addWidget(self.line)
        # connections
        self.thread.frame_ready.connect(self.widget.update_frame)
        self.thread.position_changed.connect(self.update_slider)
        self.thread.finished.connect(self.on_finished)
        self.start_btn.clicked.connect(self.on_start)
        # self.play_btn.clicked.connect(self.thread.play)
        self.pause_btn.clicked.connect(self.thread.pause)
        self.stop_btn.clicked.connect(self.on_stop)
        self.ff_btn.clicked.connect(self.thread.fast_forward)
        self.rew_btn.clicked.connect(self.thread.rewind)
        self.swath_spin.valueChanged.connect(lambda v: setattr(self.widget,'swath',v) or self.widget.update2())
        self.range_spin.valueChanged.connect(lambda v: setattr(self.widget,'range',v) or self.widget.update2())
        self.min_spin.valueChanged.connect(lambda v: setattr(self.widget,'min_intensity',v) or self.widget.update2())
        self.max_spin.valueChanged.connect(lambda v: setattr(self.widget,'max_intensity',v) or self.widget.update2())
        self.fg_btn.clicked.connect(lambda:self.select_color('foreground_color',self.fg_btn) or self.widget.update2())
        self.bg_btn.clicked.connect(lambda:self.select_color('background_color',self.bg_btn) or self.widget.update2())
        self.min_col_btn.clicked.connect(lambda:self.select_color('min_color',self.min_col_btn) or self.widget.update2())
        self.max_col_btn.clicked.connect(lambda:self.select_color('max_color',self.max_col_btn) or self.widget.update2())
        self.slider.sliderMoved.connect(self.on_slider_moved)
        self.slider.sliderPressed.connect(self.thread.pause)

    def select_color(self, attr, button):
        col = getattr(self.widget, attr)
        c = QColorDialog.getColor(col, self)
        if c.isValid():
            setattr(self.widget, attr, c); set_button_colors(button, c)

    @Slot(int)
    def update_slider(self, frame_index):
        self.slider.blockSignals(True)
        self.slider.setValue(frame_index)
        self.slider.blockSignals(False)

    @Slot()
    def on_finished(self):
        self.slider.setValue(self.slider.maximum())

    def on_start(self):
        path = self.line.text().strip()
        if not path: return

        basename = os.path.basename(path)
        p1 = re.compile(r"(\d{4})-(\d{2})-(\d{2})_(\d{2})-(\d{2})-(\d{2})\.(\d{3})\.mkv$")
        p2 = re.compile(r"(\d{4})_(\d{2})_(\d{2})-(\d{2})_(\d{2})_(\d{2})\.(\d{3})\.mkv$")
        m = p1.search(basename) or p2.search(basename)
        if m:
            y, mo, d, h, mi, s, ms = m.groups()
            dt0 = datetime(int(y), int(mo), int(d), int(h), int(mi), int(s), int(ms) * 1000)
            self.widget.set_base_timestamp(dt0.timestamp())
        p1 = re.compile(r"(\d{4})-(\d{2})-(\d{2})_(\d{2})-(\d{2})-(\d{2})\.mkv$")
        p2 = re.compile(r"(\d{4})_(\d{2})_(\d{2})-(\d{2})_(\d{2})_(\d{2})\.mkv$")
        m = p1.search(basename) or p2.search(basename)
        if m:
            y, mo, d, h, mi, s = m.groups()
            dt0 = datetime(int(y), int(mo), int(d), int(h), int(mi), int(s))
            self.widget.set_base_timestamp(dt0.timestamp())

        if not self.thread.isRunning():
            cap = cv2.VideoCapture(path)
            total = int(cap.get(cv2.CAP_PROP_FRAME_COUNT) or 0)
            cap.release()
            self.slider.setRange(0, total)
            self.slider.setValue(0)
            self.thread.set_file(path)
            self.thread.start()
        else: self.thread.play()

    def on_stop(self):
        # 1) pause the current thread
        self.thread.pause()

        # 2) rewind the capture back to frame 0
        cap = self.thread.capture
        if cap and cap.isOpened():
            cap.set(cv2.CAP_PROP_POS_FRAMES, 0)
            self.thread._frame_idx = 0

            # 3) grab & display the first frame
            ret, frame = cap.read()
            if ret:
                intensity = frame[...,0] if frame.ndim == 3 else frame
                timestamp = cap.get(cv2.CAP_PROP_POS_MSEC) / 1000.0
                self.widget.update_frame(intensity, timestamp)

            # 4) update the slider
            self.update_slider(0)

    def on_slider_moved(self, pos):
        # Pause thread before seeking
        self.thread.pause()
        cap = self.thread.capture
        if cap and cap.isOpened():
            cap.set(cv2.CAP_PROP_POS_FRAMES, pos)
            ret, frame = cap.read()
            if ret:
                intensity = frame[..., 0] if frame.ndim == 3 else frame
                timestamp = cap.get(cv2.CAP_PROP_POS_MSEC) / 1000.0
                self.widget.update_frame(intensity, timestamp)

        # Sync slider
        self.slider.blockSignals(True)
        self.slider.setValue(int(cap.get(cv2.CAP_PROP_POS_FRAMES)))
        self.slider.blockSignals(False)

    def dragEnterEvent(self, event):
        if event.mimeData().hasUrls(): event.acceptProposedAction()

    def dropEvent(self, event):
        url = event.mimeData().urls()[0].toLocalFile()
        if url.lower().endswith('.mkv'): self.line.setText(url); self.on_stop(); self.on_start()

    def closeEvent(self, event):
        # ensure thread stops before exit
        if self.thread.isRunning():
            self.thread.stop()
            self.thread.wait()
        event.accept()

if __name__=='__main__':
    args = parse_args()
    app = QApplication(sys.argv)
    if not args.mkv:
        file, _ = QFileDialog.getOpenFileName(None, "Select Sonar MKV File", "", "MKV Files (*.mkv)")
        if not file: sys.exit(0)
        args.mkv = file
    win = SonarPlayer(args)
    win.line.setText(args.mkv)
    win.on_start()
    win.resize(800, 600)
    win.show()
    sys.exit(app.exec_())
