/**
 * Copyright (c) 2009 - 2019, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/** @file
* @brief Example template project.
* @defgroup nrf_templates_example Example Template
*
*/

#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#include "nrf.h"
#include "nordic_common.h"

#include "nrfx_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "nrfx_gpiote.h"
#include "nrfx_saadc.h"
#include "nrfx_temp.h"
#include "nrfx_clock.h"
#include "nrfx_rtc.h"

#define IN_BUTTON_0 NRF_GPIO_PIN_MAP(0,10)
#define IN_PROBE_1 NRF_GPIO_PIN_MAP(0,30)
#define IN_PROBE_2 NRF_GPIO_PIN_MAP(0,31)
#define OUT_LED_0 NRF_GPIO_PIN_MAP(0,9)

#define CFG_MAIN_LOOP_DELAY_MS 30000
#define CFG_BUTTON_DEBOUNCE_DELAY_MS 100
#define CFG_BUTTON_LONG_PRESS_DELAY_MS 5000
#define CFG_PROBE_DEBOUNCE_DELAY_MS 1000

#define CFG_LED_FLASH_CYCLE_DELAY_MS 200
#define CFG_LED_FLASH_DUTY_DELAY_MS 20

#define RTC_COUNTER_FREQUENCY 100
#define RTC_MS_TO_COUNTER(t) ((t * RTC_INPUT_FREQ / \
             (RTC_FREQ_TO_PRESCALER(RTC_COUNTER_FREQUENCY) + 1)) / 1000)

const nrfx_rtc_t rtc0 = NRFX_RTC_INSTANCE(0);
const nrfx_rtc_t rtc1 = NRFX_RTC_INSTANCE(1);


uint16_t saadc_sample()
{
    nrfx_err_t err_code;
    uint16_t result = 0;
    const uint8_t saadc_rsolutions[] = { 8, 10, 12, 14 };

    err_code = nrfx_saadc_sample_convert(0, &result);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_INFO("SAADC: VDD value " NRF_LOG_FLOAT_MARKER " V", 
            NRF_LOG_FLOAT((float)result * 6.0 * 0.6 /
                    (1 << saadc_rsolutions[NRFX_SAADC_CONFIG_RESOLUTION])));
    return result;
}

int32_t temp_measure()
{
    nrfx_err_t err_code;
    int32_t result = 0;

    err_code = nrfx_temp_measure();
    APP_ERROR_CHECK(err_code);
    result = nrfx_temp_calculate(nrfx_temp_result_get());

    NRF_LOG_INFO("TEMP: temperature " NRF_LOG_FLOAT_MARKER " C", 
            NRF_LOG_FLOAT((float)result / 100));
    return result;
}

void led_blink(uint8_t count)
{
    uint32_t current_counter = nrfx_rtc_counter_get(&rtc1);
    
    /**
     * RTC1 CC Channel 1: cycle phases (duty/pause) delay
     * RTC1 CC Channel 2: cycles count limit
     */

    /**
     * TODO: PROBLEM - need to understand is enabled RTC instance or not
     */

    /**
     * LED cycle duty phase delay
     */
    nrfx_rtc_cc_set(&rtc1, 1,
            current_counter +
            RTC_MS_TO_COUNTER(CFG_LED_FLASH_DUTY_DELAY_MS), 
            true);

    /**
     * Set cycle limit
     * (cycle duration * cycle count - pause delay of last cycle)
     */
    nrfx_rtc_cc_set(&rtc1, 2,
            current_counter +
            RTC_MS_TO_COUNTER(CFG_LED_FLASH_CYCLE_DELAY_MS) * (count - 1) +
                     RTC_MS_TO_COUNTER(CFG_LED_FLASH_DUTY_DELAY_MS),
            true);

    /**
     * Switch LED to ON state
     */
    nrfx_gpiote_out_clear(OUT_LED_0);
}


