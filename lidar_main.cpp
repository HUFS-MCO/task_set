// main.cpp
#include "dummy_task.hpp"
#include "time_util.hpp"
#include "uds_util.hpp"
#include <thread>
#include <iostream>
#include <fstream>// 저장용
#include <sched.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h> // clock_nanosleep을 위해 추가

// 전역 로그 버퍼 변수들 정의
std::vector<std::string> log_buffer;
std::mutex log_mutex;

int main() {
    struct sched_param param;
    param.sched_priority = 99;


    if (sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
        perror("sched_setscheduler 실패");
        return 1;
    }

    DummyTask lidar_func("Lidar_Function", 1515);      // 11ms × 1136 = 12496 iteration

    std::ofstream cycle_file("lidar_cycle_times.csv");// 저장용
    cycle_file << "cycle_elapsed_ms\n";// 저장용

    // UDS 송신자(클라이언트)로 연결
    const char* uds_path = "/tmp/lidar_to_localization.sock";
    int uds_fd = connect_uds_client(uds_path);
    // 연결이 성립된 후에만 아래 작업(반복문) 시작
    sleep(1);
    std::cout <<"-------[lidar main start]-----------------------------------------------------------------------------------" <<std::endl;
    
    // 3. 주기적 실행을 위한 시간 설정
    struct timespec next_activation;
    const long PERIOD_NS = 33 * 1000 * 1000; // 33ms를 나노초로 변환

    // 첫 번째 활성화 시간을 현재 시간으로 설정
    clock_gettime(CLOCK_MONOTONIC, &next_activation);

    for (int i = 0; i<100; ++i){
        auto cycle_start = current_time_ms();
            
        std::thread t2([&] {
            lidar_func.run_once();
        });
        t2.join();

    auto cycle_elapsed = current_time_ms() - cycle_start;
    auto end_time = current_time_ms(); // 원래 current_system_time_ms 사용했었음

        // 작업이 끝난 후, work_start_time을 UDS로 송신
        std::string msg = std::to_string(end_time) + "\n";
        write(uds_fd, msg.c_str(), msg.size());

        cycle_file << cycle_elapsed << "\n";// 저장용
        cycle_file.flush();
            
        // 다음 사이클을 위해 ready 플래그 리셋
        lidar_func.reset();
        // --- 다음 활성화 시간까지 대기 ---
        // 이전 활성화 시간에 주기를 더하여 다음 활성화 시간 계산
        next_activation.tv_nsec += PERIOD_NS;
        // 나노초가 1초를 넘어가면 초(second) 단위로 올림 처리
        while (next_activation.tv_nsec >= 1000000000) {
            next_activation.tv_nsec -= 1000000000;
            next_activation.tv_sec++;
        }

        // clock_nanosleep을 사용하여 계산된 절대 시간까지 프로세스를 재움
        // TIMER_ABSTIME 플래그는 드리프트(drift) 누적을 방지하여 정밀한 주기를 보장
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_activation, NULL);
    }
    // 마지막에 END 신호 전송
    write(uds_fd, "END", 3);
    // END 전송 후 잠시 대기 (수신자가 처리할 시간)
    sleep(1);

    cycle_file.close();// 저장용

    // 모든 로그를 한번에 출력
    flush_logs();
    close(uds_fd);
    return 0;
}
