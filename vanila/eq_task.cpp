#include "dummy_task.hpp"
#include "time_util.hpp"
#include "monitoring.h"
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

// SCHED_DEADLINE policy constant
#ifndef SCHED_DEADLINE
#define SCHED_DEADLINE 6
#endif

#ifndef SCHED_FLAG_DL_OVERRUN
#define SCHED_FLAG_DL_OVERRUN 0x04
#endif

// Syscall numbers
#ifndef SYS_sched_setattr
#define SYS_sched_setattr 314
#endif

#ifndef SYS_sched_getattr
#define SYS_sched_getattr 315
#endif

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

    long PERIOD_NS = 0;
    const char* period_env = getenv("PERIOD_MS");
    if (period_env) {
        double period_ms = atof(period_env);
        PERIOD_NS = (long)(period_ms * 1e6); // ms -> ns
    } else {
        // 기본값: 20ms
        PERIOD_NS = 20 * 1000 * 1000;
    }

    const char* thresh_env = getenv("RUNTIME_MS");
    long CPU_THRESHOLD_NS = 0;
    if (thresh_env) {
        double thr_ms = atof(thresh_env);
        CPU_THRESHOLD_NS = (long)(thr_ms * 1e6);
    } else {
        CPU_THRESHOLD_NS = 4 * 1000 * 1000; // 4ms in ns
    }

    struct sched_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.size = sizeof(attr);
    attr.sched_policy = SCHED_DEADLINE;
    attr.sched_flags = SCHED_FLAG_DL_OVERRUN;
    attr.sched_runtime = CPU_THRESHOLD_NS;
    attr.sched_deadline = PERIOD_NS;
    attr.sched_period = PERIOD_NS;

    if (syscall(SYS_sched_setattr, 0, &attr, 0) < 0) {
        perror("sched_setattr");
        return 1;
    }

    // Initialize SIGXCPU monitoring
    monitor_init("sigxcpu_log.csv");

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
        auto cycle_elapsed = cycle_end - cycle_start; // wall-clock ms

        cycle_file << cycle_elapsed <<"\n";
        cycle_file.flush();

        eq_func.reset();

        int ret;
        do {
            ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_activation, NULL);
        } while (ret == EINTR);

    }

    cycle_file.close();
    
    // Cleanup monitoring and print summary
    monitor_cleanup();
    
    return 0;
}