static void gpio_event_handler(nrfx_gpiote_pin_t pin,
        nrf_gpiote_polarity_t action)
{
    nrfx_err_t err_code;
    bool pin_is_set = nrfx_gpiote_in_is_set(pin);
    uint32_t current_counter;

    NRF_LOG_INFO("GPIO: pin %d is %s", pin, (pin_is_set? "set": "clear"));

    NRFX_CRITICAL_SECTION_ENTER();

    switch (pin)
    {
        /*
         * Button 0 input
         */
        case IN_BUTTON_0:
            /*
             * RTC1 CC Channel 0: long press delay
             *
             * When button pressed first time all compare counters 
             * will started.
             * Until debounce counter value will reached no any other
             * events will processed. If input state wil not in LO state
             * at end of debounce interval all counters will reset
             * and processing will aborted.
             * When LOTOHI event raised after debounce interval and along
             * long press interval this means short press of button.
             * When LOTOHI event raised after long press interval
             * this means long press of button.
             * 
             * At end of event processing counter will be reset to zero.
             * So nonzero value of counter means that event processing is
             * in progress.
             */
            current_counter = nrfx_rtc_counter_get(&rtc1);

            if (current_counter == 0)
            {
                if (!pin_is_set)
                {
                /*
                 * First press of button
                 * Arm long press delay counter
                 */
                nrfx_rtc_cc_set(&rtc1, 0,
                        RTC_MS_TO_COUNTER(CFG_BUTTON_LONG_PRESS_DELAY_MS), 
                        true);
//                    nrfx_rtc_enable(&rtc1);
                }
            }
            else if (pin_is_set)
            {
                /*
                 * Button released
                 */
                if (current_counter <
                        RTC_MS_TO_COUNTER(CFG_BUTTON_DEBOUNCE_DELAY_MS))
                {
                    /**
                     * False event - under debounce time, ignore
                     */
                    nrfx_rtc_disable(&rtc1);
                    nrfx_rtc_counter_clear(&rtc1);
                }
                else if (current_counter < 
                        RTC_MS_TO_COUNTER(CFG_BUTTON_LONG_PRESS_DELAY_MS))
                {
                    /*
                     * Short press
                     */
                    nrfx_rtc_cc_disable(&rtc1, 0);

                    led_blink(2);

                    /**
                     * TODO: This place for some application job
                     */
//                    saadc_sample();
//                    temp_measure();
                }
            }
            break;
        case IN_PROBE_1:
        case IN_PROBE_2:
            led_blink(10);
            break;
        default:
            break;
    }

    NRFX_CRITICAL_SECTION_EXIT();
}

static void saadc_event_handler(nrfx_saadc_evt_t const *p_event) {}

static void temp_event_handler(int32_t value) {}

void clock_event_handler(nrfx_clock_evt_type_t event) {}

void rtc0_event_handler(nrfx_rtc_int_type_t event)
{
    nrfx_err_t err_code;

    switch (event)
    {
        /*
         * Main loop cicle
         */
        case NRFX_RTC_INT_COMPARE0:
            nrfx_rtc_counter_clear(&rtc0);
            err_code = nrfx_rtc_cc_set(&rtc0, 0, 
                    RTC_MS_TO_COUNTER(CFG_MAIN_LOOP_DELAY_MS), true);
            APP_ERROR_CHECK(err_code);

//            led_blink(3);

            saadc_sample();
            temp_measure();
            break;
        default:
            break;
    }
}


