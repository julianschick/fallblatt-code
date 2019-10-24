#include "hall_splitflap.h"

#define context_switch(a) if((a) != pdFALSE ) { portYIELD_FROM_ISR() }

HallSplitflap::HallSplitflap (
	uint8_t flap_count_,
	uint8_t zero_offset_,
	gpio_num_t motorPin_,
	gpio_num_t homePin_,
	gpio_num_t flapPin_,
	int64_t ignore_delay_,
	int64_t overshoot_delay_
) : Splitflap(flap_count_, zero_offset_) {
	motorPin = motorPin_;
	homePin = homePin_;
	flapPin = flapPin_;

	ignore_delay = ignore_delay_;
	overshoot_delay = overshoot_delay_;
	status_bits = xEventGroupCreate();
	mutex = xSemaphoreCreateBinary();

	// Now it gets hot
	setup_gpio();
	setup_timer();
	setup_isr();
}

HallSplitflap::~HallSplitflap() {

}

// called from splitflap task
void HallSplitflap::set_position(uint8_t flap_cmd_) {
	if (flap_cmd_ >= flap_count) {
		return;
	}

	if (overshoot_delay > 0) {
		esp_timer_stop(overshoot_timer);
	}

	xSemaphoreTake(mutex, 0);
	{
		flap_cmd = flap_cmd_;
        cmd_void = false;

		if ((!flap_known || flap != flap_cmd) && !cycling) {
			
			if (ignore_delay > 0) {
				xEventGroupSetBits(status_bits, SBIT_IGNORE);
			}
			
			gpio_set_level(motorPin, 1);
			cycling = true;

			if (ignore_delay > 0) {
				esp_timer_start_once(ignore_timer, ignore_delay);
			}
		}
	}
	xSemaphoreGive(mutex);
}

// called from any task
uint8_t HallSplitflap::get_position() {
	xSemaphoreTake(mutex, 0);
	{
		if (!flap_known) {
			return 0xFF;
		} else {
			return flap;
		}
	}
	xSemaphoreGive(mutex);
}

// called from any task
uint8_t HallSplitflap::get_commanded_position() {
    xSemaphoreTake(mutex, 0);
    {
        if (cmd_void) {
            return 0xFF;
        } else {
            return flap_cmd;
        }
    }
    xSemaphoreGive(mutex);
}

// caution isr!
void HallSplitflap::isr_home() {
	BaseType_t hptw1 = pdFALSE;
	BaseType_t hptw2 = pdFALSE;
	
	xEventGroupSetBitsFromISR(status_bits, 0x01, &hptw1);
	xEventGroupSetBitsFromISR(status_bits, SBIT_HOME_PENDING, &hptw2);

	context_switch(hptw1 || hptw2);
}

// caution isr!
void HallSplitflap::isr_flap() {
	if (ignore_delay > 0 && (xEventGroupGetBitsFromISR(status_bits) & SBIT_IGNORE) != 0) {
		return;
	}

	BaseType_t hptw1 = pdFALSE;
	BaseType_t hptw2 = pdFALSE;
	BaseType_t hptw3 = pdFALSE;

	xEventGroupSetBitsFromISR(status_bits, 0x02, &hptw1);

	xSemaphoreTakeFromISR(mutex, &hptw2);
	{
		if ((xEventGroupGetBitsFromISR(status_bits) & SBIT_HOME_PENDING) != 0) {
			flap = zero_offset;
			flap_known = true;
			xEventGroupClearBitsFromISR(status_bits, SBIT_HOME_PENDING);
		} else {
			if (cycling) {
				flap = (flap + 1) % flap_count;    
			}
		}

		if (flap_known && flap == flap_cmd) {
			// Keep cycling for 'overshoot_delay' us
			if (overshoot_delay > 0) {
				esp_timer_start_once(overshoot_timer, overshoot_delay);
			} else {
				gpio_set_level(motorPin, 0);
				cycling = false;	
			}
		}
	}
	xSemaphoreGiveFromISR(mutex, &hptw3);

	context_switch(hptw1 || hptw2 || hptw3);
}

// called from timer task
void HallSplitflap::ignore_timer_timeout() {
	xEventGroupClearBits(status_bits, SBIT_IGNORE);
}

// called from timer task
void HallSplitflap::overshoot_timer_timeout() {
	xSemaphoreTake(mutex, 0);
	{
		gpio_set_level(motorPin, 0);
		cycling = false;
	}
	xSemaphoreGive(mutex);
}

//
// setup routines
//

void HallSplitflap::setup_gpio() {
	gpio_config_t io_conf;
	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pin_bit_mask = BIT(motorPin);
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	gpio_config(&io_conf);
	gpio_set_level(motorPin, 0);

	io_conf.intr_type = GPIO_INTR_POSEDGE;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pin_bit_mask = BIT(flapPin) | BIT(homePin);
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	gpio_config(&io_conf);
}

void HallSplitflap::setup_timer() {
	esp_timer_create_args_t timer_conf;
	timer_conf.callback = ignore_timer_callback;
	timer_conf.dispatch_method = ESP_TIMER_TASK;
	timer_conf.name = "ignore_timer";
	timer_conf.arg = &id;
	esp_timer_create(&timer_conf, &ignore_timer);

	timer_conf.callback = overshoot_timer_callback;
	timer_conf.dispatch_method = ESP_TIMER_TASK;
	timer_conf.name = "overshoot_timer";
	timer_conf.arg = &id;
	esp_timer_create(&timer_conf, &overshoot_timer);	
}

void HallSplitflap::setup_isr() {
	gpio_isr_handler_add(homePin, home_callback, (void*) &id);
	gpio_isr_handler_add(flapPin, flap_callback, (void*) &id);
}

//
// static callback delegates
//

void HallSplitflap::ignore_timer_callback(void* arg) {
	uint8_t id = *(uint8_t*) arg;
	((HallSplitflap*) instances.at(id))->ignore_timer_timeout();
}

void HallSplitflap::overshoot_timer_callback(void* arg) {
	uint8_t id = *(uint8_t*) arg;
	((HallSplitflap*) instances.at(id))->overshoot_timer_timeout();
}

void IRAM_ATTR HallSplitflap::home_callback(void* arg) {
	uint8_t id = *(uint8_t*) arg;
	((HallSplitflap*) instances.at(id))->isr_home();
}

void IRAM_ATTR HallSplitflap::flap_callback(void* arg) {
	uint8_t id = *(uint8_t*) arg;
	((HallSplitflap*) instances.at(id))->isr_flap();
}
