#include "dummy_task.hpp"
#include "time_util.hpp"
#include <thread>
#include <iostream>
#include <fstream>
#include <mutex>
#include <unistd.h>
#include <atomic>
#include <sched.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <cstring>
#include <sys/syscall.h>
#include <linux/sched.h>
#include <linux/types.h>

// ===== 안전한 timespec 덧셈 helper =====
inline void timespec_add_ns(struct timespec &ts, long ns) {
    ts.tv_nsec += ns;
    while (ts.tv_nsec >= 1000000000L) {
        ts.tv_nsec -= 1000000000L;
        ts.tv_sec++;
    }
}

int main() {
    // 변수 선언
    struct timespec next_activation;

    // cgroup에서 RT period 값을 직접 읽기
    long PERIOD_NS = 40 * 1000 * 1000; // 기본값: 40ms
    std::ifstream period_file("/sys/fs/cgroup/cpu.rt_period_us");
    long period_us;
    if (period_file >> period_us) {
        PERIOD_NS = period_us * 1000; // microseconds → nanoseconds
        printf("Using RT period: %ld µs (%ld ns)\n", period_us, PERIOD_NS);
    } else {
        printf("Using default period: %ld ns\n", PERIOD_NS);
    }
    period_file.close();

    // SCHED_FIFO 설정
    struct sched_param param;
    param.sched_priority = 99;
    if (sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
        perror("sched_setscheduler 실패");
        return 1;
    }

    // Workload / Iterations 환경 변수 설정
    const char* workload_env = getenv("WORKLOAD");
    int workload = workload_env ? atoi(workload_env) : 300;
    printf("📊 Workload iterations: %d\n", workload);

    const char* iterations_env = getenv("ITERATIONS");
    int num_iterations = iterations_env ? atoi(iterations_env) : 500;
    printf("🔄 Total iterations: %d\n", num_iterations);

    DummyTask eq_func("EQ_Function", workload);
    std::ofstream cycle_file("eq_cycle_times.csv");
    cycle_file << "cycle_response_ms" << "\n";

    std::cout << "-------[eq main start]-------" << std::endl;

    // 첫 활성화 시간을 "현재 시각 + 한 주기"로 맞춤
    clock_gettime(CLOCK_MONOTONIC, &next_activation);
    timespec_add_ns(next_activation, PERIOD_NS);


    for (int i = 0; i < num_iterations; ++i) {
        timespec_add_ns(next_activation, PERIOD_NS);
        auto cycle_start = current_time_ms();

        // 태스크 실행
        eq_func.run_once();

        auto cycle_end = current_time_ms();
        auto cycle_elapsed = cycle_end - cycle_start;

        cycle_file << cycle_elapsed <<"\n";
        cycle_file.flush();

        eq_func.reset();

        int ret;
        do {
            ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_activation, NULL);
        } while (ret == EINTR);

    }

    cycle_file.close();
    return 0;
}
