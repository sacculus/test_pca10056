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

#define BSP_SIMPLE 1


#include <stdbool.h>
#include <stdint.h>

#include "nrf.h"
#include "nordic_common.h"
#include "boards.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "app_gpiote.h"
#include "app_timer.h"
#include "app_button.h"

#define TEST_BUTTON_0 NRF_GPIO_PIN_MAP(0,7)
#define TEST_BUTTON_1 NRF_GPIO_PIN_MAP(0,8)
#define TEST_BUTTON_2 NRF_GPIO_PIN_MAP(1,10)

#define APP_GPIOTE_MAX_USERS 3

/**
 *  Delay from a GPIOTE event until a button is reported as pushed (in number of timer ticks).
 */
#define BUTTON_DEBOUNCE_DELAY APP_TIMER_TICKS(100)

/**@brief Function for handling events from the button handler module.
 *
 * @param[in] pin_no        The pin that the event applies to.
 * @param[in] button_action The button action (press/release).
 */
static void button_event_handler(uint8_t pin_no, uint8_t button_action)
{
    NRF_LOG_INFO("Button event: pin %d, action %d", pin_no, button_action);
}

/**@brief Function for initializing the button handler module.
 */
static void buttons_init(void)
{
    ret_code_t err_code;

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

    //APP_SCHED_INIT(SCHED_MAX_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);
    APP_GPIOTE_INIT(APP_GPIOTE_MAX_USERS);

    err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);

    /**
     * The array must be static because a pointer to it
     * will be saved in the button handler module.
     */
    static app_button_cfg_t buttons[] =
    {
        {TEST_BUTTON_2, APP_BUTTON_ACTIVE_LOW,
                NRF_GPIO_PIN_PULLUP, button_event_handler},
        {TEST_BUTTON_1, APP_BUTTON_ACTIVE_LOW,
                NRF_GPIO_PIN_PULLUP, button_event_handler},
        {TEST_BUTTON_2, APP_BUTTON_ACTIVE_LOW,
                NRF_GPIO_PIN_PULLUP, button_event_handler}
    };

    err_code = app_button_init(buttons, ARRAY_SIZE(buttons),
                               BUTTON_DEBOUNCE_DELAY);
    APP_ERROR_CHECK(err_code);

    err_code = app_button_enable();
    APP_ERROR_CHECK(err_code);

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

    while (true)
    {
        if (!NRF_LOG_PROCESS())
        { 
            NRF_LOG_FLUSH();
        }
        //app_sched_execute();
    }
}
/** @} */
