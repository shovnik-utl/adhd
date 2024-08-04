#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adhd_haptics, LOG_LEVEL_DBG);

#include "haptics.h"

/* Get GPIO pin attached to vibration motor on the device. */
#define VIBRATION_MOTOR_NODE DT_NODELABEL(vibm)
static const struct gpio_dt_spec haptics = GPIO_DT_SPEC_GET(VIBRATION_MOTOR_NODE, gpios);

/* Binary semaphore for controlling haptic response thread. */
static K_SEM_DEFINE(sem_haptics, 0, 1);

/* Thread for doing haptic response continuously. */
#define HAPTIC_RESPONSE_THREAD_PRIORITY 5
#define HAPTIC_RESPONSE_THREAD_STACKSIZE 1024
K_THREAD_STACK_DEFINE(haptic_response_thread_stack_area, HAPTIC_RESPONSE_THREAD_STACKSIZE);
struct k_thread haptic_response_thread_data;

static void vibrate()
{
    for (;;) {
        gpio_pin_set_dt(&haptics, 1);
        k_msleep(100);
        gpio_pin_set_dt(&haptics, 0);
        if (0 == k_sem_take(&sem_haptics, K_NO_WAIT)) {
            /* Signal to stop haptic response. */
            gpio_pin_set_dt(&haptics, 0);
            break;
        }
        k_msleep(500);
    }
}

void haptic_response(bool start)
{
    if (start) {
        k_thread_create(
            &haptic_response_thread_data, haptic_response_thread_stack_area,
            K_THREAD_STACK_SIZEOF(haptic_response_thread_stack_area),
            vibrate, NULL, NULL, NULL,
            HAPTIC_RESPONSE_THREAD_STACKSIZE, 0, K_NO_WAIT
        );
    }
    else {
        k_sem_give(&sem_haptics);
    }
}

int gpio_init_haptics(void)
{
	int ret;
	if (gpio_is_ready_dt(&haptics)) {
		ret = gpio_pin_configure_dt(&haptics, GPIO_OUTPUT_ACTIVE);
		if (ret == 0) {
			gpio_pin_set_dt(&haptics, 0); /* Motor is off initially. */
			LOG_INF("Vibration motor initialized @%s pin %d.", haptics.port->name, haptics.pin);
			return 0;
		}
	}
	LOG_ERR("Vibration motor initialization failed.");
	return 1;
}
