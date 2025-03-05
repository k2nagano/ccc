#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <sensor_msgs/point_cloud2_iterator.hpp>
#include <vector>
#include <cmath>

class SonarPointCloudPublisher : public rclcpp::Node
{
public:
    SonarPointCloudPublisher()
        : Node("sonar_pointcloud_publisher"),
          nRange(10.0), nBeam(512), nSwath(120.0)
    {
        publisher_ = this->create_publisher<sensor_msgs::msg::PointCloud2>("sonar/pointcloud", 10);
        timer_ = this->create_wall_timer(std::chrono::milliseconds(100),
                                         std::bind(&SonarPointCloudPublisher::publish_pointcloud, this));
    }

private:
    void publish_pointcloud()
    {
        // 仮のセンサーデータを作成（本来はUDP受信等で取得）
        std::vector<uint8_t> sonar_data(nBeam * static_cast<int>(nRange), 0);
        for (size_t i = 0; i < sonar_data.size(); i++)
        {
            sonar_data[i] = static_cast<uint8_t>(std::rand() % 256); // 0~255のランダム強度
        }

        sensor_msgs::msg::PointCloud2 msg;
        msg.header.stamp = this->get_clock()->now();
        msg.header.frame_id = "sonar_frame"; // RVizで適切なTFを設定する必要あり

        msg.height = 1;
        msg.width = sonar_data.size();
        msg.is_bigendian = false;
        msg.is_dense = true;

        sensor_msgs::PointCloud2Modifier modifier(msg);
        modifier.setPointCloud2Fields(4,
                                      "x", 1, sensor_msgs::msg::PointField::FLOAT32,
                                      "y", 1, sensor_msgs::msg::PointField::FLOAT32,
                                      "z", 1, sensor_msgs::msg::PointField::FLOAT32,
                                      "intensity", 1, sensor_msgs::msg::PointField::FLOAT32);
        modifier.resize(sonar_data.size());

        sensor_msgs::PointCloud2Iterator<float> iter_x(msg, "x");
        sensor_msgs::PointCloud2Iterator<float> iter_y(msg, "y");
        sensor_msgs::PointCloud2Iterator<float> iter_z(msg, "z");
        sensor_msgs::PointCloud2Iterator<float> iter_intensity(msg, "intensity");

        // 角度分割
        double angle_step = nSwath * (M_PI / 180.0) / nBeam;
        double start_angle = -nSwath * (M_PI / 180.0) / 2.0;

        for (int b = 0; b < nBeam; b++)
        {
            double angle = start_angle + b * angle_step;
            double cos_angle = std::cos(angle);
            double sin_angle = std::sin(angle);

            for (int r = 0; r < static_cast<int>(nRange); r++)
            {
                double range = (r + 1) * (nRange / static_cast<double>(nRange));
                int index = b * static_cast<int>(nRange) + r;

                // 画像データの強度をIntensityとして指定
                double intensity = static_cast<double>(sonar_data[index]);

                *iter_x = range * cos_angle;
                *iter_y = range * sin_angle;
                *iter_z = 0.0; // Zを常に0に固定
                *iter_intensity = intensity; // Intensityにソナーのピクセル値を格納

                ++iter_x;
                ++iter_y;
                ++iter_z;
                ++iter_intensity;
            }
        }

        publisher_->publish(msg);
    }

    rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr publisher_;
    rclcpp::TimerBase::SharedPtr timer_;

    double nRange;
    int nBeam;
    double nSwath;
};

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<SonarPointCloudPublisher>());
    rclcpp::shutdown();
    return 0;
}

