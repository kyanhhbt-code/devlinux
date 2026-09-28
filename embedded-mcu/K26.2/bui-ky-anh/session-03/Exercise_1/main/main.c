#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdint.h>

#define BTN_PIN (14U)
#define DR_REG_GPIO_BASE (0x60004000UL)
#define GPIO_OUT_W1TS_REG (*(volatile uint32_t*) (DR_REG_GPIO_BASE + 0x0008UL))
#define GPIO_OUT_W1TC_REG (*(volatile uint32_t*) (DR_REG_GPIO_BASE + 0x000CUL))
#define GPIO_ENABLE_W1TS_REG (*(volatile uint32_t*) (DR_REG_GPIO_BASE + 0x0024UL))
#define GPIO_ENABLE_W1TC_REG (*(volatile uint32_t*) (DR_REG_GPIO_BASE + 0x0028UL))
#define GPIO_IN_REG (*(volatile uint32_t*) (DR_REG_GPIO_BASE + 0x003CUL))
#define DR_REG_IO_MUX_BASE (0x60009000UL)
#define IO_MUX_GPIO_REG(n) (*(volatile uint32_t*) (DR_REG_IO_MUX_BASE + 0x4U + (n) * 4U))
#define IO_MUX_FUN_WPD (7U)
#define IO_MUX_FUN_WPU (8U)
#define IO_MUX_FUN_IE (9U)
#define IO_MUX_FUN_DRV (10U)
#define IO_MUX_MCU_SEL (12U)

#define SEGMENT_A (4U)
#define SEGMENT_B (5U)
#define SEGMENT_C (6U)
#define SEGMENT_D (7U)
#define SEGMENT_E (15U)
#define SEGMENT_F (16U)
#define SEGMENT_G (17U)
#define SEGMENT_DP (18U)


#define DEBOUNCE_MS       (25U)
#define DOUBLE_CLICK_MS   (350U)  /* max gap between two clicks to count as a double */
#define LONG_PRESS_MS     (800U)  /* hold time before auto-repeat starts */
#define REPEAT_PERIOD_MS  (500U)  /* required by the spec — do not change this one */

/* Bit b0..b6 = segments a..g. This table is for a COMMON CATHODE display. */
/*
 * Display type: COMMON CATHODE
 */
static const uint8_t SEGMENT_MAP[10UL] = {
    0x3FU, 0x06U, 0x5BU, 0x4FU, 0x66U, /* 0 1 2 3 4 */
    0x6DU, 0x7DU, 0x07U, 0x7FU, 0x6FU, /* 5 6 7 8 9 */
};

static const uint8_t SEGMENT_MAP_PIN[8UL] = {
    SEGMENT_A, SEGMENT_B, SEGMENT_C, SEGMENT_D, SEGMENT_E, SEGMENT_F, SEGMENT_G, SEGMENT_DP,
} ;


void gpio_init_btn(void){
    IO_MUX_GPIO_REG(BTN_PIN) &= ~(1UL << IO_MUX_FUN_WPD); //disable pull-down
    IO_MUX_GPIO_REG(BTN_PIN) |= (1UL << IO_MUX_FUN_WPU); //set WPU = 1
    //disable output
    GPIO_ENABLE_W1TC_REG = (1UL << BTN_PIN); 
    //enable input
    IO_MUX_GPIO_REG(BTN_PIN) |= (1UL << IO_MUX_FUN_IE);
    //f1
    IO_MUX_GPIO_REG(BTN_PIN) &= ~(7UL << IO_MUX_MCU_SEL);
    IO_MUX_GPIO_REG(BTN_PIN) |= (1UL << IO_MUX_MCU_SEL);
}

