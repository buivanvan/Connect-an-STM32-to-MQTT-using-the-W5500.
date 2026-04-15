# Connect-an-STM32-to-MQTT-using-the-W5500.

STM32 (SPI1)
   ├── PA5 → SCLK → W5500
   ├── PA6 ← MISO ← W5500
   ├── PA7 → MOSI → W5500
   └── PA4 → CS   → W5500

STM32 (UART1)
   ├── PA9  → TX → External RX
   └── PA10 ← RX ← External TX

STM32
   └── PC13 → LED
