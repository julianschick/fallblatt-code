#ifndef POLLING_SPLITFLAP_H
#define POLLING_SPLITFLAP_H

#include "splitflap.h"
#include "driver/adc.h"
#include "driver/gpio.h"

enum Flank {
	FALLING, NOCHANGE, RISING
};

class PollingSplitflap : public Splitflap {

public: 
	PollingSplitflap(uint8_t flap_count_, uint8_t zero_offset_, gpio_num_t motorPin_, gpio_num_t homePin_, gpio_num_t flapPin_, adc1_channel_t sensorInputChannel_, int falling_threshold_, int rising_threshold_);

	void set_position(uint8_t flap_cmd_);
	void loop();
	uint8_t get_position();
    uint8_t get_commanded_position();

private:
	// gpio pins and adc channel
	gpio_num_t motorPin;
	gpio_num_t homePin;
	gpio_num_t flapPin;
	adc1_channel_t sensorInputChannel;

	// fine tuning
	int falling_threshold;
	int rising_threshold;

    /*uint8_t flap = 0;
    uint8_t flap_cmd = 0;
    bool flap_known = false;
    bool cmd_void = true;
    bool cycling = false;
    bool home_pending = false;*/

	uint8_t flap = 0;
    uint8_t flap_cmd = 0;
    bool flap_known = false;
    bool cmd_void = true;
    bool cycling = false;
    bool home_pending = false;

    // sensor input
	int fsensor_state;
	int fsensor_value;
	Flank fsensor_ch_flag;
	int hsensor_state;
	int hsensor_value;
	Flank hsensor_ch_flag;

	void internal_loop(bool command_valid, uint8_t flap_cmd);

	void setup_gpio();
	void init_sensors();

	void read_sensors_raw_values();
	void read_sensors();

};


#endif //POLLING_SPLITFLAP_H