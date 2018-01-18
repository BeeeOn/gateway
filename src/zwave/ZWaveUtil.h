#pragma once

#include <string>

namespace BeeeOn {

class ZWaveUtil {
public:
	ZWaveUtil() = delete;
	ZWaveUtil(const ZWaveUtil &) = delete;
	ZWaveUtil(const ZWaveUtil &&) = delete;
	~ZWaveUtil() = delete;

	static std::string commandClass(const uint8_t cclass);
	static std::string commandClass(const uint8_t cclass, const uint8_t index);
};

}
