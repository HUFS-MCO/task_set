#include "dummy_task.hpp"
#include "time_util.hpp"
#include "uds_util.hpp"
#include <thread>
#include <iostream>
#include <fstream>
#include <mutex>
#include <unistd.h>
#include <atomic>

std::string latest_msg;
std::string prev_msg;
std::mutex msg_mutex;
std::atomic<bool> should_exit{false};

int main() {
    DummyTask planner_func("PLANNER_Function", 24000); // 12ms × 1136 = 13632 iteration
    const int cycle_period_ms = 15;
    const int repeat_count = 2000;

    int recv_fd = create_uds_server("/tmp/ekf_to_planner.sock");
    sleep(1);
    std::cout <<"-------[planner main start]-----------------------------------------------------------------------------------" <<std::endl;
    
    std::ofstream cycle_file("planner_cycle_times.csv");
    cycle_file << "cycle,cycle_elapsed_ms\n";
   
    std::ofstream e2e_file("planner_e2e_latency.csv");
    e2e_file << "cycle,e2e_latency_ms\n";

    std::thread recv_thread([&] {
        char buf[128];
        while (!should_exit) {
            int n = read(recv_fd, buf, sizeof(buf));
            if (n == 0) {
                // 상대방이 소켓을 닫음
                break;
            }
            if (n > 0) {
                std::string msg(buf, n);
                if (msg == "END") {
                    should_exit = true;
                    break;
                }
                std::lock_guard<std::mutex> lock(msg_mutex);
                latest_msg = msg;
                //std::cout << "[PLANNER] Received: " << latest_msg << "\n";
            }
        }
    });

    long long total_exectime = 0;

    for (int cycle = 0; cycle < repeat_count; ++cycle) {
        auto cycle_start = current_time_ms();

        // 작업 시작 시점에 최신 메시지(work_start_time) 저장
        std::string current_msg;
        {
            std::lock_guard<std::mutex> lock(msg_mutex);
            current_msg = latest_msg;
        }

        std::thread t([&] { planner_func.run_once(); });
        t.join();

        auto cycle_elapsed = current_time_ms() - cycle_start;

        auto work_completion_time = current_time_ms();

        // 작업 완료 시점에 E2E latency 계산
        if (!current_msg.empty() && current_msg != prev_msg) {
            long long e2e_latency = current_time_ms() - std::stoll(current_msg); // current_system_time_ms 사용했었음
            e2e_file << cycle << "," << e2e_latency << "\n";
            prev_msg = current_msg;
        }

        cycle_file << cycle << "," << cycle_elapsed << "\n";

        if (cycle_elapsed < cycle_period_ms)
            std::this_thread::sleep_for(std::chrono::milliseconds(cycle_period_ms - cycle_elapsed));

        if (cycle > 9)
            total_exectime += cycle_elapsed;

        planner_func.reset();
    }

    // 수신 스레드가 종료될 때까지 대기
    recv_thread.join();

    cycle_file.close();
    e2e_file.close();
    close(recv_fd);

    std::cout << "[PLANNER] Average exec time: " << (double)total_exectime / (repeat_count - 10) << "ms\n";
    return 0;
}
