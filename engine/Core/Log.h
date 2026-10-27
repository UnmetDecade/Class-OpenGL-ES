#pragma once

#include <iostream>

#define LOG_INFO(message)  (std::cout << "[INFO ] " << __FILE__ << ":" << __LINE__ << " " << message << std::endl)
#define LOG_WARN(message)  (std::cerr << "[WARN ] " << __FILE__ << ":" << __LINE__ << " " << message << std::endl)
#define LOG_ERROR(message) (std::cerr << "[ERROR] " << __FILE__ << ":" << __LINE__ << " " << message << std::endl)
