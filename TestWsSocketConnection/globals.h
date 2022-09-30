#pragma once

#include <stdint.h>

const int AUDIO_BUFFER_LENGTH = 640 * 20;

struct Data {
	uint32_t node_id;
	unsigned int bufferLength;
	char buffer[AUDIO_BUFFER_LENGTH];
};