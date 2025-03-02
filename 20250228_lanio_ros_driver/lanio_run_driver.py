import laniopy
import rclpy
from rclpy.node import Node
from std_msgs.msg import String, Float32MultiArray
from uro_control_msgs.msg import LanioRelayOnOff
from uro_control_msgs.srv import SetLanioRelayOnOff, GetLanioRelayOnOff


class LanioRunDriver(Node):
    def __init__(self):
        super().__init__('lanio_run_driver')
        self.lanio = laniopy.Lanio()
        self.lanio_error = ""

        # Publisher
        self.lanio_relay_on_off_pub = self.create_publisher(LanioRelayOnOff, 'lanio_relay_on_off', 10)
        self.lanio_error_pub = self.create_publisher(String, 'lanio_error', 10)
        self.lanio_leak_pub = self.create_publisher(Float32MultiArray, 'lanio_leak', 10)
        self.lanio_temperature_pub = self.create_publisher(Float32MultiArray, 'lanio_temperature', 10)

        # Service Servers
        self.set_lanio_relay_service = self.create_service(
            SetLanioRelayOnOff, 'set_lanio_relay_on_off', self.set_lanio_relay_callback)
        self.get_lanio_relay_service = self.create_service(
            GetLanioRelayOnOff, 'get_lanio_relay_on_off', self.get_lanio_relay_callback)

        # Timer for periodic publishing
        self.timer = self.create_timer(1.0, self.timer_callback)

        self.get_logger().info('LanioRunDriver node has been started.')

    def timer_callback(self):
        # Publish LanioRelayOnOff message
        for id in range(2):
            relay_msg = LanioRelayOnOff()
            relay_msg.lanio_relay.id = id
            relay_msg.on_off = self.lanio.get_relay(id)
            self.lanio_relay_on_off_pub.publish(relay_msg)

        # Publish error message (example case: no errors)
        if len(self.lanio_error) > 0:
            error_msg = String()
            error_msg.data = self.lanio_error
            self.lanio_error_pub.publish(error_msg)

        # Publish lanio_leak
        leak_msg = Float32MultiArray()
        leak_msg.data = [self.lanio.get_leak()]
        self.lanio_leak_pub.publish(leak_msg)

        # Publish lanio_temperature
        temp_msg = Float32MultiArray()
        temp_msg.data = self.lanio.get_temperature()
        self.lanio_temperature_pub.publish(temp_msg)

    def set_lanio_relay_callback(self, request, response):
        self.lanio.set_relay(request.lanio_relay.id, request.on_off)
        response.success = True
        response.message = f"Relay {request.lanio_relay.id} set to {'ON' if request.on_off else 'OFF'}"
        self.get_logger().info(response.message)
        return response

    def get_lanio_relay_callback(self, request, response):
        response.on_off = self.lanio.get_relay(request.lanio_relay.id)
        response.success = True
        response.message = f"Relay {request.lanio_relay.id} is {'ON' if response.on_off else 'OFF'}"
        self.get_logger().info(response.message)
        return response


def main(args=None):
    rclpy.init(args=args)
    node = LanioRunDriver()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
