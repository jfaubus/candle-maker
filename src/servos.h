#ifndef SERVOS_H
#define SERVOS_H


#include <zephyr/drivers/gpio.h>

void thru_beam_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
int door_lock_init(void);
int door_lock(void);
int door_unlock(void);

#endif