void rtc1_event_handler(nrfx_rtc_int_type_t event)
{
    nrfx_err_t err_code;

    NRFX_CRITICAL_SECTION_ENTER();

    /*
     * Current button state
     */
    bool btn_is_set = nrfx_gpiote_in_is_set(IN_BUTTON_0);
    uint32_t current_counter = nrfx_rtc_counter_get(&rtc1);

    switch (event)
    {
        /*
         * Debounce interval
         */
        case NRFX_RTC_INT_COMPARE0:
            /*
             * Long press delay reached
             */
            if(!btn_is_set)
            {
                /**
                 * Stop button long press counter
                 */
                nrfx_rtc_cc_disable(&rtc1, 0);

                led_blink(5);

                /**
                 * TODO: This place for some application job
                 */
            }
            break;
        case NRFX_RTC_INT_COMPARE1:
            /*
             * Toggle LED state and set counter of next phase
             */

            if(nrf_gpio_pin_out_read(OUT_LED_0))
            {
                /**
                 * LED is off - going to duty phase
                 */
                nrfx_rtc_cc_set(&rtc1, 1,
                        current_counter +
                                RTC_MS_TO_COUNTER(CFG_LED_FLASH_DUTY_DELAY_MS), 
                        true);

                nrfx_gpiote_out_clear(OUT_LED_0);
            }
            else
            {
                /**
                 * LED is on - going to passive phase
                 */
                nrfx_rtc_cc_set(&rtc1, 1,
                        current_counter +
                                (RTC_MS_TO_COUNTER(CFG_LED_FLASH_CYCLE_DELAY_MS) -
                                RTC_MS_TO_COUNTER(CFG_LED_FLASH_DUTY_DELAY_MS)),
                        true);

                nrfx_gpiote_out_set(OUT_LED_0);
            }
            break;
        case NRFX_RTC_INT_COMPARE2:
            /*
             * LED blink finished - disable counter and switch led off
             */
            nrfx_rtc_disable(&rtc1);
            nrfx_rtc_cc_disable(&rtc1, 1);
            nrfx_rtc_cc_disable(&rtc1, 2);
            nrfx_rtc_counter_clear(&rtc1);

            nrfx_gpiote_out_set(OUT_LED_0);
            break;
        default:
            break;
    }

    NRFX_CRITICAL_SECTION_EXIT();
}

/*
 * @brief Function for initializing the button handler module.
 */
static void gpio_init(void)
{
    nrfx_err_t err_code;

    if (!nrfx_gpiote_is_init())
    {
        err_code = nrfx_gpiote_init();
        APP_ERROR_CHECK(err_code);
    }

    /**
     * Input button 0
     */
    nrfx_gpiote_in_config_t in_config_button =
            NRFX_GPIOTE_CONFIG_IN_SENSE_TOGGLE(false);
    in_config_button.pull = NRF_GPIO_PIN_PULLUP;

    err_code = nrfx_gpiote_in_init(IN_BUTTON_0, &in_config_button,
            gpio_event_handler);
    APP_ERROR_CHECK(err_code);

    /**
     * Input probes
     */
    nrfx_gpiote_in_config_t in_config_probe =
            NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(false);
    in_config_probe.pull = NRF_GPIO_PIN_PULLUP;

    /**
     * Input probe 1
     */
    err_code = nrfx_gpiote_in_init(IN_PROBE_1, &in_config_probe,
            gpio_event_handler);
    APP_ERROR_CHECK(err_code);

    /**
     * Input probe 2
     */
    err_code = nrfx_gpiote_in_init(IN_PROBE_2, &in_config_probe,
            gpio_event_handler);
    APP_ERROR_CHECK(err_code);

    /*
     * Output LED indicator
     */
    nrfx_gpiote_out_config_t out_config =
            NRFX_GPIOTE_CONFIG_OUT_SIMPLE(true);
    /*
     * Output LED 0
     */
    err_code = nrfx_gpiote_out_init(OUT_LED_0, &out_config);
    APP_ERROR_CHECK(err_code);

    /*
     * Input interrupts
     */
    nrfx_gpiote_in_event_enable(IN_BUTTON_0, true);
    nrfx_gpiote_in_event_enable(IN_PROBE_1, true);
    nrfx_gpiote_in_event_enable(IN_PROBE_2, true);
}

void saadc_init()
{
    nrfx_err_t err_code;

    nrfx_saadc_config_t config = NRFX_SAADC_DEFAULT_CONFIG;
    err_code = nrfx_saadc_init(&config, saadc_event_handler);
    APP_ERROR_CHECK(err_code);

    nrf_saadc_channel_config_t config_ch_vdd =
            NRFX_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_VDD);
    config_ch_vdd.acq_time = NRF_SAADC_ACQTIME_20US;
    config_ch_vdd.burst = NRF_SAADC_BURST_ENABLED;

    err_code = nrfx_saadc_channel_init(0, &config_ch_vdd);
    APP_ERROR_CHECK(err_code);

    err_code = nrfx_saadc_calibrate_offset();
    APP_ERROR_CHECK(err_code);
}

