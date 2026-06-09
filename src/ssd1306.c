/**
 * @file ssd1306.c
 * @brief Implementation of SSD1306 driver
 */

#include "ssd1306.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// SSD1306 Commands
#define SSD1306_SET_CONTRAST            0x81
#define SSD1306_DISPLAY_ALL_ON_RESUME   0xA4
#define SSD1306_DISPLAY_ALL_ON          0xA5
#define SSD1306_NORMAL_DISPLAY          0xA6
#define SSD1306_INVERT_DISPLAY          0xA7
#define SSD1306_DISPLAY_OFF             0xAE
#define SSD1306_DISPLAY_ON              0xAF
#define SSD1306_SET_DISPLAY_CLOCK_DIV   0xD5
#define SSD1306_SET_MULTIPLEX           0xA8
#define SSD1306_SET_DISPLAY_OFFSET      0xD3
#define SSD1306_SET_START_LINE          0x40
#define SSD1306_CHARGE_PUMP             0x8D
#define SSD1306_MEMORY_MODE             0x20
#define SSD1306_SEG_REMAP               0xA0
#define SSD1306_COM_SCAN_DIR            0xC0
#define SSD1306_SET_COM_PINS            0xDA
#define SSD1306_SET_VCOM_DETECT         0xDB
#define SSD1306_SET_PAGE_ADDR           0x22
#define SSD1306_SET_COLUMN_ADDR         0x21
#define SSD1306_DEACTIVATE_SCROLL       0x2E
#define SSD1306_ACTIVATE_SCROLL         0x2F

// I2C Control Byte
#define SSD1306_I2C_CTRL_CMD            0x00
#define SSD1306_I2C_CTRL_DATA           0x40

// Display dimensions
#define SSD1306_WIDTH_DEFAULT           128
#define SSD1306_HEIGHT_64               64
#define SSD1306_HEIGHT_32               32
#define SSD1306_PAGES_64                8
#define SSD1306_PAGES_32                4

// Font: 8x8 basic ASCII (32-127)
static const uint8_t font8x8_basic[96][8] = {
    // ... (font data would go here - for brevity, using simple placeholder)
    // In real implementation, you'd include full font array
};

typedef struct ssd1306_t {
    i2c_port_t i2c_port;
    uint8_t i2c_addr;
    uint8_t width;
    uint8_t height;
    uint8_t pages;
    uint8_t* buffer;
} ssd1306_t;

// Helper: send command to display
static esp_err_t ssd1306_send_cmd(ssd1306_handle_t handle, uint8_t cmd) {
    ssd1306_t* dev = (ssd1306_t*)handle;
    uint8_t buf[2] = {SSD1306_I2C_CTRL_CMD, cmd};
    return i2c_master_write_to_device(dev->i2c_port, dev->i2c_addr, buf, 2, pdMS_TO_TICKS(100));
}

// Helper: send data buffer
static esp_err_t ssd1306_send_data(ssd1306_handle_t handle, uint8_t* data, size_t len) {
    ssd1306_t* dev = (ssd1306_t*)handle;
    uint8_t* buf = malloc(len + 1);
    if (!buf) return ESP_ERR_NO_MEM;
    buf[0] = SSD1306_I2C_CTRL_DATA;
    memcpy(buf + 1, data, len);
    esp_err_t ret = i2c_master_write_to_device(dev->i2c_port, dev->i2c_addr, buf, len + 1, pdMS_TO_TICKS(100));
    free(buf);
    return ret;
}

ssd1306_config_t ssd1306_get_default_config_128x64(void) {
    ssd1306_config_t config = {
        .i2c_port = I2C_NUM_0,
        .i2c_addr = 0x3C,
        .width = 128,
        .height = 64,
        .sda_pin = 21,
        .scl_pin = 22,
        .reset_pin_enabled = false,
        .reset_pin = -1
    };
    return config;
}

ssd1306_config_t ssd1306_get_default_config_128x32(void) {
    ssd1306_config_t config = {
        .i2c_port = I2C_NUM_0,
        .i2c_addr = 0x3C,
        .width = 128,
        .height = 32,
        .sda_pin = 21,
        .scl_pin = 22,
        .reset_pin_enabled = false,
        .reset_pin = -1
    };
    return config;
}

