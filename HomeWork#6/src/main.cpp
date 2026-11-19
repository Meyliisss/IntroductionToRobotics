#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "ELEVATOR";

static const int NUM_FLOORS = 3;

static const gpio_num_t BUTTON_PINS[NUM_FLOORS + 1] = {GPIO_NUM_NC, GPIO_NUM_18, GPIO_NUM_19, GPIO_NUM_21};
static const gpio_num_t REQUEST_LED_PINS[NUM_FLOORS + 1] = {GPIO_NUM_NC, GPIO_NUM_26, GPIO_NUM_25, GPIO_NUM_33};

static const gpio_num_t PIN_MOVING_LED = GPIO_NUM_27;
static const gpio_num_t PIN_BUZZER = GPIO_NUM_17;

static const gpio_num_t PIN_SR_DATA = GPIO_NUM_23;   // 74HC595 DS   (pin 14)
static const gpio_num_t PIN_SR_LATCH = GPIO_NUM_22;  // 74HC595 STCP (pin 12)
static const gpio_num_t PIN_SR_CLOCK = GPIO_NUM_16;  // 74HC595 SHCP (pin 11)

static const uint32_t LOOP_PERIOD_MS = 10;       // main loop tick
static const uint32_t DEBOUNCE_MS = 50;          // button must be stable this long
static const uint32_t DOORS_OPEN_MIN_MS = 2000;  // doors stay open at least this long
static const uint32_t DOOR_CLOSING_MS = 1500;    // simulated door closing
static const uint32_t FLOOR_TRAVEL_MS = 2000;    // travel time between adjacent floors
static const uint32_t DOOR_OPENING_MS = 1000;    // simulated door opening after arrival
static const uint32_t MOVING_BLINK_MS = 250;     // movement LED half-period

static const uint8_t SEG_A = 1 << 7;   // Q7
static const uint8_t SEG_B = 1 << 6;   // Q6
static const uint8_t SEG_C = 1 << 5;   // Q5
static const uint8_t SEG_D = 1 << 4;   // Q4
static const uint8_t SEG_E = 1 << 3;   // Q3
static const uint8_t SEG_F = 1 << 2;   // Q2
static const uint8_t SEG_G = 1 << 1;   // Q1
static const uint8_t SEG_DP = 1 << 0;  // Q0 (unused, kept for completeness)

static const uint8_t DIGIT_PATTERNS[NUM_FLOORS + 1] = {
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,  // 0 (not used as a floor)
    SEG_B | SEG_C,                                  // 1
    SEG_A | SEG_B | SEG_D | SEG_E | SEG_G,          // 2
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_G,          // 3
};

enum ElevatorState {
    STATE_DOORS_OPEN,
    STATE_DOORS_CLOSING,
    STATE_MOVING,
    STATE_DOORS_OPENING,  // arriving: arrival sound + doors opening
};

static const char *stateName(ElevatorState state) {
    switch (state) {
        case STATE_DOORS_OPEN:    return "DOORS_OPEN";
        case STATE_DOORS_CLOSING: return "DOORS_CLOSING";
        case STATE_MOVING:        return "MOVING";
        case STATE_DOORS_OPENING: return "DOORS_OPENING";
    }
    return "UNKNOWN";
}

struct Elevator {
    ElevatorState state;
    int currentFloor;
    int destinationFloor;
    int direction;            // +1 up, -1 down, 0 stopped
    uint32_t stateStartMs;    // when we entered the current state (or the current floor leg)
};

static Elevator elevator;

static inline uint32_t nowMs() {
    return (uint32_t)(esp_timer_get_time() / 1000);
}

static void shiftOutByte(uint8_t value) {
    gpio_set_level(PIN_SR_LATCH, 0);
    for (int bit = 7; bit >= 0; --bit) {
        gpio_set_level(PIN_SR_DATA, (value >> bit) & 0x01);
        gpio_set_level(PIN_SR_CLOCK, 1);
        gpio_set_level(PIN_SR_CLOCK, 0);
    }
    gpio_set_level(PIN_SR_LATCH, 1);
    gpio_set_level(PIN_SR_LATCH, 0);
}

static void showFloor(int floor) {
    if (floor < 1 || floor > NUM_FLOORS) {
        shiftOutByte(0x00);
        return;
    }
    shiftOutByte(DIGIT_PATTERNS[floor] & (uint8_t)~SEG_DP);
}