void temp_init()
{
    nrfx_err_t err_code;

    nrfx_temp_config_t config = NRFX_TEMP_DEFAULT_CONFIG;
    err_code = nrfx_temp_init(&config, NULL);
    APP_ERROR_CHECK(err_code);
}

void rtc0_init()
{
    uint32_t err_code;

    /*
     * Start the low-frequency clock if it hasn't been started
     */
    if (!nrfx_clock_lfclk_is_running()) {
        nrfx_clock_lfclk_start();
    }

    /*
     * Init RTC frequency
     */
    nrfx_rtc_config_t rtc_config = NRFX_RTC_DEFAULT_CONFIG;
    rtc_config.prescaler = RTC_FREQ_TO_PRESCALER(RTC_COUNTER_FREQUENCY);
    err_code = nrfx_rtc_init(&rtc0, &rtc_config, rtc0_event_handler);
    APP_ERROR_CHECK(err_code);
    nrfx_rtc_counter_clear(&rtc0);
}

void rtc1_init()
{
    uint32_t err_code;

    /*
     * Start the low-frequency clock if it hasn't been started
     */
    if (!nrfx_clock_lfclk_is_running()) {
        nrfx_clock_lfclk_start();
    }

    /*
     * Init RTC frequency
     */
    nrfx_rtc_config_t rtc_config = NRFX_RTC_DEFAULT_CONFIG;
    rtc_config.prescaler = RTC_FREQ_TO_PRESCALER(RTC_COUNTER_FREQUENCY);
    err_code = nrfx_rtc_init(&rtc1, &rtc_config, rtc1_event_handler);
    APP_ERROR_CHECK(err_code);
    nrfx_rtc_counter_clear(&rtc1);
}

/**
 * @brief Function for application main entry.
 */
int main(void)
{
    uint32_t err_code;

    err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();

    /*
     * Initialize peripherials
     */
    gpio_init();

    saadc_init();

    temp_init();

    err_code = nrfx_clock_init(clock_event_handler);
    APP_ERROR_CHECK(err_code);

    /*
     * RTC instance #0 - used for main loop cicle
     */
//    rtc0_init();

    /*
     * RTC instance #1 - used for button 0 input
     */
    rtc1_init();

    /**
     * Initalization complete
     */

    uint32_t info = NRF_FICR->INFO.VARIANT;
    uint8_t *p = (uint8_t *)&info;
    
    NRF_LOG_INFO("Nordic Semiconductor nRF%x, Variant: %c%c%c%c",
        NRF_FICR->INFO.PART, p[3], p[2], p[1], p[0]);
    NRF_LOG_INFO("RAM: %dKB, Flash: %dKB",
        NRF_FICR->INFO.RAM,
        NRF_FICR->INFO.FLASH);
    NRF_LOG_INFO("Device ID: %x%x",
        NRF_FICR->DEVICEID[0],
        NRF_FICR->DEVICEID[1]);
    
    NRF_LOG_INFO("System initialized, enter to main loop");

    /*
     * Start RTC0 for main loop
     */
//    nrfx_rtc_counter_clear(&rtc0);
//    err_code = nrfx_rtc_cc_set(&rtc0, 0, 
//            RTC_MS_TO_COUNTER(CFG_MAIN_LOOP_DELAY_MS), true);
//    APP_ERROR_CHECK(err_code);
//
//    nrfx_rtc_enable(&rtc0);

    /*
     * Main loop
     */
    while (true)
    {
        if (!NRF_LOG_PROCESS())
        { 
            NRF_LOG_FLUSH();
            __WFE();
            __SEV();
            __WFE();
        }
    }
}
/** @} */
