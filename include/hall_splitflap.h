#ifndef HALL_SPLITFLAP_H_
#define HALL_SPLITFLAP_H_

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/semphr.h>
#include "splitflap.h"
#include "driver/gpio.h"
#include <esp_timer.h>

class HallSplitflap : public Splitflap {

public:
	HallSplitflap(
		uint8_t flap_count_,
		uint8_t zero_offset_,
		gpio_num_t motorPin_,
		gpio_num_t homePin_,
		gpio_num_t flapPin_,
		int64_t ignore_delay_,
		int64_t overshoot_delay_
	);

	~HallSplitflap();

	void loop();
	void set_position(uint8_t pos);
	uint8_t get_position();
    uint8_t get_commanded_position();
	
	EventGroupHandle_t getStatusBits() { return status_bits; };

private:
	// gpio pins
	gpio_num_t motorPin;
	gpio_num_t homePin;
	gpio_num_t flapPin;

	// fine tuning
	int64_t ignore_delay;
	int64_t overshoot_delay;

    // status variables
    uint8_t flap = 0;
    uint8_t flap_cmd = 0;
    bool flap_known = false;
    bool cmd_void = true;
    bool cycling = false;
	bool home_pending = false;
	bool flap_pending = false;

    int64_t last_tick;

	// timers
	esp_timer_handle_t ignore_timer;
	esp_timer_handle_t overshoot_timer;

	// concurrency management
	EventGroupHandle_t status_bits;
	uint32_t SBIT_FLAP_PENDING =	0x00000001;
	uint32_t SBIT_HOME_PENDING =	0x00000002;
    uint32_t SBIT_IGNORE = 			0x00000004;
	SemaphoreHandle_t mutex;

	void setup_gpio();
	void setup_timer();
	void setup_isr();

	// interrupt and timer callbacks
	static void ignore_timer_callback(void* arg);
	static void overshoot_timer_callback(void* arg);
	static void IRAM_ATTR home_callback(void* arg);
	static void IRAM_ATTR flap_callback(void* arg);
	void isr_home();
	void isr_flap();
	void ignore_timer_timeout();
	void overshoot_timer_timeout();

};

#endif //HALL_SPLITFLAP_H_
