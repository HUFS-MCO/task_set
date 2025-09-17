#include "dummy_task.hpp"
#include "time_util.hpp"
#include "uds_util.hpp"
#include <thread>
#include <iostream>
#include <fstream>
#include <mutex>
#include <unistd.h>
#include <atomic> // UDS Error Sol
#include <sched.h>
#include <stdlib.h>
#include <stdio.h>

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

    DummyTask ekf_func("EKF_Function", 800);      // 4ms × 1136 = 4544 iteration

    int recv_fd = create_uds_server("/tmp/localization_to_ekf.sock");
    int send_fd = connect_uds_client("/tmp/ekf_to_planner.sock");
    sleep(1);
    std::cout <<"-------[ekf main start]-----------------------------------------------------------------------------------" <<std::endl;
    
    std::ofstream cycle_file("ekf_cycle_times.csv");
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
                //std::cout << "[EKF] Received: " << latest_msg << "\n";
            }
        }
    });

    auto cycle_start = current_time_ms();

    // 작업 시작 시점에 최신 메시지(work_start_time) 저장
    std::string current_msg;
    {
        std::lock_guard<std::mutex> lock(msg_mutex);
        current_msg = latest_msg;
    }

    std::thread t([&] { ekf_func.run_once(); });
    t.join();

    auto cycle_elapsed = current_time_ms() - cycle_start;

    // 작업 완료 시점에 work_start_time을 그대로 전송 (이전 값과 다르면)
    if (!current_msg.empty() && current_msg != prev_msg) {
       std::string msg_to_send = current_msg; // work_start_time 그대로 전달
        write(send_fd, msg_to_send.c_str(), msg_to_send.size());
        prev_msg = current_msg;
    }

    cycle_file << cycle_elapsed << "\n";
    cycle_file.flush();

    ekf_func.reset();

    // END 신호를 다음 프로세스(planner)에 전달
    write(send_fd, "END", 3);
    // 수신 스레드가 종료될 때까지 대기
    recv_thread.join();

    cycle_file.close();
    close(recv_fd);
    close(send_fd);

    return 0;
}