static const ledc_mode_t BUZZER_SPEED_MODE = LEDC_LOW_SPEED_MODE;
static const ledc_timer_t BUZZER_TIMER = LEDC_TIMER_0;
static const ledc_channel_t BUZZER_CHANNEL = LEDC_CHANNEL_0;
static const ledc_timer_bit_t BUZZER_RESOLUTION = LEDC_TIMER_10_BIT;
static const uint32_t BUZZER_DUTY_ON = 512;  // 50% of 1024 -> loudest square wave

struct Note {
    uint32_t frequencyHz;  // 0 = silence
    uint32_t durationMs;
};

static const Note DOOR_CLOSING_SOUND[] = {
    {880, 120}, {0, 80}, {880, 120},
};

static const Note ARRIVAL_SOUND[] = {
    {1319, 250}, {0, 40}, {1047, 400},
};

struct SoundPlayer {
    const Note *notes;
    size_t noteCount;
    size_t noteIndex;
    uint32_t noteStartMs;
    bool active;
};

static SoundPlayer soundPlayer = {nullptr, 0, 0, 0, false};

static void buzzerSilence() {
    ledc_set_duty(BUZZER_SPEED_MODE, BUZZER_CHANNEL, 0);
    ledc_update_duty(BUZZER_SPEED_MODE, BUZZER_CHANNEL);
}

static void buzzerTone(uint32_t frequencyHz) {
    if (frequencyHz == 0) {
        buzzerSilence();
        return;
    }
    ledc_set_freq(BUZZER_SPEED_MODE, BUZZER_TIMER, frequencyHz);
    ledc_set_duty(BUZZER_SPEED_MODE, BUZZER_CHANNEL, BUZZER_DUTY_ON);
    ledc_update_duty(BUZZER_SPEED_MODE, BUZZER_CHANNEL);
}

static void playSound(const Note *notes, size_t noteCount, uint32_t now) {
    soundPlayer.notes = notes;
    soundPlayer.noteCount = noteCount;
    soundPlayer.noteIndex = 0;
    soundPlayer.noteStartMs = now;
    soundPlayer.active = true;
    buzzerTone(notes[0].frequencyHz);
}

static void updateSound(uint32_t now) {
    if (!soundPlayer.active) {
        return;
    }
    const Note &note = soundPlayer.notes[soundPlayer.noteIndex];
    if (now - soundPlayer.noteStartMs < note.durationMs) {
        return;
    }
    soundPlayer.noteIndex++;
    if (soundPlayer.noteIndex >= soundPlayer.noteCount) {
        soundPlayer.active = false;
        buzzerSilence();
        return;
    }
    soundPlayer.noteStartMs = now;
    buzzerTone(soundPlayer.notes[soundPlayer.noteIndex].frequencyHz);
}

static bool movingLedOn = false;
static uint32_t movingLedLastToggleMs = 0;

static void setMovingLed(bool on) {
    movingLedOn = on;
    gpio_set_level(PIN_MOVING_LED, on ? 1 : 0);
}

static void updateMovingLed(uint32_t now) {
    if (elevator.state != STATE_MOVING) {
        if (movingLedOn) {
            setMovingLed(false);
        }
        return;
    }
    if (now - movingLedLastToggleMs >= MOVING_BLINK_MS) {
        movingLedLastToggleMs = now;
        setMovingLed(!movingLedOn);
    }
}

static int requestQueue[NUM_FLOORS];
static int requestCount = 0;
static bool floorPending[NUM_FLOORS + 1] = {false, false, false, false};

static void setRequestLed(int floor, bool on) {
    gpio_set_level(REQUEST_LED_PINS[floor], on ? 1 : 0);
}

static void logQueue() {
    char text[32];
    int length = snprintf(text, sizeof(text), "[");
    for (int i = 0; i < requestCount; ++i) {
        length += snprintf(text + length, sizeof(text) - length, i == 0 ? "%d" : ", %d", requestQueue[i]);
    }
    snprintf(text + length, sizeof(text) - length, "]");
    ESP_LOGI(TAG, "Requests: %s", text);
}

static bool enqueueRequest(int floor) {
    if (requestCount >= NUM_FLOORS) {
        ESP_LOGE(TAG, "Request queue full, Floor %d dropped", floor);
        return false;
    }
    requestQueue[requestCount++] = floor;
    floorPending[floor] = true;
    setRequestLed(floor, true);
    return true;
}

