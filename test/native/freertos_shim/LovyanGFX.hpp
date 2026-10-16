#pragma once
// V1.5.20 host shim of LovyanGFX 1.2.32 for src/display/display.cpp (via src/display/lgfx_config.h).
// Same names and signatures as the library for the calls the display may use:
//   init, setRotation, setBrightness, startWrite, endWrite, fillScreen, fillRect, drawRect,
//   setTextSize, setTextColor(fg, bg), setTextDatum, drawString, getTouch, color888, width, height.
// Colours are passed as uint32_t RGB888 (as with the real library when the argument is uint32_t).
// The shim keeps a model of the screen: a 480x320 pixel colour buffer (fills and rectangles) and
// the list of visible texts with their bounding boxes (built-in font: 6 x 8 px per character times
// the text size). A fill that covers a text completely removes it; a new text removes every text it
// overlaps. The touch is set by the test (shimLcd().touch_*).
#include <stdint.h>
#include <string.h>
#include <string>
#include <vector>
#include <algorithm>

#define LGFX_USE_V1
#ifndef VSPI_HOST
#define VSPI_HOST 3
#endif
#ifndef SPI_DMA_CH_AUTO
#define SPI_DMA_CH_AUTO 3
#endif

struct ShimText { int x0, y0, x1, y1; std::string text; uint32_t fg, bg; int size; };
struct ShimLcd {
    static constexpr int W = 480, H = 320;
    std::vector<uint32_t> px = std::vector<uint32_t>(W * H, 0);
    std::vector<ShimText> texts;
    bool init_result = true;
    int  inits = 0, rotation = -1, brightness = -1, writes_open = 0;
    long draw_calls = 0;                 ///< every fill/rect/text call
    bool touch_down = false; int touch_x = 0, touch_y = 0;
    int  touch_polls = 0;
    void reset() { *this = ShimLcd(); }
    uint32_t pixel(int x, int y) const { return (x >= 0 && y >= 0 && x < W && y < H) ? px[y * W + x] : 0xFFFFFFFFu; }
    /// visible texts that contain `needle`
    std::vector<ShimText> find(const char* needle) const {
        std::vector<ShimText> r; for (auto& t : texts) if (t.text.find(needle) != std::string::npos) r.push_back(t); return r;
    }
    bool visible(const char* needle) const { return !find(needle).empty(); }
    /// all visible texts whose box lies inside [x0,x1) x [y0,y1), concatenated in drawing order
    std::string textIn(int x0, int y0, int x1, int y1) const {
        std::string s; for (auto& t : texts) if (t.x0 >= x0 && t.y0 >= y0 && t.x1 <= x1 && t.y1 <= y1) { if (!s.empty()) s += " "; s += t.text; }
        return s;
    }
};
inline ShimLcd& shimLcd() { static ShimLcd l; return l; }

