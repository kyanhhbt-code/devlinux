# Assignment — Session: 12
**Deadline: 2026-10-04 23:59:00**

---

## Exercise_1 — Two Tasks Under a Watchdog [review-only]

### Problem Statement

Field devices hang. A task waits on a lock that is never released, or a sensor stops answering and a loop spins forever. Nobody is there to press reset. The Task Watchdog Timer (TWDT) exists so the device notices that one of its tasks has stopped making progress and reboots itself. After the reboot, the firmware should be able to tell *that* it was reset by the watchdog, and how often it has happened.

Build a program with two watched tasks and a way to hang one of them on purpose:

1. Task `sensor` runs every 200 ms and task `report` runs every 1 s. **Each task subscribes itself** to the TWDT and feeds it once per iteration.
2. Configure the TWDT with `esp_task_wdt_reconfigure()`: **3 s timeout**, with panic enabled, so a timeout reboots the board.
3. A press of the **BOOT** button makes `report` hang: it blocks on a semaphore that nobody ever gives.
4. On every boot, log the **reset reason**, and the **total number of watchdog resets** so far, counted in a variable declared with `RTC_NOINIT_ATTR` and protected by a magic number.

Requirements:
- Use `esp_task_wdt.h`. Each task calls `esp_task_wdt_add(NULL)` for itself and `esp_task_wdt_reset()` in its own loop. Do not feed the watchdog on another task's behalf.
- Create both tasks with `xTaskCreate()` using the names `"sensor"` and `"report"` exactly. The TWDT log prints the task name, and you need to recognise it.
- The hang must be a genuine block on a semaphore (`xSemaphoreTake(..., portMAX_DELAY)` on one that is never given). Not a busy loop, and not simply skipping the feed.
- The button only sets a flag (declared `volatile`) or gives a task notification. The hang happens inside `report` itself.
- Read the reset reason with `esp_reset_reason()` and print it as a readable name, not a bare integer.
- The reset counter must survive a watchdog reset but **start again from 0 after a power cycle**. Use `RTC_NOINIT_ATTR` for both the counter and a magic-number variable next to it. On boot, if the magic number is wrong, or the reset reason is a power-on, initialise the counter to 0 and write the magic.
- Only increment the counter when the reset reason really is a watchdog reset.
- Every period, timeout, pin and magic value must be a named constant.

### Hardware

ESP32-S3 DevKitC-1 only. The BOOT button (GPIO0, pressed = LOW) is the trigger. The display module stays unplugged.

### Design Hints

```c
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "freertos/semphr.h"

#define BTN_PIN            GPIO_NUM_0
#define SENSOR_PERIOD_MS   (200U)
#define REPORT_PERIOD_MS   (1000U)
#define TWDT_TIMEOUT_MS    (3000U)
#define RESET_MAGIC        (0xC0FFEE42U)

RTC_NOINIT_ATTR static uint32_t wdt_magic;
RTC_NOINIT_ATTR static uint32_t wdt_reset_count;

static const char* reset_reason_name(esp_reset_reason_t reason)
{
    switch (reason)
    {
    case ESP_RST_POWERON:
        return "POWERON";
    /* ... ESP_RST_SW, ESP_RST_PANIC, ESP_RST_TASK_WDT, ... */
    default:
        return "OTHER";
    }
}

esp_task_wdt_config_t twdt_cfg = {
    .timeout_ms     = TWDT_TIMEOUT_MS,
    .idle_core_mask = /* decide: should the idle tasks still be watched? */,
    .trigger_panic  = true,
};
```

The framework usually initialises the TWDT for you at boot (see `CONFIG_ESP_TASK_WDT_INIT`). That is why this exercise asks for `esp_task_wdt_reconfigure()` rather than `esp_task_wdt_init()`. Read its return value, and find out what it means if it says the watchdog is not initialised.

Before you write the "is this a watchdog reset?" check, trigger the hang once and confirm what `esp_reset_reason()` really reports after a TWDT panic. Your check has to match what the chip says, not what you expect it to say.

`RTC_NOINIT_ATTR` means the startup code never touches the variable, on any kind of reset. That is why it survives a panic, and also why after power-on it contains random bits. The magic number is how you tell those two cases apart.

### Suggested Approach

```
app_main():
  1. reason = esp_reset_reason()
  2. if wdt_magic != RESET_MAGIC or reason is POWERON:
       wdt_reset_count = 0, wdt_magic = RESET_MAGIC
  3. if reason is a watchdog reset: wdt_reset_count++
  4. log reason name and wdt_reset_count
  5. esp_task_wdt_reconfigure(&twdt_cfg)
  6. create the semaphore (never given), BOOT as input with pull-up
  7. xTaskCreate(sensor_task, "sensor", ...), xTaskCreate(report_task, "report", ...)

sensor_task: esp_task_wdt_add(NULL); loop { read something / log; esp_task_wdt_reset(); delay 200 ms }
report_task: esp_task_wdt_add(NULL); loop { if hang requested: take the semaphore forever;
                                            log a report; esp_task_wdt_reset(); delay 1 s }
BOOT: when pressed, request the hang
```

### Expected Output

```
I (312) WDT: boot: reason=POWERON  watchdog_resets=0
I (520) WDT: sensor tick
I (1322) WDT: report: all good
...
I (5100) WDT: BOOT pressed -> report will hang
E (7330) task_wdt: Task watchdog got triggered. The following tasks/users did not reset the watchdog in time:
E (7330) task_wdt:  - report (CPU 0)
...
I (311) WDT: boot: reason=TASK_WDT  watchdog_resets=1
```

`report` last fed the watchdog at about 4322 ms, and the next iteration blocked before feeding, so the timeout fires about 3 s after that. The TWDT log must name `report`, and only `report`. `sensor` kept feeding and must not appear. Hang it again and the counter goes to 2, 3, and so on. Unplug the USB cable, plug it back in, and the counter is back at 0 with reason `POWERON`.

### Submission

```
Exercise_1/
├── main/
│   ├── main.c              (required — two subscribed tasks, reconfigured TWDT, RTC_NOINIT counter)
│   └── CMakeLists.txt      (required)
└── CMakeLists.txt          (required — ESP-IDF project root)
```
