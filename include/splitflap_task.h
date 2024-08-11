#ifndef SPLITFLAP_TASK_H
#define SPLITFLAP_TASK_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "names.h"
#include "splitflap.h"

class SplitflapTask {

public:
	
	static void setup() {
		xTaskCreate(&worker, "splitflap_task", 2048, NULL, configMAX_PRIORITIES - 1, &handle);
	}

	static void send_command(uint8_t module, uint8_t position) {
		uint32_t value = module << 8;
		value |= position;

		xTaskNotify(handle, value, eSetValueWithOverwrite);
	}

	static uint8_t number_of_modules() {
		return Splitflap::number_of_modules();
	}

	static uint8_t number_of_positions(int module) {
		return Splitflap::module(module)->number_of_flaps();
	}

	static uint8_t get_position(int module) {
		return Splitflap::module(module)->get_position();
	}

    static uint8_t get_commanded_position(int module) {
        return Splitflap::module(module)->get_commanded_position();
    }

private:
	static TaskHandle_t handle;

	static void worker(void* arg);
};

#endif //SPLITFLAP_TASK_H