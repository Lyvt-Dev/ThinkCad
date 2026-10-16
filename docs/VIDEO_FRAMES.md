# Video to frames.h (ThinkCad 1.2.1)

Use **File → Convert video to frames.h…** (also available in Settings).
Choose a local video, then select 128×64 or 128×32, 1–30 frames/second,
start time, clip duration, threshold and optional inversion. Export chooses the
output path with a standard save dialog. Existing files are replaced only after
a successful conversion; closing the dialog cancels conversion.

The converter uses Qt Multimedia's installed video backend. MP4, WebM, MKV,
MOV, AVI and animated GIF are offered; actual codec availability depends on the
platform/backend. Audio is not exported. The importer does not execute sketches
or use an external ffmpeg executable. Conversion follows the clip's playback
clock, so a 10-second clip takes about 10 seconds. Letterboxing preserves the
source aspect ratio. Sampling uses presentation timestamps; low-frame-rate
sources repeat their last displayed frame.

The limit is **256 KiB of packed bitmap data**. At 128×64, a frame takes 1,024
bytes, allowing up to 256 frames (25.6 seconds at 10 FPS). The generated C++ text
is larger than the bitmap data, but remains within ThinkCad's 2 MiB sketch-input
limit. Choose fewer FPS, a shorter clip or 128×32 when the limit is exceeded.
The dialog shows the frame count and bitmap size before export.

Copy `frames.h` into the Arduino sketch folder. Open/reopen that `.ino` in
ThinkCad so the header is loaded as an auxiliary tab. The output is constant
`PROGMEM` data, with rows packed MSB first for Adafruit GFX `drawBitmap`:

```cpp
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "frames.h"
Adafruit_SSD1306 display(FRAME_WIDTH, FRAME_HEIGHT, &Wire, -1);
void setup() {
    Wire.begin(D2, D1); // ESP8266: SDA, SCL
    display.begin(SSD1306_SWITCHCAPVCC, 0x3c);
}
void loop() {
    const uint32_t i = (uint64_t(micros()) * FRAME_FPS / 1000000) % FRAME_COUNT;
    display.clearDisplay();
    display.drawBitmap(0, 0, frames[i], FRAME_WIDTH, FRAME_HEIGHT, SSD1306_WHITE);
    display.display();
    delay(1);
}
```

Connect OLED VCC to 3V3 and GND to GND. Use the matching display height/model.
I²C transfer time limits achievable refresh rate; selecting 30 FPS does not
remove the physical bus limit. Final firmware flash usage also includes code and
libraries. The converter's bitmap limit is not a promise that every target's
firmware will fit.
