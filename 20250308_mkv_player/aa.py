import sys
import numpy as np
import ffmpeg
from PySide2.QtWidgets import QApplication, QMainWindow, QFileDialog, QGraphicsScene, QGraphicsView, QGraphicsPixmapItem, QSlider
from PySide2.QtCore import Qt, QTimer
from PySide2.QtGui import QPixmap, QImage

class SonarViewer(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("ソナー MKV ビューア")
        self.setGeometry(100, 100, 800, 600)

        # QGraphicsView（ソナー表示）
        self.scene = QGraphicsScene()
        self.view = QGraphicsView(self.scene, self)
        self.setCentralWidget(self.view)

        # QSlider（時間位置変更）
        self.slider = QSlider(Qt.Horizontal, self)
        self.slider.setGeometry(50, 550, 700, 20)
        self.slider.setEnabled(False)
        self.slider.valueChanged.connect(self.change_frame)

        # MKV関連
        self.video_path = None
        self.total_frames = 0
        self.fps = 15
        self.timer = QTimer()
        self.timer.timeout.connect(self.next_frame)

        # ウィンドウにドラッグ＆ドロップを許可
        self.setAcceptDrops(True)

        # 初回ファイル選択
        self.open_video()

    def open_video(self):
        file_path, _ = QFileDialog.getOpenFileName(self, "MKVファイルを選択", "", "MKV Files (*.mkv)")
        if file_path:
            self.load_video(file_path)

    def load_video(self, file_path):
        self.video_path = file_path

        # FFmpegで動画のメタデータを取得
        try:
            probe = ffmpeg.probe(file_path)
            video_streams = [stream for stream in probe['streams'] if stream['codec_type'] == 'video']
            if not video_streams:
                raise Exception("MKV内に動画ストリームがありません")

            video_stream = video_streams[0]
            self.fps = eval(video_stream.get('r_frame_rate', '15'))  # FPS を取得
            self.total_frames = int(video_stream.get('nb_frames', 1000))  # フレーム数

            print(f"ロード完了: {file_path}, FPS: {self.fps}, 総フレーム: {self.total_frames}")

            self.slider.setRange(0, self.total_frames - 1)
            self.slider.setEnabled(True)
            self.timer.start(int(1000 / self.fps))  # FPS に応じたタイマー設定

        except Exception as e:
            print(f"MKV 読み込みエラー: {e}")
            return

    def change_frame(self, frame_idx):
        if self.video_path:
            self.next_frame(frame_idx)

    def next_frame(self, frame_idx=None):
        if not self.video_path:
            return

        # 指定したフレームを抽出
        try:
            out, _ = (
                ffmpeg.input(self.video_path, ss=frame_idx / self.fps)
                .output('pipe:', format='rawvideo', pix_fmt='gray')
                .run(capture_stdout=True, quiet=True)
            )

            frame = np.frombuffer(out, np.uint8).reshape((256, 512))  # サイズに合わせて変更
            sonar_image = self.create_sonar_display(frame)

            # QGraphicsView に表示
            h, w = sonar_image.shape
            qimg = QImage(sonar_image.data, w, h, w, QImage.Format_Grayscale8)
            pixmap = QPixmap.fromImage(qimg)
            self.scene.clear()
            self.scene.addPixmap(pixmap)

            # スライダーの位置を更新
            self.slider.blockSignals(True)
            self.slider.setValue(frame_idx if frame_idx else self.slider.value() + 1)
            self.slider.blockSignals(False)

        except Exception as e:
            print(f"フレーム読み込みエラー: {e}")
            self.timer.stop()

    def create_sonar_display(self, frame):
        """ ソナー画像を扇形に変換 """
        h, w = frame.shape
        output = np.zeros((w, w), dtype=np.uint8)

        # ソナーのビームの本数と角度
        n_beams = w
        angle_step = 180 / n_beams

        for i in range(n_beams):
            theta = np.deg2rad(i * angle_step - 90)
            x1 = int(w / 2 + np.cos(theta) * w / 2)
            y1 = int(w / 2 + np.sin(theta) * w / 2)
            cv2.line(output, (w // 2, w // 2), (x1, y1), int(frame[:, i].mean()), 1)

        return output

    def dragEnterEvent(self, event):
        """ ウィンドウにドラッグされたとき """
        if event.mimeData().hasUrls():
            event.acceptProposedAction()

    def dropEvent(self, event):
        """ ウィンドウにドロップされたとき """
        urls = event.mimeData().urls()
        if urls:
            file_path = urls[0].toLocalFile()
            if file_path.endswith(".mkv"):
                self.load_video(file_path)

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = SonarViewer()
    window.show()
    sys.exit(app.exec_())