namespace lgfx {
inline namespace v1 {
enum textdatum_t : uint8_t { top_left = 0, top_center = 1, top_right = 2, middle_left = 4, middle_center = 5,
                             middle_right = 6, bottom_left = 8, bottom_center = 9, bottom_right = 10 };
inline uint32_t color888(uint8_t r, uint8_t g, uint8_t b) { return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b; }

struct Bus_SPI {
    struct config_t { int spi_host = 0, spi_mode = 0; uint32_t freq_write = 0, freq_read = 0; bool spi_3wire = false,
                      use_lock = true; int dma_channel = 0, pin_sclk = -1, pin_mosi = -1, pin_miso = -1, pin_dc = -1; };
    config_t cfg_; config_t config() const { return cfg_; } void config(const config_t& c) { cfg_ = c; }
};
struct Light_PWM {
    struct config_t { int pin_bl = -1; bool invert = false; uint32_t freq = 0; int pwm_channel = 0; };
    config_t cfg_; config_t config() const { return cfg_; } void config(const config_t& c) { cfg_ = c; }
};
struct Touch_XPT2046 {
    struct config_t { int x_min = 0, x_max = 0, y_min = 0, y_max = 0, pin_int = -1; bool bus_shared = false;
                      int offset_rotation = 0, spi_host = 0; uint32_t freq = 0;
                      int pin_sclk = -1, pin_mosi = -1, pin_miso = -1, pin_cs = -1; };
    config_t cfg_; config_t config() const { return cfg_; } void config(const config_t& c) { cfg_ = c; }
};
struct Panel_ST7796 {
    struct config_t { int pin_cs = -1, pin_rst = -1, pin_busy = -1, panel_width = 0, panel_height = 0,
                      offset_x = 0, offset_y = 0, offset_rotation = 0; bool readable = false, invert = false,
                      rgb_order = false, dlen_16bit = false, bus_shared = false; };
    config_t cfg_; config_t config() const { return cfg_; } void config(const config_t& c) { cfg_ = c; }
    void setBus(Bus_SPI*) {} void setLight(Light_PWM*) {} void setTouch(Touch_XPT2046*) {}
};

class LGFX_Device {
    uint8_t size_ = 1; uint32_t fg_ = 0xFFFFFF, bg_ = 0; textdatum_t datum_ = top_left;
    static void fill(int x, int y, int w, int h, uint32_t c) {
        ShimLcd& l = shimLcd(); ++l.draw_calls;
        int x0 = std::max(0, x), y0 = std::max(0, y), x1 = std::min(ShimLcd::W, x + w), y1 = std::min(ShimLcd::H, y + h);
        for (int yy = y0; yy < y1; ++yy) for (int xx = x0; xx < x1; ++xx) l.px[yy * ShimLcd::W + xx] = c;
        l.texts.erase(std::remove_if(l.texts.begin(), l.texts.end(), [&](const ShimText& t) {
            return t.x0 >= x && t.y0 >= y && t.x1 <= x + w && t.y1 <= y + h; }), l.texts.end());
    }
public:
    void setPanel(Panel_ST7796*) {}
    bool init() { ++shimLcd().inits; return shimLcd().init_result; }
    void setRotation(uint_fast8_t r) { shimLcd().rotation = r; }
    void setBrightness(uint8_t b) { shimLcd().brightness = b; }
    void startWrite() { ++shimLcd().writes_open; }
    void endWrite() { --shimLcd().writes_open; }
    int32_t width() const { return ShimLcd::W; }
    int32_t height() const { return ShimLcd::H; }
    static uint32_t color888(uint8_t r, uint8_t g, uint8_t b) { return lgfx::color888(r, g, b); }
    void fillScreen(uint32_t c) { fill(0, 0, ShimLcd::W, ShimLcd::H, c); }
    void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t c) { fill(x, y, w, h, c); }
    void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t c) {
        ShimLcd& l = shimLcd(); ++l.draw_calls;
        for (int xx = x; xx < x + w; ++xx) { if (xx >= 0 && xx < ShimLcd::W) { if (y >= 0 && y < ShimLcd::H) l.px[y * ShimLcd::W + xx] = c;
            if (y + h - 1 >= 0 && y + h - 1 < ShimLcd::H) l.px[(y + h - 1) * ShimLcd::W + xx] = c; } }
        for (int yy = y; yy < y + h; ++yy) { if (yy >= 0 && yy < ShimLcd::H) { if (x >= 0 && x < ShimLcd::W) l.px[yy * ShimLcd::W + x] = c;
            if (x + w - 1 >= 0 && x + w - 1 < ShimLcd::W) l.px[yy * ShimLcd::W + x + w - 1] = c; } }
    }
    void setTextSize(float s) { size_ = (uint8_t)(s < 1 ? 1 : s); }
    void setTextColor(uint32_t fg, uint32_t bg) { fg_ = fg; bg_ = bg; }
    void setTextColor(uint32_t fg) { fg_ = fg; bg_ = fg; }   // like the library: transparent background
    void setTextDatum(textdatum_t d) { datum_ = d; }
    void setTextDatum(uint8_t d) { datum_ = (textdatum_t)d; }
    size_t drawString(const char* s, int32_t x, int32_t y) {
        ShimLcd& l = shimLcd(); ++l.draw_calls;
        int w = (int)strlen(s) * 6 * size_, h = 8 * size_;
        int hx = (datum_ & 3) == 1 ? w / 2 : (datum_ & 3) == 2 ? w : 0;
        int vy = (datum_ >> 2) == 1 ? h / 2 : (datum_ >> 2) == 2 ? h : 0;
        ShimText t{(int)x - hx, (int)y - vy, (int)x - hx + w, (int)y - vy + h, s, fg_, bg_, size_};
        l.texts.erase(std::remove_if(l.texts.begin(), l.texts.end(), [&](const ShimText& o) {
            return o.x0 < t.x1 && t.x0 < o.x1 && o.y0 < t.y1 && t.y0 < o.y1; }), l.texts.end());
        if (fg_ != bg_) {   // text with a background colour paints its box
            for (int yy = std::max(0, t.y0); yy < std::min(ShimLcd::H, t.y1); ++yy)
                for (int xx = std::max(0, t.x0); xx < std::min(ShimLcd::W, t.x1); ++xx) l.px[yy * ShimLcd::W + xx] = bg_;
        }
        if (w > 0) l.texts.push_back(t);
        return (size_t)w;
    }
    template <typename T> uint_fast8_t getTouch(T* x, T* y, uint_fast8_t index = 0) {
        ShimLcd& l = shimLcd(); ++l.touch_polls;
        if (!l.touch_down) return 0;
        if (x) { *x = (T)l.touch_x; }
        if (y) { *y = (T)l.touch_y; }
        return 1;
    }
};
}  // namespace v1
}  // namespace lgfx