void gpio_init_led7thanh(void){
   
    for (uint8_t i = 0UL; i < 7UL; i++){
        //set DRV = 10mA
        if (SEGMENT_MAP_PIN[i] == 17UL || SEGMENT_MAP_PIN[i] == 18UL){
            IO_MUX_GPIO_REG(SEGMENT_MAP_PIN[i]) &= ~(3UL << IO_MUX_FUN_DRV);
            IO_MUX_GPIO_REG(SEGMENT_MAP_PIN[i]) |= (2UL << IO_MUX_FUN_DRV);
    }
        else {
            IO_MUX_GPIO_REG(SEGMENT_MAP_PIN[i]) &= ~(3UL << IO_MUX_FUN_DRV);
            IO_MUX_GPIO_REG(SEGMENT_MAP_PIN[i]) |= (1UL << IO_MUX_FUN_DRV);
        }
    //disable input
    IO_MUX_GPIO_REG(SEGMENT_MAP_PIN[i]) &= ~(1UL << IO_MUX_FUN_IE);
    //enable output
    GPIO_ENABLE_W1TS_REG = (1UL << SEGMENT_MAP_PIN[i]);
    //set F1
    IO_MUX_GPIO_REG(SEGMENT_MAP_PIN[i]) &= ~(7UL << IO_MUX_MCU_SEL);
    IO_MUX_GPIO_REG(SEGMENT_MAP_PIN[i]) |= (1UL << IO_MUX_MCU_SEL);
    }
}

void gpio_register_set_high(uint8_t pin){
    GPIO_OUT_W1TS_REG = (1UL << pin);
}

void gpio_register_set_low(uint8_t pin){
    GPIO_OUT_W1TC_REG = (1UL << pin);
}
uint8_t gpio_read(uint32_t pin){
    return ((GPIO_IN_REG & (1UL << pin)) != 0UL);
}

void display_led(uint8_t number){
    if (number > 9U) {
    return;
}
    for (uint8_t i = 0UL; i < 7UL; i++){
        if (((SEGMENT_MAP[number] >> i) & 1UL) != 0UL){
            gpio_register_set_high(SEGMENT_MAP_PIN[i]);
        }
        else gpio_register_set_low(SEGMENT_MAP_PIN[i]);
    }
    
}
void clear_led(void){
    for (uint8_t i = 0; i < 7UL; i++){
        GPIO_OUT_W1TC_REG = (1UL << SEGMENT_MAP_PIN[i]);
    }
}









void app_main(void)
{
    clear_led();
    gpio_init_btn();
    gpio_init_led7thanh();
    uint8_t s = 0;
    display_led(s);
    while (1){
    if(gpio_read(BTN_PIN) == 0){
        vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
        uint8_t i = 0;
        while(i<(LONG_PRESS_MS/DEBOUNCE_MS - 1) && gpio_read(BTN_PIN) == 0){
                vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
                i++;
            }
        
        if(gpio_read(BTN_PIN) == 1 && i<LONG_PRESS_MS/DEBOUNCE_MS){
            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
            uint8_t y = 0;
        while (y<(DOUBLE_CLICK_MS/DEBOUNCE_MS) && (gpio_read(BTN_PIN) == 1)){
        vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
        y++;
        }
        if(gpio_read(BTN_PIN )== 0 && y < (DOUBLE_CLICK_MS/DEBOUNCE_MS )){
            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
            if (gpio_read(BTN_PIN) == 0U) {
            if (s == 0) s = 9;
            else s--;
            display_led(s);
            vTaskDelay(pdMS_TO_TICKS(REPEAT_PERIOD_MS));
            continue;
        }}
        else {
            
            if(s == 9) s = 0;
            else s++;
            display_led(s);
            continue;
        }
    }
        else{
            
            while(gpio_read(BTN_PIN) == 0){
                if(s == 9) s = 0;
                else s++;
                display_led(s);
                for(uint8_t x = 0; x < (REPEAT_PERIOD_MS/DEBOUNCE_MS); x++){
                    vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
                    if(gpio_read(BTN_PIN) == 1) {
                        break;
                    }
                    
                }
               
            }
        }
    
    }
}


}