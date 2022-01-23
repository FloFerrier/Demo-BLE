#ifndef MAIN_H
#define MAIN_H

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <libopencm3/cm3/systick.h>
#include <libopencm3/cm3/nvic.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/pwr.h>
#include <libopencm3/stm32/rtc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>

#include "FreeRTOS.h"
#include "task.h"

#define BUFFER_CONSOLE_LEN_MAX (255)

#endif /* MAIN_H */