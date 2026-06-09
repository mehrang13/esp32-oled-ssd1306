/**
 * @file ssd1306.h
 * @brief ESP32 SSD1306 OLED Display Driver (I2C)
 * 
 * This driver supports 128x64 and 128x32 OLED displays with SSD1306 controller.
 */

#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>
#include <stdbool.h>
#include "driver/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SSD1306 display configuration structure
 */
typedef struct {
    i2c_port_t i2c_port;        /**< I2C port number (I2C_NUM_0 or I2C_NUM_1) */
    uint8_t i2c_addr;           /**< I2C address (typically 0x3C or 0x3D) */
    uint8_t width;              /**< Display width in pixels (128 for most) */
    uint8_t height;             /**< Display height in pixels (64 or 32) */
    int sda_pin;                /**< SDA GPIO pin */
    int scl_pin;                /**< SCL GPIO pin */
    bool reset_pin_enabled;     /**< Whether reset pin is used */
    int reset_pin;              /**< Reset GPIO pin (-1 if not used) */
} ssd1306_config_t;

/**
 * @brief SSD1306 handle (opaque structure)
 */
typedef struct ssd1306_t* ssd1306_handle_t;

/**
 * @brief Default configuration for 128x64 display
 * @return Default configuration structure
 */
ssd1306_config_t ssd1306_get_default_config_128x64(void);

/**
 * @brief Default configuration for 128x32 display
 * @return Default configuration structure
 */
ssd1306_config_t ssd1306_get_default_config_128x32(void);

/**
 * @brief Initialize SSD1306 display
 * @param config Display configuration
 * @return Handle to display, or NULL on error
 */
ssd1306_handle_t ssd1306_init(const ssd1306_config_t *config);

/**
 * @brief Deinitialize display and free resources
 * @param handle Display handle
 */
void ssd1306_deinit(ssd1306_handle_t handle);

/**
 * @brief Clear entire display (fill with 0)
 * @param handle Display handle
 */
void ssd1306_clear(ssd1306_handle_t handle);

/**
 * @brief Fill entire display (fill with 1)
 * @param handle Display handle
 */
void ssd1306_fill(ssd1306_handle_t handle);

/**
 * @brief Update display buffer to hardware
 * @param handle Display handle
 */
void ssd1306_update(ssd1306_handle_t handle);

/**
 * @brief Display text on screen
 * @param handle Display handle
 * @param text String to display
 * @param x X position in pixels
 * @param y Y position in pixels (row number 0-7 for 8x8 font)
 */
void ssd1306_show_text(ssd1306_handle_t handle, const char* text, uint8_t x, uint8_t y);

/**
 * @brief Draw a single pixel
 * @param handle Display handle
 * @param x X coordinate
 * @param y Y coordinate
 * @param color 1 = white, 0 = black
 */
void ssd1306_draw_pixel(ssd1306_handle_t handle, uint8_t x, uint8_t y, bool color);

/**
 * @brief Invert display colors
 * @param handle Display handle
 * @param invert true = invert, false = normal
 */
void ssd1306_invert(ssd1306_handle_t handle, bool invert);

#ifdef __cplusplus
}
#endif

#endif /* SSD1306_H */
