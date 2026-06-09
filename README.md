# ESP32 SSD1306 OLED Driver


**ESP-IDF component for SSD1306 OLED displays (128x64 and 128x32) over I2C**

## Features
- ✅ I2C interface support
- ✅ Supports 128x64 and 128x32 panels
- ✅ Configurable GPIO pins via menuconfig
- ✅ Simple API for text and graphics
- ✅ 8x8 font included

## Installation

### Using IDF Component Manager (Recommended)

Add to your `main/idf_component.yml`:

```yaml
dependencies:
  mehrang13/esp32-oled-ssd1306: "^1.0.0"
```