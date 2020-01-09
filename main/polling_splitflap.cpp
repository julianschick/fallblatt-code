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
    read_sensors();

    if (hsensor_ch_flag == RISING) {
        ESP_LOGI(TAG_FLAP, "Home sensor level RISING");
    } else if (hsensor_ch_flag == FALLING) {
        ESP_LOGI(TAG_FLAP, "Home sensor level FALLING");
    }

    if (fsensor_ch_flag == RISING) {
        ESP_LOGI(TAG_FLAP, "Flap sensor level RISING");
    } else if (fsensor_ch_flag == FALLING) {
        ESP_LOGI(TAG_FLAP, "Flap sensor level FALLING");
    }

    if (fsensor_ch_flag == RISING) {
        flap = (flap + 1) % flap_count;
        //ESP_LOGI(TAG_FLAP, "current_flap = %d", current_flap);

        if (home_pending) {
            if (flap != 0) {
                ESP_LOGI(TAG_FLAP, "Home disagree (should be 0 but is %d)", flap);
            } else {
                ESP_LOGI(TAG_FLAP, "Home OK");
            }
            flap = zero_offset;
            flap_known = true;
            home_pending = false;
        }

        if (flap == flap_cmd && flap_known) {
            gpio_set_level(motorPin, 1);
            cycling = false;
            ESP_LOGI(TAG_FLAP, "Motor OFF");
        }
    }

    if (hsensor_ch_flag == RISING) {
        home_pending = true;
    }

    hsensor_ch_flag = NOCHANGE;
    fsensor_ch_flag = NOCHANGE;

    if (flap != flap_cmd && !cycling) {
        gpio_set_level(motorPin, 0);
        cycling = true;
        ESP_LOGI(TAG_FLAP, "Motor ON");
    }
}

void PollingSplitflap::set_position(uint8_t pos) {
    flap_cmd = pos;
    cmd_void = false;

    loop();
}

uint8_t PollingSplitflap::get_position() {
    if (!flap_known) {
        return 0xFF;
    } else {
        return flap;
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

    //ESP_LOGI("SENSOR", "HSensor %d", hsensor_value);
    //ESP_LOGI("SENSOR", "FSensor %d", fsensor_value);
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