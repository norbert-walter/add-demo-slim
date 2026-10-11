/**
 * @file lgfx_config.h
 * @brief LovyanGFX setup for MKS TS35R on MKS DLC32 V2.1 (Sprint 0a, verified by smoketest).
 *
 * ST7796 320x480 (used as 480x320, rotation 1), touch ADS7843E (XPT2046 protocol),
 * both on VSPI. Pins and touch calibration from config.h (smoketest 0a section 6).
 */
#pragma once
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "config.h"

class LGFX : public lgfx::LGFX_Device {
    lgfx::Panel_ST7796  _panel;
    lgfx::Bus_SPI       _bus;
    lgfx::Light_PWM     _light;
    lgfx::Touch_XPT2046 _touch;

public:
    LGFX() {
        {
            auto cfg = _bus.config();
            cfg.spi_host    = VSPI_HOST;
            cfg.spi_mode    = 0;
            cfg.freq_write  = 40000000;
            cfg.freq_read   = 16000000;
            cfg.spi_3wire   = false;
            cfg.use_lock    = true;
            cfg.dma_channel = SPI_DMA_CH_AUTO;
            cfg.pin_sclk    = PIN_DISP_SCK;
            cfg.pin_mosi    = PIN_DISP_MOSI;
            cfg.pin_miso    = PIN_DISP_MISO;
            cfg.pin_dc      = PIN_DISP_DC;
            _bus.config(cfg);
            _panel.setBus(&_bus);
        }
        {
            auto cfg = _panel.config();
            cfg.pin_cs          = PIN_DISP_CS;
            cfg.pin_rst         = PIN_DISP_RST;
            cfg.pin_busy        = -1;
            cfg.panel_width     = 320;
            cfg.panel_height    = 480;
            cfg.offset_x        = 0;
            cfg.offset_y        = 0;
            cfg.offset_rotation = 0;
            cfg.readable        = true;
            cfg.invert          = false;   // verified 0a 6.1
            cfg.rgb_order       = false;   // verified 0a 6.1
            cfg.dlen_16bit      = false;
            cfg.bus_shared      = true;    // touch on the same bus
            _panel.config(cfg);
        }
        {
            auto cfg = _light.config();
            cfg.pin_bl      = PIN_DISP_BL;
            cfg.invert      = false;
            cfg.freq        = 44100;
            cfg.pwm_channel = 7;
            _light.config(cfg);
            _panel.setLight(&_light);
        }
        {
            auto cfg = _touch.config();
            cfg.x_min           = TOUCH_X_MIN;
            cfg.x_max           = TOUCH_X_MAX;
            cfg.y_min           = TOUCH_Y_MIN;
            cfg.y_max           = TOUCH_Y_MAX;
            cfg.pin_int         = -1;
            cfg.bus_shared      = true;
            cfg.offset_rotation = 2;       // smoketest 0a 6.2: touch was rotated 180 deg against the picture
            cfg.spi_host        = VSPI_HOST;
            cfg.freq            = 1000000;
            cfg.pin_sclk        = PIN_DISP_SCK;
            cfg.pin_mosi        = PIN_DISP_MOSI;
            cfg.pin_miso        = PIN_DISP_MISO;
            cfg.pin_cs          = PIN_TOUCH_CS;
            _touch.config(cfg);
            _panel.setTouch(&_touch);
        }
        setPanel(&_panel);
    }
};