static bool removeRequest(int floor) {
    for (int i = 0; i < requestCount; ++i) {
        if (requestQueue[i] == floor) {
            for (int j = i; j < requestCount - 1; ++j) {
                requestQueue[j] = requestQueue[j + 1];
            }
            requestCount--;
            floorPending[floor] = false;
            setRequestLed(floor, false);
            return true;
        }
    }
    return false;
}

static void setState(ElevatorState newState, uint32_t now) {
    elevator.state = newState;
    elevator.stateStartMs = now;
    ESP_LOGI(TAG, "State: %s", stateName(newState));

    if (newState == STATE_MOVING) {
        movingLedLastToggleMs = now;
        setMovingLed(true);  // start blinking immediately
    }
}

static void completeRequestAtCurrentFloor() {
    if (removeRequest(elevator.currentFloor)) {
        ESP_LOGI(TAG, "Request for Floor %d completed", elevator.currentFloor);
        logQueue();
    }
}

static void handleFloorRequest(int floor, uint32_t now) {
    ESP_LOGI(TAG, "Floor %d requested", floor);

    bool atThisFloor = (floor == elevator.currentFloor);

    if (atThisFloor && elevator.state == STATE_DOORS_OPEN) {
        ESP_LOGW(TAG, "Already at Floor %d with doors open - request satisfied", floor);
        elevator.stateStartMs = now;
        return;
    }

    if (floorPending[floor]) {
        ESP_LOGW(TAG, "Floor %d is already pending - duplicate ignored", floor);
        return;
    }

    if (atThisFloor && elevator.state == STATE_DOORS_OPENING) {
        ESP_LOGW(TAG, "Doors are already opening at Floor %d - request satisfied", floor);
        return;
    }

    if (!enqueueRequest(floor)) {
        return;
    }
    logQueue();

    if (atThisFloor && elevator.state == STATE_DOORS_CLOSING) {
        ESP_LOGW(TAG, "Floor %d requested while doors closing - reopening doors", floor);
        setState(STATE_DOORS_OPENING, now);
    }
}

struct Button {
    bool lastRawPressed;     // last raw reading
    bool stablePressed;      // debounced state
    uint32_t lastChangeMs;   // when the raw reading last changed
};

static Button buttons[NUM_FLOORS + 1];

static void updateButtons(uint32_t now) {
    for (int floor = 1; floor <= NUM_FLOORS; ++floor) {
        Button &button = buttons[floor];
        bool rawPressed = (gpio_get_level(BUTTON_PINS[floor]) == 0);  // active LOW

        if (rawPressed != button.lastRawPressed) {
            button.lastRawPressed = rawPressed;
            button.lastChangeMs = now;
            continue;
        }

        if (now - button.lastChangeMs >= DEBOUNCE_MS && rawPressed != button.stablePressed) {
            button.stablePressed = rawPressed;
            if (rawPressed) {
                handleFloorRequest(floor, now);
            }
        }
    }
}

static void updateElevator(uint32_t now) {
    uint32_t elapsed = now - elevator.stateStartMs;

    switch (elevator.state) {
        case STATE_DOORS_OPEN: {
            if (requestCount == 0 || elapsed < DOORS_OPEN_MIN_MS) {
                break;
            }
            int nextFloor = requestQueue[0];  // FIFO: oldest request first
            if (nextFloor == elevator.currentFloor) {
                completeRequestAtCurrentFloor();
                elevator.stateStartMs = now;
                break;
            }
            elevator.destinationFloor = nextFloor;
            elevator.direction = (nextFloor > elevator.currentFloor) ? 1 : -1;
            ESP_LOGI(TAG, "Destination: Floor %d", nextFloor);
            playSound(DOOR_CLOSING_SOUND, sizeof(DOOR_CLOSING_SOUND) / sizeof(DOOR_CLOSING_SOUND[0]), now);
            setState(STATE_DOORS_CLOSING, now);
            break;
        }

        case STATE_DOORS_CLOSING:
            if (elapsed >= DOOR_CLOSING_MS) {
                setState(STATE_MOVING, now);
                ESP_LOGI(TAG, "Moving %s from Floor %d", elevator.direction > 0 ? "up" : "down",
                         elevator.currentFloor);
            }
            break;

        case STATE_MOVING:
            if (elapsed < FLOOR_TRAVEL_MS) {
                break;
            }
            elevator.currentFloor += elevator.direction;
            showFloor(elevator.currentFloor);
            ESP_LOGI(TAG, "Reached Floor %d", elevator.currentFloor);

            if (elevator.currentFloor == elevator.destinationFloor) {
                ESP_LOGI(TAG, "Arrived at Floor %d", elevator.currentFloor);
                elevator.direction = 0;
                playSound(ARRIVAL_SOUND, sizeof(ARRIVAL_SOUND) / sizeof(ARRIVAL_SOUND[0]), now);
                setState(STATE_DOORS_OPENING, now);
            } else {
                elevator.stateStartMs = now;  // start the next floor leg
            }
            break;

        case STATE_DOORS_OPENING:
            if (elapsed >= DOOR_OPENING_MS) {
                completeRequestAtCurrentFloor();  // LED turns off once the doors are open
                setState(STATE_DOORS_OPEN, now);
            }
            break;
    }
}

