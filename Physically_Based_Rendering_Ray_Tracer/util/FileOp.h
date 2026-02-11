#ifndef PBRT_UTIL_FILE_OP_H
#define PBRT_UTIL_FILE_OP_H

#include "base/base.h"
#include "util/pstd.h"

#include <fstream>
#include <filesystem>

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

	static pstd::vector<std::string> findFilesWithPattern(const std::string& directory, const std::string& pattern, bool recursive = true) {
		pstd::vector<std::string> results;

		namespace fs = std::filesystem;

		if (!fs::exists(directory) || !fs::is_directory(directory)) {
			return results;
		}

		auto searchDir = [&](auto&& iterator) {
			for (const auto& entry : iterator) {
				if (entry.is_regular_file()) {
					std::string filename = entry.path().filename().string();
					if (filename.find(pattern) != std::string::npos)
						results.push_back(entry.path().string());
				}
			}
			};

		if (recursive) {
			searchDir(fs::recursive_directory_iterator(directory));
		}
		else {
			searchDir(fs::directory_iterator(directory));
		}

		return results;
	}
};

#endif // !PBRT_UTIL_FILE_OP_H

