# Connect-an-STM32-to-MQTT-using-the-W5500.

# 🚀 Giới thiệu
Project này sử dụng STM32F103 kết hợp với module Ethernet W5500 để:
- Kết nối mạng qua DHCP
- Giao tiếp với MQTT Broker
- Nhận dữ liệu từ MQTT → gửi xuống UART
- Cấu hình UART từ xa thông qua MQTT

# 🔌 Sơ đồ đấu dây phần cứng
## 🧠 STM32F103C8T6 ↔ W5500 (SPI)
| STM32 Pin | Function     | W5500 Pin | Description      |
|----------|-------------|----------|------------------|
| PA5      | SPI1_SCK    | SCLK     | SPI Clock        |
| PA6      | SPI1_MISO   | MISO     | Master In Slave Out |
| PA7      | SPI1_MOSI   | MOSI     | Master Out Slave In |
| PA4      | GPIO (CS)   | SCSn     | Chip Select      |
| 3.3V     | Power       | VCC      | Supply Voltage   |
| GND      | Ground      | GND      | Ground           |

- Use 3.3V for W5500 (NOT 5V)
- SPI Mode: 0 (CPOL=0, CPHA=0)
- CS pin is fixed at PA4 in code
- Ensure common GND between all devices

## 💻 UART1 ( STM32F103C8T6 ↔ External Device)
| STM32 Pin | Function | External Device | Description |
|----------|---------|----------------|------------|
| PA9      | TX      | RX             | Transmit   |
| PA10     | RX      | TX             | Receive    |
| GND      | GND     | GND            | Ground     |

<p><b>Chỉnh sửa:</b></p>
<pre>
unsigned char broker_mqtt[] = "broker.hivemq.com";
</pre>

<p><b>Chỉnh sửa:</b></p>
<pre>
data.username.cstring = "your_username";
data.password.cstring = "your_password";
</pre>

<p><b>Chỉnh sửa:</b></p>
<pre>
char topic_send[] = "your_topic_send";
char topic_recv[] = "your_topic_recv";
</pre>

<p><b>Chỉnh sửa nếu dùng TLS:</b></p>
<pre>
int SERVER_PORT = 8883;
</pre>
