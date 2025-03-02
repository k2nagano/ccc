import sys
import rclpy
from rclpy.node import Node
from PySide2.QtWidgets import QApplication, QWidget, QVBoxLayout, QPushButton, QLineEdit, QLabel, QGridLayout
from PySide2.QtCore import QTimer, Qt
from PySide2.QtGui import QColor, QPalette
from std_msgs.msg import String, Float32MultiArray
from uro_control_msgs.msg import LanioRelayOnOff
from uro_control_msgs.srv import SetLanioRelayOnOff, GetLanioRelayOnOff


class LanioUI(Node, QWidget):
    def __init__(self):
        Node.__init__(self, 'lanio_ui')
        QWidget.__init__(self)
        self.setWindowTitle("Lanio UI")
        self.setGeometry(100, 100, 400, 250)

        # Layouts
        self.main_layout = QVBoxLayout()
        self.grid_layout = QGridLayout()

        # Buttons
        self.dvl_button = QPushButton("DVL: OFF")
        self.dvl_button.setStyleSheet("color: blue;")
        self.thruster_button = QPushButton("Thruster: OFF")
        self.thruster_button.setStyleSheet("color: green;")

        # Service Response Display
        self.service_result = self.create_qlineedit()

        # Error Display
        self.error_display = self.create_qlineedit()

        # Grid Labels and Values
        self.labels = [
            ("Leak[0~24V]", "V", "red"),
            ("Temperature(300V)", "℃", "green"),
            ("Temperature(24V)", "℃", "blue")
        ]
        self.value_fields = []
        self.status_fields = []

        for i, (label, unit, color) in enumerate(self.labels):
            lbl = QLabel(label)
            lbl.setStyleSheet(f"color: {color}; font-weight: bold;")
            lbl.setAlignment(Qt.AlignRight)

            value_field = self.create_qlineedit()
            value_field.setAlignment(Qt.AlignRight)
            self.value_fields.append(value_field)

            unit_label = QLabel(unit)
            unit_label.setAlignment(Qt.AlignRight)

            status_field = self.create_qlineedit()
            status_field.setAlignment(Qt.AlignLeft)
            self.status_fields.append(status_field)

            self.grid_layout.addWidget(lbl, i, 0)
            self.grid_layout.addWidget(value_field, i, 1)
            self.grid_layout.addWidget(unit_label, i, 2)
            self.grid_layout.addWidget(status_field, i, 3)

        # Add widgets to main layout
        self.main_layout.addWidget(self.dvl_button)
        self.main_layout.addWidget(self.thruster_button)
        self.main_layout.addWidget(self.service_result)
        self.main_layout.addWidget(self.error_display)
        self.main_layout.addLayout(self.grid_layout)
        self.setLayout(self.main_layout)

        # ROS2 Service Clients
        self.set_lanio_relay_client = self.create_client(SetLanioRelayOnOff, 'set_lanio_relay_on_off')
        self.get_lanio_relay_client = self.create_client(GetLanioRelayOnOff, 'get_lanio_relay_on_off')

        # ROS2 Subscribers
        self.create_subscription(LanioRelayOnOff, 'lanio_relay_on_off', self.relay_callback, 10)
        self.create_subscription(String, 'lanio_error', self.error_callback, 10)
        self.create_subscription(Float32MultiArray, 'lanio_leak', self.leak_callback, 10)
        self.create_subscription(Float32MultiArray, 'lanio_temperature', self.temperature_callback, 10)

        # Connect button events
        self.dvl_button.clicked.connect(lambda: self.toggle_relay(0))
        self.thruster_button.clicked.connect(lambda: self.toggle_relay(1))

        # Get initial state
        self.get_initial_state()

        # Timer to handle ROS2 events
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.ros_spin)
        self.timer.start(100)

    def relay_callback(self, msg):
        self.update_button_state(msg.lanio_relay.id, msg.on_off)

    def create_qlineedit(self):
        line_edit = QLineEdit()
        line_edit.setReadOnly(True)
        line_edit.setFocusPolicy(Qt.ClickFocus)
        palette = line_edit.palette()
        palette.setColor(QPalette.Base, self.palette().color(QPalette.Window))
        line_edit.setPalette(palette)
        return line_edit

    def ros_spin(self):
        rclpy.spin_once(self, timeout_sec=0.01)

    def toggle_relay(self, relay_id):
        new_state = not (self.dvl_state if relay_id == 0 else self.thruster_state)
        request = SetLanioRelayOnOff.Request()
        request.lanio_relay.id = relay_id
        request.on_off = new_state
        future = self.set_lanio_relay_client.call_async(request)
        future.add_done_callback(lambda future: self.handle_service_result(future, relay_id, new_state))

    def handle_service_result(self, future, relay_id, new_state):
        response = future.result()
        result_text = f"Success: {response.success}, Message: {response.message}"
        self.service_result.setText(result_text)
        if response.success:
            self.update_button_state(relay_id, new_state)

    def update_button_state(self, relay_id, state):
        if relay_id == 0:
            self.dvl_state = state
            self.dvl_button.setText(f"DVL: {'ON' if state else 'OFF'}")
            self.dvl_button.setStyleSheet("color: blue; background-color: lightblue;" if state else "color: blue;")
        else:
            self.thruster_state = state
            self.thruster_button.setText(f"Thruster: {'ON' if state else 'OFF'}")
            self.thruster_button.setStyleSheet(
                "color: green; background-color: lightgreen;" if state else "color: green;")

    def get_initial_state(self):
        for relay_id in [0, 1]:
            request = GetLanioRelayOnOff.Request()
            request.lanio_relay.id = relay_id
            future = self.get_lanio_relay_client.call_async(request)
            future.add_done_callback(lambda future, rid=relay_id: self.handle_initial_state(future, rid))

    def handle_initial_state(self, future, relay_id):
        response = future.result()
        if response.success:
            self.update_button_state(relay_id, response.on_off)

    def error_callback(self, msg):
        if len(msg.data) > 0:
            self.error_display.setText(msg.data)
            self.error_display.setStyleSheet("color: red; background-color: yellow;")
        else:
            self.error_display.setText("")
            self.error_display.setStyleSheet("")

    def leak_callback(self, msg):
        self.value_fields[0].setText(f"{msg.data[0]:.1f}")
        if msg.data[0] < 1.0:
            self.status_fields[0].setText("Leaked")
            self.status_fields[0].setStyleSheet("color: white; background-color: red;")
        else:
            self.status_fields[0].setText("")
            self.status_fields[0].setStyleSheet("")

    def temperature_callback(self, msg):
        for i in range(2):
            self.value_fields[i + 1].setText(f"{msg.data[i]:.1f}")
            if msg.data[i] > 75:
                self.status_fields[i + 1].setText("too hot")
                self.status_fields[i + 1].setStyleSheet("background-color: red;")
            elif msg.data[i] > 65:
                self.status_fields[i + 1].setText("hot")
                self.status_fields[i + 1].setStyleSheet("background-color: yellow;")
            else:
                self.status_fields[i + 1].setText("")
                self.status_fields[i + 1].setStyleSheet("")


def main(args=None):
    rclpy.init(args=args)
    app = QApplication(sys.argv)
    window = LanioUI()
    window.show()
    app.exec_()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
