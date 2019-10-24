#include "polling_splitflap.h"

PollingSplitflap::PollingSplitflap(uint8_t flap_count_, uint8_t zero_offset_, gpio_num_t motorPin_, gpio_num_t homePin_, gpio_num_t flapPin_, adc1_channel_t sensorInputChannel_, int falling_threshold_, int rising_threshold_)
: Splitflap(flap_count_, zero_offset_) {

    motorPin = motorPin_;
    homePin = homePin_;
    flapPin = flapPin_;
    sensorInputChannel = sensorInputChannel_;

    falling_threshold = falling_threshold_;
    rising_threshold = rising_threshold_;

	setup_gpio();
    init_sensors();
}

void PollingSplitflap::loop() {
    internal_loop(false, 0);
}

void PollingSplitflap::set_position(uint8_t pos) {
    internal_loop(true, pos);
}

void PollingSplitflap::internal_loop(bool command_valid, uint8_t pos) {

    if (command_valid) {
        flap_cmd = pos;
        cmd_void = false;
    }

    read_sensors();

    if (hsensor_ch_flag == RISING) {
        //ESP_LOGI(TAG_FLAP, "Home sensor level RISING");
    } else if (hsensor_ch_flag == FALLING) {
        //ESP_LOGI(TAG_FLAP, "Home sensor level FALLING");
    }

    if (fsensor_ch_flag == RISING) {
        //ESP_LOGI(TAG_FLAP, "Flap sensor level RISING");
    } else if (fsensor_ch_flag == FALLING) {
        //ESP_LOGI(TAG_FLAP, "Flap sensor level FALLING");
    }

    if (fsensor_ch_flag == RISING) {
        current_flap = (current_flap + 1) % flap_count;
        flap_ch_flag = true;
        //ESP_LOGI(TAG_FLAP, "current_flap = %d", current_flap);

        if (next_flap_is_home) {
            if (current_flap != 0) {
                ESP_LOGI(TAG_FLAP, "Home disagree (should be 0 but is %d)", current_flap);
            } else {
                ESP_LOGI(TAG_FLAP, "Home OK");
            }
            current_flap = 0;
            current_flap_inaccurate = false;
            next_flap_is_home = false;
        }
    }

    if (hsensor_ch_flag == RISING) {
        next_flap_is_home = true;
    }

    hsensor_ch_flag = NOCHANGE;
    fsensor_ch_flag = NOCHANGE;

    if (flap_ch_flag) {
        if (current_flap == flap_cmd && !current_flap_inaccurate) {
            gpio_set_level(motorPin, 1);
            motor_running = false;
            ESP_LOGI(TAG_FLAP, "Motor OFF");
        }
    }

    flap_ch_flag = false;

    if (current_flap != flap_cmd && !motor_running) {
        gpio_set_level(motorPin, 0);
        motor_running = true;
        ESP_LOGI(TAG_FLAP, "Motor ON");
    }
    
}

uint8_t PollingSplitflap::get_position() {
    if (current_flap_inaccurate) {
        return 0xFF;
    } else {
        return current_flap;
    }
}

uint8_t PollingSplitflap::get_commanded_position() {
    if (cmd_void) {
        return 0xFF;
    } else {
        return flap_cmd;
    }
}


void PollingSplitflap::setup_gpio() {
	gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = BIT(motorPin) | BIT(homePin) | BIT(flapPin);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    gpio_set_level(motorPin, 1);
    gpio_set_level(homePin, 0);
    gpio_set_level(flapPin, 0);

   	adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(sensorInputChannel, ADC_ATTEN_DB_11);
}

void PollingSplitflap::read_sensors_raw_values() {
	gpio_set_level(flapPin, 1);
	vTaskDelay(1);
    fsensor_value = adc1_get_raw(sensorInputChannel);
    gpio_set_level(flapPin, 0);
    gpio_set_level(homePin, 1);
    vTaskDelay(1);
    hsensor_value = adc1_get_raw(sensorInputChannel);
    gpio_set_level(homePin, 0);
}

void PollingSplitflap::init_sensors() {
    read_sensors_raw_values();

    if (fsensor_value >= rising_threshold) {
        fsensor_state = 1;
    } else {
        fsensor_state = 0;
    }

    if (hsensor_value >= rising_threshold) {
        hsensor_state = 1;
    } else {
        hsensor_state = 0;
    }

    fsensor_ch_flag = NOCHANGE;
    hsensor_ch_flag = NOCHANGE;
}

void PollingSplitflap::read_sensors() {
    read_sensors_raw_values();

    // detect rising and falling edges for sensors
    if (fsensor_state == 1 && fsensor_value <= falling_threshold) {
        fsensor_state = 0;
        fsensor_ch_flag = FALLING;
    } else if (fsensor_state == 0 && fsensor_value >= rising_threshold) {
        fsensor_state = 1;
        fsensor_ch_flag = RISING;
    }

    if (hsensor_state == 1 && hsensor_value <= falling_threshold) {
        hsensor_state = 0;
        hsensor_ch_flag = FALLING;
    } else if (hsensor_state == 0 && hsensor_value >= rising_threshold) {
        hsensor_state = 1;
        hsensor_ch_flag = RISING;
    }
    
}