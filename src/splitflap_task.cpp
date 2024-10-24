#include "splitflap_task.h"

#define TAG "flap"
#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include <esp_log.h>

#if  DEPLOYMENT_TARGET == TARGET_DB
    #include "polling_splitflap.h"
#elif DEPLOYMENT_TARGET == TARGET_SBAHN
    #include "hall_splitflap.h"
#endif

TaskHandle_t SplitflapTask::handle;

#define PORT3 GPIO_NUM_25, GPIO_NUM_26, GPIO_NUM_27
#define PORT4 GPIO_NUM_19, GPIO_NUM_18, GPIO_NUM_5
#define PORT2 GPIO_NUM_23, GPIO_NUM_22, GPIO_NUM_21

void SplitflapTask::worker(void* arg) {
	ESP_LOGI(TAG, "Splitflap-Task started.");

    #if DEPLOYMENT_TARGET == TARGET_DB
	    new PollingSplitflap(40, 0, GPIO_NUM_23, GPIO_NUM_19, GPIO_NUM_21, ADC1_CHANNEL_0, 500, 1500);
    #elif DEPLOYMENT_TARGET == TARGET_SBAHN
        //Linie
        new HallSplitflap(64, 63, PORT3, 38000, 14000);
        //Endbahnhof
        new HallSplitflap(80, 69, PORT4, 30000, 4000);
        //Zuglauf
        new HallSplitflap(80, 11, PORT2, 40000, 5000);
    #endif

	while(true) {

		uint32_t cmd_value;
		if (xTaskNotifyWait(0, 0, &cmd_value, 0) == pdTRUE) {
			uint8_t module = 	(cmd_value >> 8) & 0x000000FF;
			uint8_t pos = 		(cmd_value >> 0) & 0x000000FF;

			ESP_LOGD(TAG, "Command: module %d, position %d", module, pos);

            Splitflap::module(module)->set_position(pos);
		} else {

			for (int i = 0; i < Splitflap::number_of_modules(); i++) {
				Splitflap::module(i)->loop();
			}
			
		}

		vTaskDelay(1); 
	}

}