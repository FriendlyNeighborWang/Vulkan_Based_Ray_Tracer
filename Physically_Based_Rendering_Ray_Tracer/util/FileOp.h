#ifndef PBRT_UTIL_FILE_OP_H
#define PBRT_UTIL_FILE_OP_H

#include "base/base.h"
#include "util/pstd.h"

#include <fstream>

class FileOp {
public:
	static pstd::vector<char> readFile(const std::string& filename) {
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open())
			throw std::runtime_error("FileOp:: Failed to open files");

		size_t fileSize = (size_t)file.tellg();
		pstd::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();

		return buffer;
	}
};

#endif // !PBRT_UTIL_FILE_OP_H

