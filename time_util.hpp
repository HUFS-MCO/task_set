// time_utils.hpp
#pragma once
#include <chrono>
#include <string>
#include <iostream>
#include <vector>
#include <mutex>

inline long long current_time_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}
/*
// 현재 실행할 task chain은 어차피 하나의 노드에서 실행되기에 system time 필요없음 -> steady clock으로 대체 가능
inline long long current_system_time_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}
*/


// 로그 버퍼를 위한 전역 변수들
extern std::vector<std::string> log_buffer;
extern std::mutex log_mutex;

inline void log_event(const std::string& task_name, const std::string& event) {
    std::lock_guard<std::mutex> lock(log_mutex);
    std::string log_entry = "[" + std::to_string(current_time_ms()) + "ms] " + task_name + " " + event;
    log_buffer.push_back(log_entry);
}

inline void flush_logs() {
    std::lock_guard<std::mutex> lock(log_mutex);
    for (const auto& log : log_buffer) {
        std::cout << log << std::endl;
    }
    log_buffer.clear();
}
