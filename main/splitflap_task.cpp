#include "splitflap_task.h"

#if  DEPLOYMENT_TARGET == TARGET_DB
    #include "polling_splitflap.h"
#elif DEPLOYMENT_TARGET == TARGET_SBAHN
    #include "hall_splitflap.h"
#endif

#ifdef TARGET_DB

#endif

TaskHandle_t SplitflapTask::handle;

#define PORT3 GPIO_NUM_25, GPIO_NUM_26, GPIO_NUM_27
#define PORT4 GPIO_NUM_19, GPIO_NUM_18, GPIO_NUM_5
#define PORT2 GPIO_NUM_23, GPIO_NUM_22, GPIO_NUM_21

void SplitflapTask::worker(void* arg) {
	ESP_LOGI(TAG_FLAP, "Splitflap-Task started.");

    #if DEPLOYMENT_TARGET == TARGET_DB
	    new PollingSplitflap(40, 0, GPIO_NUM_23, GPIO_NUM_19, GPIO_NUM_21, ADC1_CHANNEL_0, 500, 1500);
    #elif DEPLOYMENT_TARGET == TARGET_SBAHN
        //Linie
        new HallSplitflap(64, 62, PORT3, 35000, 5000);
        //Endbahnhof
        new HallSplitflap(80, 70, PORT4, 30000, 15000);
        //Zuglauf
        new HallSplitflap(80, 11, PORT2, 45000, 0000);
    #endif
	
		
	while(true) {

		uint32_t cmd_value;
		if (xTaskNotifyWait(0, 0, &cmd_value, /*portMAX_DELAY*/ 0) == pdTRUE) {
			uint8_t module = 	(cmd_value >> 8) & 0x000000FF;
			uint8_t pos = 		(cmd_value >> 0) & 0x000000FF;

			ESP_LOGI(TAG_FLAP, "Command: module %d, position %d", module, pos);
			Splitflap::module(module)->set_position(pos);
		} else {

			for (int i = 0; i < Splitflap::number_of_modules(); i++) {
				Splitflap::module(i)->loop();
			}
			
		}

        #if OPTION_EVENT_BITS_DEBUG
            EventBits_t status_bits = xEventGroupGetBits(spf->getStatusBits());
            xEventGroupClearBits(spf->getStatusBits(), 0xFF);

            if ((status_bits & 0x01) != 0) {
                printf("home\n");
            }

            if ((status_bits & 0x02) != 0) {
                printf("flap\n");
            }

            if ((status_bits & 0x000100) != 0) {
                printf("ignoring\n");
            }
        #endif

		vTaskDelay(1); 
	}

}