ssd1306_handle_t ssd1306_init(const ssd1306_config_t* config) {
    ssd1306_t* dev = calloc(1, sizeof(ssd1306_t));
    if (!dev) return NULL;
    
    dev->i2c_port = config->i2c_port;
    dev->i2c_addr = config->i2c_addr;
    dev->width = config->width;
    dev->height = config->height;
    dev->pages = dev->height / 8;
    
    // Allocate buffer
    size_t buffer_size = dev->width * dev->pages;
    dev->buffer = calloc(1, buffer_size);
    if (!dev->buffer) {
        free(dev);
        return NULL;
    }
    
    // Initialize I2C bus if needed
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = config->sda_pin,
        .scl_io_num = config->scl_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000
    };
    
    esp_err_t ret = i2c_param_config(dev->i2c_port, &i2c_conf);
    if (ret != ESP_OK) {
        free(dev->buffer);
        free(dev);
        return NULL;
    }
    
    ret = i2c_driver_install(dev->i2c_port, I2C_MODE_MASTER, 0, 0, 0);
    if (ret != ESP_OK) {
        free(dev->buffer);
        free(dev);
        return NULL;
    }
    
    // Initialize display
    ssd1306_send_cmd(dev, SSD1306_DISPLAY_OFF);
    ssd1306_send_cmd(dev, SSD1306_SET_DISPLAY_CLOCK_DIV);
    ssd1306_send_cmd(dev, 0x80);
    ssd1306_send_cmd(dev, SSD1306_SET_MULTIPLEX);
    ssd1306_send_cmd(dev, dev->height - 1);
    ssd1306_send_cmd(dev, SSD1306_SET_DISPLAY_OFFSET);
    ssd1306_send_cmd(dev, 0x00);
    ssd1306_send_cmd(dev, SSD1306_SET_START_LINE);
    ssd1306_send_cmd(dev, SSD1306_CHARGE_PUMP);
    ssd1306_send_cmd(dev, 0x14);  // Enable charge pump
    ssd1306_send_cmd(dev, SSD1306_MEMORY_MODE);
    ssd1306_send_cmd(dev, 0x00);  // Horizontal addressing
    ssd1306_send_cmd(dev, SSD1306_SEG_REMAP);
    ssd1306_send_cmd(dev, 0xC0);  // COM scan direction
    ssd1306_send_cmd(dev, SSD1306_SET_COM_PINS);
    if (dev->height == 64) {
        ssd1306_send_cmd(dev, 0x12);
    } else {
        ssd1306_send_cmd(dev, 0x02);
    }
    ssd1306_send_cmd(dev, SSD1306_SET_CONTRAST);
    ssd1306_send_cmd(dev, 0x7F);
    ssd1306_send_cmd(dev, SSD1306_NORMAL_DISPLAY);
    ssd1306_send_cmd(dev, SSD1306_DISPLAY_ON);
    
    // Clear buffer
    memset(dev->buffer, 0, buffer_size);
    ssd1306_update(dev);
    
    return dev;
}

void ssd1306_deinit(ssd1306_handle_t handle) {
    if (!handle) return;
    ssd1306_t* dev = (ssd1306_t*)handle;
    ssd1306_send_cmd(dev, SSD1306_DISPLAY_OFF);
    i2c_driver_delete(dev->i2c_port);
    free(dev->buffer);
    free(dev);
}

void ssd1306_clear(ssd1306_handle_t handle) {
    if (!handle) return;
    ssd1306_t* dev = (ssd1306_t*)handle;
    memset(dev->buffer, 0, dev->width * dev->pages);
    ssd1306_update(handle);
}

void ssd1306_fill(ssd1306_handle_t handle) {
    if (!handle) return;
    ssd1306_t* dev = (ssd1306_t*)handle;
    memset(dev->buffer, 0xFF, dev->width * dev->pages);
    ssd1306_update(handle);
}

void ssd1306_update(ssd1306_handle_t handle) {
    if (!handle) return;
    ssd1306_t* dev = (ssd1306_t*)handle;
    
    // Set column address range
    ssd1306_send_cmd(dev, SSD1306_SET_COLUMN_ADDR);
    ssd1306_send_cmd(dev, 0);
    ssd1306_send_cmd(dev, dev->width - 1);
    
    // Set page address range
    ssd1306_send_cmd(dev, SSD1306_SET_PAGE_ADDR);
    ssd1306_send_cmd(dev, 0);
    ssd1306_send_cmd(dev, dev->pages - 1);
    
    // Send buffer
    ssd1306_send_data(dev, dev->buffer, dev->width * dev->pages);
}

void ssd1306_show_text(ssd1306_handle_t handle, const char* text, uint8_t x, uint8_t y) {
    if (!handle || !text) return;
    ssd1306_t* dev = (ssd1306_t*)handle;
    
    if (y >= dev->pages) return;
    
    size_t len = strlen(text);
    for (size_t i = 0; i < len && x + i * 8 < dev->width; i++) {
        char c = text[i];
        if (c < 32 || c > 127) continue;
        
        const uint8_t* glyph = font8x8_basic[c - 32];
        for (int row = 0; row < 8; row++) {
            size_t buffer_index = (y * 8 + row) * dev->width + x + i * 8;
            if (buffer_index < dev->width * dev->pages * 8) {
                dev->buffer[buffer_index / 8] |= glyph[row] << (buffer_index % 8);
            }
        }
    }
    ssd1306_update(handle);
}

void ssd1306_draw_pixel(ssd1306_handle_t handle, uint8_t x, uint8_t y, bool color) {
    if (!handle) return;
    ssd1306_t* dev = (ssd1306_t*)handle;
    if (x >= dev->width || y >= dev->height) return;
    
    uint8_t page = y / 8;
    uint8_t bit = y % 8;
    size_t idx = page * dev->width + x;
    
    if (color) {
        dev->buffer[idx] |= (1 << bit);
    } else {
        dev->buffer[idx] &= ~(1 << bit);
    }
}

void ssd1306_invert(ssd1306_handle_t handle, bool invert) {
    if (!handle) return;
    ssd1306_t* dev = (ssd1306_t*)handle;
    ssd1306_send_cmd(dev, invert ? SSD1306_INVERT_DISPLAY : SSD1306_NORMAL_DISPLAY);
}
