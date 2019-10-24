#include "splitflap.h"

std::vector<Splitflap*> Splitflap::instances;

Splitflap::Splitflap(uint8_t flap_count_, uint8_t zero_offset_) {
	id = instances.size();
	instances.push_back(this);

	flap_count = flap_count_;
	zero_offset = zero_offset_;
}
