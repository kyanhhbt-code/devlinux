# Assignment — Session: 10
**Deadline: 2026-09-27 23:59:00**

---

## Exercise_1 — A Mini Voltmeter With a Threshold Alarm [review-only]

### Problem Statement

Every peripheral so far has been digital: a pin is high or it is low. The ADC is where the outside world stops being tidy. The same knob position gives you a slightly different number on every read, and the number is not a voltage until you have calibrated it. A real product that reacts to an analog signal (a battery monitor, a temperature alarm) also has to cope with the signal wobbling around its trip point without the output flickering.

Build a small voltmeter with an alarm:

1. Read a potentiometer on GPIO4 with the ADC in one-shot mode, convert the reading to **millivolts** with the calibration API, and average several samples per reading.
2. Every 500 ms, log the raw value and the millivolts.
3. Drive an LED as a threshold alarm with **hysteresis**: the LED switches **on** when the voltage rises above 2000 mV, and switches **off** only when it falls below 1800 mV. Between the two thresholds the LED keeps whatever state it already had.

Requirements:
- Use the current `esp_adc/adc_oneshot.h` API: `adc_oneshot_new_unit()`, `adc_oneshot_config_channel()`, `adc_oneshot_read()`. Do **not** use the deprecated `driver/adc.h`. Almost every tutorial online still does, so check the include before copying anything.
- Convert raw counts to millivolts with `esp_adc/adc_cali.h` and `esp_adc/adc_cali_scheme.h` using the **curve fitting** scheme. If calibration is unavailable, log a warning and keep logging the raw counts only. Do not print a millivolt figure from a hand-rolled formula.
- Use **ADC1**. ADC2 shares hardware with the Wi-Fi radio and becomes unreliable once Wi-Fi is in use.
- Pick an attenuation that covers the potentiometer's full 0 V to 3.3 V swing, and state in a comment what input range it actually covers.
- Average at least 8 samples per reading.
- The two thresholds must be two separate named constants. A single threshold with an `if (mv > X) on; else off;` does not meet the requirement.
- When the LED changes state, log one extra line saying so. Do not repeat the LED state on every reading.
- Every pin, channel, attenuation, threshold, period and sample count must be a named constant.
- `adc_oneshot_read()` returns an `esp_err_t`. In the runtime loop, check it and skip the sample on failure. Use `ESP_ERROR_CHECK` for init only.

### Hardware

None of Sessions 10 to 13 use the display. **GPIO4 is also `CTP_SDA` of the touch panel from Session 07**, so disconnect the module's four touch lines (`CTP_SDA`, `CTP_SCL`, `CTP_RST`, `CTP_INT`) before wiring the potentiometer. The pull-ups on the touch controller side would otherwise drag your reading. Unplugging the whole module for these four sessions is simplest.

| Part | ESP32-S3 | Notes |
| --- | --- | --- |
| potentiometer, outer pin 1 | 3V3 | |
| potentiometer, outer pin 2 | GND | |
| potentiometer, wiper (middle) | GPIO4 | `ADC_CHANNEL_3` on ADC1 |
| LED anode, via 220 Ω–330 Ω resistor | GPIO15 | cathode to GND |

Any linear potentiometer from 1 kΩ to 100 kΩ works. Use a plain single-colour LED, not the onboard RGB LED. That one is a WS2812 and cannot be driven with `gpio_set_level()`.

### Design Hints

```c
#include "driver/gpio.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"

#define POT_ADC_UNIT     ADC_UNIT_1
#define POT_ADC_CHANNEL  ADC_CHANNEL_3     /* GPIO4 on ESP32-S3 */
#define POT_ADC_ATTEN    ADC_ATTEN_DB_12
#define POT_ADC_BITWIDTH ADC_BITWIDTH_DEFAULT
#define LED_PIN          GPIO_NUM_15

#define SAMPLE_COUNT     (8U)
#define REPORT_PERIOD_MS (500U)
#define ALARM_ON_MV      (2000)
#define ALARM_OFF_MV     (1800)

adc_oneshot_unit_init_cfg_t unit_cfg = {
    .unit_id = POT_ADC_UNIT,
};

adc_oneshot_chan_cfg_t chan_cfg = {
    .atten    = POT_ADC_ATTEN,
    .bitwidth = POT_ADC_BITWIDTH,
};

adc_cali_curve_fitting_config_t cali_cfg = {
    .unit_id  = POT_ADC_UNIT,
    .atten    = POT_ADC_ATTEN,
    .bitwidth = POT_ADC_BITWIDTH,
};
```

Create the calibration handle once at startup, then call `adc_cali_raw_to_voltage()` for every reading. Should you average the raw counts and convert once, or convert every sample and average the millivolts? Pick one and be able to explain why.

For the alarm, think of the LED as having a **state**, not as a function of the current voltage. Each reading, the question is "given the state I am in, does this voltage make me leave it?"

### Suggested Approach

```
1. Configure LED_PIN as an output, LED off
2. Init ADC1 unit + channel, then create the calibration handle
3. led_on = false
   forever:
     a. read SAMPLE_COUNT samples, average
     b. raw -> mV with the calibration handle
     c. log raw and mV
     d. if !led_on and mv > ALARM_ON_MV  -> LED on,  log the change
        if  led_on and mv < ALARM_OFF_MV -> LED off, log the change
     e. vTaskDelay(pdMS_TO_TICKS(REPORT_PERIOD_MS))
```

### Expected Output

Turning the knob changes the millivolt reading smoothly from near 0 to near 3300 mV. Neither end reaches its theoretical limit exactly, and that is normal ADC behaviour on this chip. The LED switches on just after 2000 mV and stays on until the voltage drops below 1800 mV. Park the knob right at 2000 mV and the LED must hold steady rather than flicker.

```
I (1512) VMETER: raw= 1203  ->  980 mV
I (2012) VMETER: raw= 2388  -> 1941 mV
I (2512) VMETER: raw= 2530  -> 2057 mV
I (2512) VMETER: ALARM ON  (2057 mV > 2000 mV)
I (3012) VMETER: raw= 2410  -> 1958 mV
I (3512) VMETER: raw= 2150  -> 1745 mV
I (3512) VMETER: ALARM OFF (1745 mV < 1800 mV)
```

The 1958 mV reading at 3012 ms is below the ON threshold, yet the LED stays on. That is the hysteresis working.

### Submission

```
Exercise_1/
├── main/
│   ├── main.c              (required — adc_oneshot + adc_cali, hysteresis alarm)
│   └── CMakeLists.txt      (required)
└── CMakeLists.txt          (required — ESP-IDF project root)
```
