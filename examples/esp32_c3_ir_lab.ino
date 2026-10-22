#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <IRremote.hpp>
// ESP32-C3: four independent resistor-limited IR LEDs and four momentary buttons.
LiquidCrystal_I2C lcd(0x27, 16, 2);
const uint8_t LED_PINS[] = {2, 4, 5, 6};
const uint8_t BUTTON_PINS[] = {0, 1, 3, 10};
const uint8_t COMMANDS[] = {0x45, 0x46, 0x47, 0x44};
bool wasPressed[4] = {};
enum class Channel : uint8_t { One, Two, Three, Four };
void setup() {
    Serial.begin(115200);
    Wire.begin(8, 9);
    lcd.init();
    lcd.backlight();
    uint8_t glyph[] = {B00100,B01110,B11111,B00100,B00100,B00100,B00100,B00000};
    lcd.createChar(0, glyph);
    lcd.setCursor(0, 0);
    lcd.print("ESP32-C3 IR lab");
    for (uint8_t pin : BUTTON_PINS) pinMode(pin, INPUT_PULLUP);
    IrReceiver.begin(7, DISABLE_LED_FEEDBACK);
    transmit(Channel::One); // Startup self-test; subsequently press any button.
}
void transmit(Channel channel) {
    uint8_t index = static_cast<uint8_t>(channel);
    IrSender.begin(LED_PINS[index], DISABLE_LED_FEEDBACK);
    IrSender.sendNEC(0x1234, COMMANDS[index], 0);
}
void loop() {
    for (uint8_t i = 0; i < 4; ++i) {
        bool pressed = digitalRead(BUTTON_PINS[i]) == LOW;
        if (pressed && !wasPressed[i]) transmit(static_cast<Channel>(i));
        wasPressed[i] = pressed;
    }
    if (IrReceiver.decode()) {
        lcd.setCursor(0, 1);
        lcd.print("NEC 0x");
        lcd.print(IrReceiver.decodedIRData.command, HEX);
        lcd.print("       ");
        Serial.print("C3 IR decoded: 0x");
        Serial.println(IrReceiver.decodedIRData.command, HEX);
        IrReceiver.resume();
    }
    delay(10);
}
