#ifndef PBRT_UTIL_LOG_H
#define PBRT_UTIL_LOG_H

#include <iostream>
#include <sstream>
#include <string>
#include <mutex>
#include <memory>
#include <unordered_map>

#ifdef DEBUG

// TODO: ∂‡œﬂ≥Ã¬“–Ú

class LogStream {
public:
	LogStream(const std::string& name, bool enabled = true):name(name), enabled(enabled){}

	void enable() { enabled = true; }
	void disable() { enabled = false; }
	bool isEnabled() const { return enabled; }

	template <typename T>
	LogStream& operator<<(const T& value) {
		if (enabled) {
			std::lock_guard<std::recursive_mutex> lock(mut);
			buffer << value;
		}
		return *this;
	}

	LogStream& operator<<(std::ostream& (*manip)(std::ostream&)) {
		if (enabled) {
			std::lock_guard<std::recursive_mutex> lock(mut);
			buffer << manip;
			flush();
		}
		return *this;
	}
	void flush() {
		if (enabled) {
			std::lock_guard<std::recursive_mutex> lock(mut);
			std::cout << "[" << name << "]" << buffer.str();
			std::cout.flush();
			buffer.str("");
			buffer.clear();
		}
	}

private:
	std::string name;
	bool enabled;
	std::ostringstream buffer;
	mutable std::recursive_mutex mut;
};

class LogController {
public:
	static LogController& getInstance() {
		static LogController instance;
		return instance;
	}

	LogStream& registerStream(const std::string& name, bool enabled = true) {
		std::lock_guard<std::mutex> lock(mut);
		auto it = streams.find(name);
		if (it == streams.end()) {
			streams.emplace(name, std::make_unique<LogStream>(name, enabled));
		}
		return *streams.at(name);
	}

	LogStream& getStream(const std::string& name) {
		{
			std::lock_guard<std::mutex> lock(mut);
			auto it = streams.find(name);
			if (it != streams.end()) {
				return *(it->second);
			}
		}
		
		return registerStream(name, true);
	}

	void enableStream(const std::string& name) {
		std::lock_guard<std::mutex> lock(mut);
		auto it = streams.find(name);
		if (it != streams.end()) {
			it->second->enable();
		}
	}

	void disableStream(const std::string& name) {
		std::lock_guard<std::mutex> lock(mut);
		auto it = streams.find(name);
		if (it != streams.end()) {
			it->second->disable();
		}
	}

	void enableAll() {
		std::lock_guard<std::mutex> lock(mut);
		for (auto& it : streams) {
			it.second->enable();
		}
	}

	void disableAll() {
		std::lock_guard<std::mutex> lock(mut);
		for (auto& it : streams) {
			it.second->disable();
		}
	}

private:
	LogController() = default;
	~LogController() = default;
	LogController(const LogController&) = delete;
	LogController& operator=(const LogController&) = delete;

	std::unordered_map<std::string, std::unique_ptr<LogStream>> streams;
	mutable std::mutex mut;
};

#else

class LogStream {
public:
	LogStream(const std::string & = "", bool = true) {}
	void enable() {}
	void disable() {}
	bool isEnabled() const { return false; }

	template <typename T>
	LogStream& operator<<(const T&) { return *this; }

	LogStream& operator<<(std::ostream& (*)(std::ostream&)) { return *this; }

	void flush() {}
};

class LogController {
public:
	static LogController& getInstance() {
		static LogController instance;
		return instance;
	}

	LogStream& registerStream(const std::string&, bool = true) { return dummy; }
	LogStream& getStream(const std::string&) { return dummy; }
	void enableStream(const std::string&) {}
	void disableStream(const std::string&) {}
	void enableAll() {}
	void disableAll() {}

private:
	LogStream dummy;
};

#endif  // DEBUG

#define LOG_STREAM(name) LogController::getInstance().getStream(name)
#define REGISTER_LOG(name, enabled) LogController::getInstance().registerStream(name, enabled)
#define ENABLE_LOG(name) LogController::getInstance().enableStream(name)
#define DISABLE_LOG(name) LogController::getInstance().disableStream(name)
#define ENABLE_ALL_LOGS() LogController::getInstance().enableAll()
#define DISABLE_ALL_LOGS() LogController::getInstance().disableAll()



#endif // !PBRT_UTIL_LOG_H