static void initOutputs() {
    gpio_config_t config = {};
    config.pin_bit_mask = (1ULL << PIN_MOVING_LED) | (1ULL << PIN_SR_DATA) | (1ULL << PIN_SR_LATCH) |
                          (1ULL << PIN_SR_CLOCK);
    for (int floor = 1; floor <= NUM_FLOORS; ++floor) {
        config.pin_bit_mask |= (1ULL << REQUEST_LED_PINS[floor]);
    }
    config.mode = GPIO_MODE_OUTPUT;
    config.pull_up_en = GPIO_PULLUP_DISABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&config));

    gpio_set_level(PIN_SR_DATA, 0);
    gpio_set_level(PIN_SR_CLOCK, 0);
    gpio_set_level(PIN_SR_LATCH, 0);
    setMovingLed(false);
    for (int floor = 1; floor <= NUM_FLOORS; ++floor) {
        setRequestLed(floor, false);
    }
}

static void initButtons() {
    gpio_config_t config = {};
    for (int floor = 1; floor <= NUM_FLOORS; ++floor) {
        config.pin_bit_mask |= (1ULL << BUTTON_PINS[floor]);
    }
    config.mode = GPIO_MODE_INPUT;
    config.pull_up_en = GPIO_PULLUP_ENABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&config));

    uint32_t now = nowMs();
    for (int floor = 1; floor <= NUM_FLOORS; ++floor) {
        bool pressed = (gpio_get_level(BUTTON_PINS[floor]) == 0);
        buttons[floor] = {pressed, pressed, now};  // a button held at boot does not count as a press
    }
}

static void initBuzzer() {
    ledc_timer_config_t timerConfig = {};
    timerConfig.speed_mode = BUZZER_SPEED_MODE;
    timerConfig.duty_resolution = BUZZER_RESOLUTION;
    timerConfig.timer_num = BUZZER_TIMER;
    timerConfig.freq_hz = 1000;
    timerConfig.clk_cfg = LEDC_AUTO_CLK;
    ESP_ERROR_CHECK(ledc_timer_config(&timerConfig));

    ledc_channel_config_t channelConfig = {};
    channelConfig.gpio_num = PIN_BUZZER;
    channelConfig.speed_mode = BUZZER_SPEED_MODE;
    channelConfig.channel = BUZZER_CHANNEL;
    channelConfig.intr_type = LEDC_INTR_DISABLE;
    channelConfig.timer_sel = BUZZER_TIMER;
    channelConfig.duty = 0;  // silent at start
    channelConfig.hpoint = 0;
    ESP_ERROR_CHECK(ledc_channel_config(&channelConfig));
}

extern "C" void app_main(void) {
    initOutputs();
    initButtons();
    initBuzzer();

    uint32_t now = nowMs();
    elevator.currentFloor = 1;
    elevator.destinationFloor = 1;
    elevator.direction = 0;
    showFloor(elevator.currentFloor);

    ESP_LOGI(TAG, "Initialized at Floor %d", elevator.currentFloor);
    setState(STATE_DOORS_OPEN, now);
    logQueue();

    TickType_t loopDelay = pdMS_TO_TICKS(LOOP_PERIOD_MS);
    if (loopDelay == 0) {
        loopDelay = 1;  // always yield at least one tick so the idle task / watchdog stay happy
    }

    while (true) {
        now = nowMs();
        updateButtons(now);
        updateElevator(now);
        updateMovingLed(now);
        updateSound(now);
        vTaskDelay(loopDelay);
    }
}
