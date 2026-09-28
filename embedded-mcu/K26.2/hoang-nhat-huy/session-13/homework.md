# Assignment — Session: 13
**Deadline: 2026-10-04 23:59:00**

---

## Exercise_1 — A Deep-Sleep Doorbell [review-only]

### Problem Statement

A battery doorbell spends almost its whole life doing nothing. If it stayed awake waiting for the button, the battery would last days. If it sleeps and only wakes when needed, it lasts months. On the ESP32-S3 the deepest useful sleep switches off the CPUs and almost all RAM. Waking from it is a reboot, and only what you deliberately kept in RTC memory remembers what happened before.

Build a doorbell that lives in deep sleep:

1. The board sits in **deep sleep**. Pressing **BOOT** wakes it through **ext0** on GPIO0, level 0.
2. On a button wake: blink the LED 3 times, increment a **press counter** kept in RTC memory, log it, and go back to sleep.
3. If nobody presses for **60 s**, a timer wakes the board. It logs a **heartbeat** line with the total press count and goes back to sleep.
4. On a fresh power-on, log that too, and go to sleep.

Requirements:
- Use `esp_sleep.h`: `esp_sleep_enable_ext0_wakeup()` for the button, `esp_sleep_enable_timer_wakeup()` for the heartbeat, `esp_deep_sleep_start()` to sleep.
- On every boot, read `esp_sleep_get_wakeup_cause()` and branch on it. Button, timer and power-on must each produce their own, clearly different log line.
- The press counter must be declared with `RTC_DATA_ATTR`. Only a button wake increments it. A heartbeat only reports it.
- After an ext0 wake, the wake pin is still configured as an **RTC IO**, not a normal GPIO. Hand it back to the digital side with `rtc_gpio_deinit()` before you read it with `gpio_get_level()`, or the read is meaningless.
- Before going back to sleep, **wait until the button has been released**. ext0 wakes on a level, not an edge. If you sleep while the button is still held LOW, the board wakes again immediately and counts a press that never happened.
- The pull-up you might set with `gpio_config()` belongs to the digital domain and is switched off in deep sleep. In deep sleep GPIO0 is an RTC pin. Arm its pull-up with the RTC GPIO API (`driver/rtc_io.h`) before sleeping, so the pin cannot float.
- Every timeout, pin and blink count must be a named constant.
- Watch the log through the **UART** port, not the native USB port (see Monitoring below).

### Hardware

| Part | ESP32-S3 | Notes |
| --- | --- | --- |
| BOOT button (onboard) | GPIO0 | pressed = LOW, RTC-capable, so ext0 can use it |
| LED anode, via 220 Ω–330 Ω resistor | GPIO15 | cathode to GND, same as Sessions 10 and 11 |

The display module stays unplugged.

**Monitoring:** deep sleep switches off the USB Serial/JTAG peripheral, so a monitor on the native **USB** port loses the connection on every sleep and misses the first lines after each wake. The **UART** port goes through the CP2102N bridge chip, which stays powered from USB, so the log survives. Use the two-port arrangement from Session 05, but with the console on UART0. That is the ESP-IDF default. Session 05 moved it to USB Serial/JTAG, so check this project's menuconfig before you start.

**Reflashing a sleeping board:** `idf.py flash` resets the chip over DTR/RTS and normally gets through even while it sleeps. If it ever does not, hold **BOOT**, tap **RESET**, release **BOOT**, then flash again.

### Design Hints

```c
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_sleep.h"

#define BTN_PIN            GPIO_NUM_0
#define BTN_WAKE_LEVEL     (0)              /* pressed = LOW */
#define LED_PIN            GPIO_NUM_15
#define BLINK_COUNT        (3U)
#define BLINK_HALF_MS      (150U)
#define HEARTBEAT_US       (60ULL * 1000000ULL)

RTC_DATA_ATTR static uint32_t press_count = 0;

static const char* wakeup_cause_name(esp_sleep_wakeup_cause_t cause)
{
    switch (cause)
    {
    case ESP_SLEEP_WAKEUP_EXT0:
        return "BUTTON";
    case ESP_SLEEP_WAKEUP_TIMER:
        return "HEARTBEAT";
    default:
        return "POWER_ON / RESET";
    }
}
```

`esp_deep_sleep_start()` never returns. Everything the program does happens in `app_main()`, from the top, on every wake. Write it as "boot, look at why I woke, act, sleep again".

Session 12 used `RTC_NOINIT_ATTR`. Here it is `RTC_DATA_ATTR`. Both live in RTC memory. Find out what differs between them on power-on, and why this counter does not need a magic number.

### Suggested Approach

```
app_main():
  1. cause = esp_sleep_get_wakeup_cause()
  2. switch cause:
       BUTTON:    blink LED BLINK_COUNT times, press_count++, log "ding-dong #N"
       HEARTBEAT: log "heartbeat, total presses = N"
       default:   log "power-on, doorbell armed"
  3. rtc_gpio_deinit(BTN_PIN), configure it as a digital input,
     then wait until it reads HIGH (released)
  4. arm the RTC pull-up on BTN_PIN
     esp_sleep_enable_ext0_wakeup(BTN_PIN, BTN_WAKE_LEVEL)
     esp_sleep_enable_timer_wakeup(HEARTBEAT_US)
  5. log "sleeping", esp_deep_sleep_start()
     /* nothing below this line ever runs */
```

### Expected Output

```
I (298) BELL: power-on, doorbell armed. presses=0
I (305) BELL: sleeping
I (297) BELL: ding-dong #1
I (1210) BELL: sleeping
I (297) BELL: ding-dong #2
I (1210) BELL: sleeping
I (296) BELL: heartbeat, total presses=2
I (303) BELL: sleeping
```

The timestamp restarts near zero on every wake because the chip really did reboot. The press count keeps climbing across sleeps and returns to 0 after you unplug the board. Holding the button down for a few seconds counts as **one** press, not several.

A multimeter in series with the USB supply shows the current dropping sharply while the board sleeps. This measurement is optional and not graded. Do not expect microamps: on a DevKitC-1 the power LED, the regulator and the USB-UART bridge keep drawing current, so the floor you see is set by the board, not by the chip.

### Submission

```
Exercise_1/
├── main/
│   ├── main.c              (required — deep sleep, ext0 + timer wakeup, RTC_DATA_ATTR counter)
│   └── CMakeLists.txt      (required)
└── CMakeLists.txt          (required — ESP-IDF project root)
```
