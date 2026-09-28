# Assignment — Session: 11
**Deadline: 2026-09-27 23:59:00**

---

## Exercise_1 — A Stopwatch Driven by a Hardware Timer [review-only]

### Problem Statement

`vTaskDelay()` has carried you a long way, but it only promises that *at least* that much time has passed. It says nothing about exactly when you wake up, and the error adds up. Anything that has to keep time (a stopwatch, a sampling loop, a motor controller) needs a hardware timer that ticks independently of what the scheduler is doing.

Build a stopwatch:

1. Configure a `gptimer` that raises an alarm **every 1 ms**. The alarm callback does one thing: increment a millisecond counter.
2. The onboard **BOOT** button controls it. A short press starts or pauses the stopwatch. Holding the button for **1 s or longer** resets it to zero.
3. A task prints the elapsed time as `mm:ss.mmm` every 100 ms.
4. While the stopwatch is running, an LED blinks at 1 Hz. While it is paused, the LED is off.

Requirements:
- Use `driver/gptimer.h`. The alarm must use **auto-reload**, so the timer rearms itself in hardware. Do not rearm it from the callback.
- The alarm callback runs in interrupt context. It may only increment the counter and return: no `ESP_LOGI`, no `printf`, no delays, no GPIO work.
- The counter shared between the callback and your tasks must be `volatile`.
- The elapsed time must come **only** from the `gptimer` counter. Do not accumulate `vTaskDelay()` periods and do not read `esp_timer_get_time()`.
- Pausing must not lose or invent time. Start, pause, start again: the total must be the sum of the running periods.
- A long press must reset **without** also toggling start/pause. Work out when a short press should be recognised so that it cannot be confused with a long one.
- The button runs in a task, not in the timer callback. Debounce it.
- A reset leaves the stopwatch **paused at zero**.
- Resetting means a task writes the counter while the timer callback may be incrementing it at the same moment, possibly on the other core. `volatile` makes the value visible to both sides, but it does not make `elapsed_ms++` atomic. Make sure a reset can never be lost to that collision, and be able to explain how your code guarantees it.
- Every period, pin and duration must be a named constant.
- Check the return value of every `gptimer_*` call. Use `ESP_ERROR_CHECK` for init only.

### Hardware

ESP32-S3 DevKitC-1 plus one LED. The display module stays unplugged.

| Part | ESP32-S3 | Notes |
| --- | --- | --- |
| BOOT button (onboard) | GPIO0 | pressed = LOW. It is a strapping pin, so it is fine to read after boot, but do not reconfigure it as an output |
| LED anode, via 220 Ω–330 Ω resistor | GPIO15 | cathode to GND, same as Session 10 |

### Design Hints

```c
#include "driver/gpio.h"
#include "driver/gptimer.h"

#define BTN_PIN            GPIO_NUM_0
#define LED_PIN            GPIO_NUM_15

#define TIMER_RESOLUTION   (1000000U)   /* 1 MHz -> 1 tick == 1 us */
#define TIMER_ALARM_TICKS  (1000ULL)    /* 1 ms at that resolution */
#define PRINT_PERIOD_MS    (100U)
#define LONG_PRESS_MS      (1000U)
#define DEBOUNCE_MS        (20U)
#define LED_HALF_PERIOD_MS (500U)       /* 1 Hz blink */

static volatile uint32_t elapsed_ms = 0;

static bool IRAM_ATTR on_alarm(gptimer_handle_t timer, const gptimer_alarm_event_data_t* edata, void* user_ctx)
{
    elapsed_ms++;
    return false; /* what does this return value control? Look it up. */
}

gptimer_config_t timer_cfg = {
    .clk_src       = GPTIMER_CLK_SRC_DEFAULT,
    .direction     = GPTIMER_COUNT_UP,
    .resolution_hz = TIMER_RESOLUTION,
};

gptimer_alarm_config_t alarm_cfg = {
    .alarm_count                = TIMER_ALARM_TICKS,
    .reload_count               = 0,
    .flags.auto_reload_on_alarm = true,
};
```

`gptimer` has a strict lifecycle: create, set the alarm action, register the callback, **enable**, then start. Skipping `gptimer_enable()` is the usual reason a timer silently never fires. For pause and resume, look at what `gptimer_stop()` and `gptimer_start()` do to the count and to your callback, and decide whether they are all you need.

When the print task formats `mm:ss.mmm`, read `elapsed_ms` **once** into a local variable and do all the arithmetic on that copy. Think about what could go wrong if you read the shared variable three times for minutes, seconds and milliseconds.

The LED blink can be derived from the same counter. You do not need a second timer.

### Suggested Approach

```
app_main():
  1. LED_PIN as output, BTN_PIN as input with pull-up
  2. gptimer: new -> set_alarm_action -> register_event_callbacks -> enable
     (do not start yet: the stopwatch begins paused)
  3. create the print task and the button task

print task, every PRINT_PERIOD_MS:
  ms = elapsed_ms
  print mm:ss.mmm
  if running: LED = (ms / LED_HALF_PERIOD_MS) % 2   else: LED off

button task, every few ms:
  debounce the level
  on press:   remember when it started
  while held: if held >= LONG_PRESS_MS and not yet handled -> reset to 0
  on release: if it was a short press -> toggle start/pause
```

### Expected Output

```
I (1210) WATCH: 00:00.000  [paused]
I (1310) WATCH: 00:00.000  [paused]
I (1402) WATCH: started
I (1410) WATCH: 00:00.008
I (1510) WATCH: 00:00.108
...
I (61410) WATCH: 01:00.008
I (61430) WATCH: paused at 01:00.028
I (65000) WATCH: started
I (65010) WATCH: 01:00.038
...
I (70120) WATCH: reset to 00:00.000
```

Let it run for one minute next to the stopwatch on your phone. Starting two stopwatches by hand is only accurate to a couple of tenths of a second, so that is as close as the two can agree. That is still enough to catch the real mistakes: a wrong resolution or alarm count shows up as a **gross** error (the display running twice as fast, or ten times too slow), not as a few milliseconds. The hardware timer itself drifts by only a few milliseconds per minute, far below what you can see by hand. Pause it for a few seconds, resume, and check that the paused time was not counted.

### Submission

```
Exercise_1/
├── main/
│   ├── main.c              (required — gptimer 1 ms tick, start/pause/reset on BOOT)
│   └── CMakeLists.txt      (required)
└── CMakeLists.txt          (required — ESP-IDF project root)
```
