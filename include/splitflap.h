#ifndef SPLITFLAP_H_
#define SPLITFLAP_H_

#include <vector>
#include <stdint.h>
#include "names.h"


class Splitflap {

public:
	Splitflap(uint8_t flap_count_, uint8_t zero_offset_);

	virtual void set_position(uint8_t) = 0;
	virtual void loop() { };
	virtual uint8_t get_position() = 0;
	virtual uint8_t get_commanded_position() = 0;
	virtual uint8_t number_of_flaps() { return flap_count; };
	
	static uint8_t number_of_modules() { return instances.size(); };
	static Splitflap* module(uint8_t id) { return instances.at(id); };

protected:
	uint8_t id;

	// basic settings
	uint8_t flap_count;
	uint8_t zero_offset;

	static std::vector<Splitflap*> instances;
};

#endif //SPLITFLAP_H_