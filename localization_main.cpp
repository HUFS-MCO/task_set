#include "dummy_task.hpp"
#include "time_util.hpp"
#include "uds_util.hpp"
#include <thread>
#include <iostream>
#include <fstream>
#include <mutex>
#include <unistd.h>
#include <atomic>
#include <sched.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h> // clock_nanosleep을 위해 추가

std::string latest_msg;
std::string prev_msg;
std::mutex msg_mutex;
std::atomic<bool> should_exit{false};

int main() {
    struct sched_param param;
    param.sched_priority = 99;

    if (sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
        perror("sched_setscheduler 실패");
        return 1;
    }

    DummyTask localization_func("LOCALIZATION_Function", 19081);      // 139ms × 1136 = 157904 iteration


    int recv_fd = create_uds_server("/tmp/lidar_to_localization.sock");
    int send_fd = connect_uds_client("/tmp/localization_to_ekf.sock");
    sleep(1);
    std::cout <<"-------[localization main start]-----------------------------------------------------------------------------------" <<std::endl;
    
    std::ofstream cycle_file("localization_cycle_times.csv");
    cycle_file << "cycle_elapsed_ms\n";

    std::thread recv_thread([&] { // UDS Error Sol
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
                //std::cout << "[LOCALIZATION] Received: " << latest_msg << "\n";
            }
        }
    });
    // 3. 주기적 실행을 위한 시간 설정
    struct timespec next_activation;
    const long PERIOD_NS = 400 * 1000 * 1000; // 33ms를 나노초로 변환

    // 첫 번째 활성화 시간을 현재 시간으로 설정
    clock_gettime(CLOCK_MONOTONIC, &next_activation);

    for (int i=0; i<100; ++i){
        auto cycle_start = current_time_ms();

    // 작업 시작 시점에 최신 메시지(work_start_time) 저장
    std::string current_msg;
    {
        std::lock_guard<std::mutex> lock(msg_mutex);
        current_msg = latest_msg;
    }

        std::thread t([&] { localization_func.run_once(); });
        t.join();

        auto cycle_elapsed = current_time_ms() - cycle_start;

        // 작업 완료 시점에 work_start_time을 그대로 전송
        if (!current_msg.empty() && current_msg != prev_msg) {
            std::string msg_to_send = current_msg + "\n"; // work_start_time 그대로 전달
            write(send_fd, msg_to_send.c_str(), msg_to_send.size());
            prev_msg = current_msg;
        }

        cycle_file << cycle_elapsed << "\n";
        cycle_file.flush();

        localization_func.reset();
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

    // END 신호를 다음 프로세스(ekf)에 전달
    write(send_fd, "END", 3);

    // 수신 스레드가 종료될 때까지 대기
    recv_thread.join();

    cycle_file.close();
    close(recv_fd);
    close(send_fd);

    return 0;
}
