#pragma once

#include <stdint.h>

const int AUDIO_BUFFER_LENGTH = 32*200;

struct Data {
	enum class Code {
		RECONNECT = -1
	};
	uint32_t node_id;
	unsigned int bufferLength;
	char buffer[AUDIO_BUFFER_LENGTH];
};