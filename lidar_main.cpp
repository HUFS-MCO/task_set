// main.cpp
#include "dummy_task.hpp"
#include "time_util.hpp"
#include "uds_util.hpp"
#include <thread>
#include <iostream>
#include <unistd.h>      // for close
#include <fstream>// 저장용

// 전역 로그 버퍼 변수들 정의
std::vector<std::string> log_buffer;
std::mutex log_mutex;

int main() {
    DummyTask lidar_func("Lidar_Function", 21500);      // 11ms × 1136 = 12496 iteration

    const int cycle_period_ms = 33; // 33ms 주기
    const int repeat_count = 1000; 

    std::ofstream cycle_file("lidar_cycle_times.csv");// 저장용
    cycle_file << "cycle,cycle_elapsed_ms\n";// 저장용


    long long total_exectime = 0;

    // UDS 송신자(클라이언트)로 연결
    const char* uds_path = "/tmp/lidar_to_localization.sock";
    int uds_fd = connect_uds_client(uds_path);
    // 연결이 성립된 후에만 아래 작업(반복문) 시작
    sleep(1);
    std::cout <<"-------[lidar main start]-----------------------------------------------------------------------------------" <<std::endl;
    
    for (int cycle = 0; cycle < repeat_count; ++cycle) {
        log_event("Main", "Cycle " + std::to_string(cycle) + " started");
        auto cycle_start = current_time_ms();
        

        std::thread t2([&] {
            lidar_func.run_once();
        });
        t2.join();

        auto cycle_elapsed = current_time_ms() - cycle_start;
        auto end_time = current_time_ms(); // 원래 current_system_time_ms 사용했었음

        // 작업이 끝난 후, work_start_time을 UDS로 송신
        std::string msg = std::to_string(end_time);
        write(uds_fd, msg.c_str(), msg.size());
        // std::cout << "[LIDAR] Sent message: " << msg << " at " << sent_time << "ms" << std::endl;

        // 전체 사이클 시간 계산 및 대기
        cycle_file << cycle << "," << cycle_elapsed << "\n";// 저장용


        log_event("Main", "Cycle " + std::to_string(cycle) + " actual execution time: " + std::to_string(cycle_elapsed) + "ms");
        if (cycle_elapsed < cycle_period_ms) {
            auto wait_time = cycle_period_ms - cycle_elapsed;
            std::this_thread::sleep_for(std::chrono::milliseconds(wait_time));
        } else {
            log_event("Main", "Cycle " + std::to_string(cycle) + " exceeded target period by: " + std::to_string(cycle_elapsed - cycle_period_ms) + "ms");
        }
         // 결과 출력
        log_event("Main", "Total iterations in 11ms: " + std::to_string(lidar_func.get_iteration_count()));

        // 총 시간 누적
        if(cycle > 9){
            total_exectime += cycle_elapsed;
        }
        

        // 다음 사이클을 위해 ready 플래그 리셋
        lidar_func.reset();
    }

    // 마지막에 END 신호 전송
    write(uds_fd, "END", 3);
    // END 전송 후 잠시 대기 (수신자가 처리할 시간)
    sleep(1);

    // 평균 계산 및 출력
    double avg_exectime = (double)total_exectime / (repeat_count-10);
    log_event("Main", "Average execution time per cycle: " + std::to_string(avg_exectime));

    cycle_file.close();// 저장용

    // 모든 로그를 한번에 출력
    flush_logs();
    close(uds_fd);
    return 0;
}
