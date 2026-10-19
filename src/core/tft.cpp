#include "thinkercad/tft.hpp"
#include <algorithm>
#include <stdexcept>
namespace thinkercad {
TftController::TftController(int w, int h, bool ili) : width_(w), height_(h), ili_(ili) {
    if ((w != 128 || h != 160) && (w != 240 || h != 320))
        throw std::invalid_argument("Unsupported TFT panel geometry");
    reset();
}
void TftController::reset() {
    pixels_.assign(width_ * height_, 0);
    sleep_ = true;
    on_ = false;
    invert_ = false;
    format_ = false;
    command_ = 0;
    madctl_ = 8;
    xValid_ = yValid_ = true;
    arguments_.clear();
    half_ = false;
    top_ = bottom_ = scroll_ = 0;
    x0_ = y0_ = x_ = y_ = read_ = 0;
    x1_ = width_ ? width_ - 1 : 0;
    y1_ = height_ ? height_ - 1 : 0;
    ready_ = 0;
    error_.clear();
    ++revision_;
    cached_.reset();
}
void TftController::next() {
    if (++x_ > x1_) {
        x_ = x0_;
        if (++y_ > y1_)
            y_ = y0_;
    }
}
void TftController::parameter() {
    auto word = [&](int at) { return unsigned(arguments_[at]) * 256 + arguments_[at + 1]; };
    if (command_ == 0x3a && arguments_.size() == 1) {
        format_ = arguments_[0] == 0x55 || arguments_[0] == 0x05;
        if (!format_)
            error_ = "Only RGB565 TFT pixel transfers are modeled";
    }
    if (command_ == 0x36 && arguments_.size() == 1) {
        madctl_ = arguments_[0];
        ++revision_;
    }
    if ((command_ == 0x2a || command_ == 0x2b) && arguments_.size() == 4) {
        const unsigned start = word(0), end = word(2);
        const unsigned bound =
            command_ == 0x2a ? ((madctl_ & 32) ? height_ : width_) : ((madctl_ & 32) ? width_ : height_);
        if (start > end || end >= bound) {
            error_ = "TFT address window is outside the panel";
            if (command_ == 0x2a)
                xValid_ = false;
            else
                yValid_ = false;
            return;
        }
        if (command_ == 0x2a) {
            x0_ = start;
            x1_ = end;
            xValid_ = true;
        } else {
            y0_ = start;
            y1_ = end;
            yValid_ = true;
        }
    }
    if (command_ == 0x33 && arguments_.size() == 6) {
        auto top = word(0), area = word(2), bottom = word(4);
        if (top + area + bottom == unsigned(height_) && area) {
            top_ = top;
            bottom_ = bottom;
            if (scroll_ < top_ || scroll_ >= unsigned(height_) - bottom_)
                scroll_ = top_;
            ++revision_;
        } else
            error_ = "Invalid TFT scrolling area";
    }
    if (command_ == 0x37 && arguments_.size() == 2) {
        const auto start = word(0);
        if (start >= top_ && start < unsigned(height_) - bottom_) {
            scroll_ = start;
            ++revision_;
        } else
            error_ = "Invalid TFT scroll start";
    }
}
std::uint8_t TftController::transfer(bool data, std::uint8_t value, Microseconds now) {
    if (!width_ || !height_)
        return 0xff;
    if (!data) {
        command_ = value;
        arguments_.clear();
        half_ = false;
        read_ = 0;
        if (value == 0x01) {
            reset();
            ready_ = now + 120000;
        } else if (value == 0x11) {
            sleep_ = false;
            ready_ = now + 120000;
            ++revision_;
        } else if (value == 0x10) {
            sleep_ = true;
            ++revision_;
        } else if (value == 0x29) {
            on_ = true;
            ++revision_;
        } else if (value == 0x28) {
            on_ = false;
            ++revision_;
        } else if (value == 0x20 || value == 0x21) {
            invert_ = value == 0x21;
            ++revision_;
        } else if (value == 0x2c || value == 0x2e) {
            x_ = x0_;
            y_ = y0_;
        }
        return 0xff;
    }
    if (command_ == 0x04) {
        const std::uint8_t ili[] = {0, 0x93, 0x41}, st[] = {0x7c, 0x89, 0xf0};
        return read_ < 3 ? (ili_ ? ili[read_++] : st[read_++]) : 0;
    }
    if (command_ == 0x2c) {
        if (!format_ || sleep_ || now < ready_ || !xValid_ || !yValid_) {
            error_ = "TFT is not initialized or is still waking up";
            return 0xff;
        }
        if (!half_) {
            high_ = value;
            half_ = true;
            return 0xff;
        }
        half_ = false;
        int px = int(x_), py = int(y_);
        if (madctl_ & 32)
            std::swap(px, py);
        if (madctl_ & 64)
            px = width_ - 1 - px;
        if (madctl_ & 128)
            py = height_ - 1 - py;
        if (px >= 0 && px < width_ && py >= 0 && py < height_) {
            pixels_[py * width_ + px] = std::uint16_t(high_) * 256 + value;
            ++revision_;
        }
        next();
        return 0xff;
    }
    if (arguments_.size() < 16) {
        arguments_.push_back(value);
        parameter();
    }
    return 0xff;
}
std::shared_ptr<const ColorFrame> TftController::frame(Microseconds now, double brightness) {
    if (sleep_ || !on_ || !format_ || now < ready_ || brightness <= 0)
        return {};
    brightness = std::clamp(brightness, 0., 1.);
    if (cached_ && cachedRevision_ == revision_ && cached_->brightness == brightness)
        return cached_;
    auto frame = std::make_shared<ColorFrame>();
    frame->width = width_;
    frame->height = height_;
    frame->brightness = brightness;
    frame->pixels.resize(pixels_.size());
    for (int y = 0; y < height_; ++y)
        for (int x = 0; x < width_; ++x) {
            int source = y;
            if (y >= int(top_) && y < height_ - int(bottom_)) {
                const int area = height_ - int(top_ + bottom_);
                source = int(top_) + (y - int(top_) + int(scroll_) - int(top_)) % area;
            }
            unsigned value = pixels_[source * width_ + x];
            if (invert_)
                value ^= 0xffff;
            unsigned r = (value >> 11) & 31, g = (value >> 5) & 63, b = value & 31;
            if (!(madctl_ & 8))
                std::swap(r, b);
            frame->pixels[y * width_ + x] =
                0xff000000u | ((r * 255 / 31) << 16) | ((g * 255 / 63) << 8) | (b * 255 / 31);
        }
    cached_ = frame;
    cachedRevision_ = revision_;
    return frame;
}
} // namespace thinkercad
