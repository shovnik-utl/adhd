#include <zephyr/kernel.h>
#include <zephyr/bluetooth/services/bas.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adhd_battery, LOG_LEVEL_DBG);

#include "battery.h"

/* Notify the client about battery level via BAS (Battery Service). */
static void bas_notify(struct k_work *work)
{
    (void)work;
	uint8_t battery_level = bt_bas_get_battery_level();
	/* Fake battery status, ie a simulation: decrement from 100 down to 0 and reset. 
	   In actual practice, you would have some kind of a battery_measure()
	   function call here which queries your battery status by reading an ADC
	   and doing calculations with reference to your battery's discharge curve!
	   Refer to https://github.com/zephyrproject-rtos/zephyr/tree/main/samples/boards/nrf/battery
	*/
	battery_level--;
	if (!battery_level) {
		battery_level = 100U;
	}
	bt_bas_set_battery_level(battery_level);
}

/* For non-trivial work, its best to submit a work item to the system work queue
   from the timer expiry callback, instead of doing it within the callback 
   itself. */
K_WORK_DEFINE(bas_work, bas_notify);

static void bas_timer_expiry_fn(struct k_timer *timer_id)
{
    (void)timer_id;
    k_work_submit(&bas_work);
}

K_TIMER_DEFINE(bas_timer, bas_timer_expiry_fn, NULL);

void bas_start(k_timeout_t bas_update_interval)
{
    /* Start periodic timer. */
    k_timer_start(&bas_timer, bas_update_interval, bas_update_interval);
}