# Posture Pro

Posture Pro is a smart posture correction device designed to help individuals improve their posture and combat the negative effects of prolonged sitting. By utilizing ESP32, MPU6050, and haptic feedback vibration motors, Posture Pro offers real-time posture monitoring and alerts to encourage better sitting habits.

## Features

- **Real-time Posture Monitoring**: Utilizes MPU6050 sensor to detect the user's posture in real-time by checking angle deviation from gyro sensor and movement detection from accelerometer data.
- **Motion Detection**: Checks whether the user is sitting or in motion - walking/running.
- **Haptic Feedback Alerts**: Provides haptic feedback through vibration motors to alert the user when poor posture is detected.
- **Web Interface**: Connects to the user's Wi-Fi network, allowing data to be accessed via a designated IP address on a web page.
- **Mobile App Integration**: In progress - aims to create a mobile app for a more convenient user experience.

## How It Works

Posture Pro works by continuously monitoring the user's posture using the MPU6050 sensor. It checks the angle deviation from the gyro sensor and movement detection from accelerometer data to determine the user's posture in real-time. When poor posture is detected, the device provides haptic feedback through vibration motors to alert the user. Additionally, the device connects to the user's Wi-Fi network, allowing posture data to be accessed via a web page hosted on a designated IP address.

## Future Plans

- **Machine Learning Integration**: Implement machine learning algorithms to further enhance posture detection and provide personalized feedback and guidance.
- **Mobile App Development**: Create a mobile app to provide users with a more convenient and feature-rich experience, including posture tracking, personalized tips, and guides.
- **Additional Features**: Explore additional features such as posture analysis, exercise recommendations, and progress tracking.

## Getting Started

To get started with Posture Pro, follow these steps:

1. Assemble the required hardware components: ESP32, MPU6050, and haptic feedback vibration motors.
2. Set up the ESP32 device and connect it to your Wi-Fi network.
3. Install the necessary libraries and dependencies on the ESP32.
4. Upload the Posture Pro firmware to the ESP32.
5. Place the device on the back of your neck by using a hanging case or something similar.
6. Access the posture data via the designated IP address on a web page hosted by the ESP32.
7. Use the device regularly to monitor and improve your posture!

## Contribution

Contributions to Posture Pro are welcome! If you have any ideas, suggestions, or improvements, feel free to open an issue or submit a pull request.

## License

This project is licensed under the MIT License.
