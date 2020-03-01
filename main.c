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

//#ifdef BSP_SIMPLE
//#undef BSP_SIMPLE
//#endif

#define SAADC_SAMPLES 1
#define SAADC_SAMPLE_TIME_US 20

#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#include "nrf.h"
#include "nordic_common.h"
#include "boards.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "nrfx_gpiote.h"
#include "nrfx_saadc.h"
#include "nrf_drv_clock.h"
#include "nrfx_rtc.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_delay.h"

#define IN_BUTTON_0 NRF_GPIO_PIN_MAP(0,10)
#define IN_PROBE_1 NRF_GPIO_PIN_MAP(0,30)
#define IN_PROBE_2 NRF_GPIO_PIN_MAP(0,31)
#define OUT_LED_0 NRF_GPIO_PIN_MAP(0,9)

static nrf_saadc_value_t saadc_buffer[SAADC_SAMPLES];

const nrfx_rtc_t p_rtc = NRFX_RTC_INSTANCE(0);

/**@brief Function for handling events from the button handler module.
 *
 * @param[in] pin_no        The pin that the event applies to.
 * @param[in] button_action The button action (press/release).
 */
static void gpio_event_handler(nrfx_gpiote_pin_t pin,
        nrf_gpiote_polarity_t action)
{
    NRF_LOG_INFO("GPIOTE event: pin %d, action %d, pin is %s", pin, action,
            (nrfx_gpiote_in_is_set(pin)? "set": "clear"));
    NRF_LOG_INFO("GPIO event: RTC0 ticks %d", nrfx_rtc_counter_get(&p_rtc));
    if (!nrfx_gpiote_in_is_set(pin))
    {
        bsp_board_led_on(1);
        nrf_delay_ms(500);
        bsp_board_led_off(1);
        NRF_LOG_INFO("GPIO event: RTC0 ticks %d", nrfx_rtc_counter_get(&p_rtc));
    }
}

static void saadc_event_handler(nrfx_saadc_evt_t const *p_event)
{
    nrfx_err_t err_code;
    char *buff;

    switch (p_event->type)
    {
        case NRFX_SAADC_EVT_DONE:
            buff = "done";
            err_code = nrfx_saadc_buffer_convert(
                    p_event->data.done.p_buffer, SAADC_SAMPLES);
            NRF_LOG_INFO("SAADC value: " NRF_LOG_FLOAT_MARKER " V (%d raw)",
                    NRF_LOG_FLOAT((float)p_event->data.done.p_buffer[0] 
                            * 6.0 * 0.6 / pow(2, 10)),
                    p_event->data.done.p_buffer[0]);
            APP_ERROR_CHECK(err_code);
            break;
        case NRFX_SAADC_EVT_LIMIT:
            buff = "limit reached";
            break;
        case NRFX_SAADC_EVT_CALIBRATEDONE:
            buff = "calibrate done";
            break;
        default:
            buff = "unknown event";
    }
    NRF_LOG_INFO("SAADC event: %s", buff);
}

void rtc_event_handler(nrfx_rtc_int_type_t event)
{
}

/**@brief Function for initializing the button handler module.
 */
static void gpio_init(void)
{
    nrfx_err_t err_code;

    if (!nrfx_gpiote_is_init())
    {
        err_code = nrfx_gpiote_init();
        APP_ERROR_CHECK(err_code);
    }

    bsp_board_init(BSP_INIT_LEDS);

    nrfx_gpiote_in_config_t in_config =
            NRFX_GPIOTE_CONFIG_IN_SENSE_TOGGLE(false);
    in_config.pull = NRF_GPIO_PIN_PULLUP;

    /**
     * Input button 0
     */
    err_code = nrfx_gpiote_in_init(IN_BUTTON_0, &in_config,
            gpio_event_handler);
    APP_ERROR_CHECK(err_code);

    nrfx_gpiote_in_event_enable(IN_BUTTON_0, true);

    /**
     * Input probe 1
     */
    err_code = nrfx_gpiote_in_init(IN_PROBE_1, &in_config,
            gpio_event_handler);
    APP_ERROR_CHECK(err_code);

    nrfx_gpiote_in_event_enable(IN_PROBE_1, true);

    /**
     * Input probe 2
     */
    err_code = nrfx_gpiote_in_init(IN_PROBE_2, &in_config,
            gpio_event_handler);
    APP_ERROR_CHECK(err_code);

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

    err_code = nrfx_saadc_channel_init(0, &config_ch_vdd);
    APP_ERROR_CHECK(err_code);
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

    gpio_init();

    saadc_init();

    //err_code = nrfx_clock_init();
    APP_ERROR_CHECK(err_code);
    
    //nrf_drv_clock_lfclk_request(NULL);

    nrfx_rtc_config_t rtc_config = NRFX_RTC_DEFAULT_CONFIG;
    rtc_config.prescaler = 217;
    err_code = nrfx_rtc_init(&p_rtc, &rtc_config, rtc_event_handler);
    APP_ERROR_CHECK(err_code);
    nrfx_rtc_enable(&p_rtc);

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
    
    NRF_LOG_INFO("System initialized");

    err_code = nrfx_saadc_buffer_convert(saadc_buffer, SAADC_SAMPLES);
    APP_ERROR_CHECK(err_code);

    err_code = nrfx_saadc_sample();
    APP_ERROR_CHECK(err_code);
    nrf_delay_us(SAADC_SAMPLE_TIME_US);
    err_code = nrfx_saadc_sample();
    APP_ERROR_CHECK(err_code);
    nrf_delay_us(SAADC_SAMPLE_TIME_US);
    err_code = nrfx_saadc_sample();
    APP_ERROR_CHECK(err_code);
    nrf_delay_us(SAADC_SAMPLE_TIME_US);
    err_code = nrfx_saadc_sample();
    APP_ERROR_CHECK(err_code);

    while (true)
    {
        if (!NRF_LOG_PROCESS())
        { 
            NRF_LOG_FLUSH();
        }
//        __SEV();
//        __WFE();
//        __WFE();
    }
}
/** @} */
