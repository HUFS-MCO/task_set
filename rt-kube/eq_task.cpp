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
#include "monitoring.h"

//#define gettid() syscall(SYS_gettid)
#define RUNTIME_NS  (4ULL * 1000 * 1000)    // 4ms
#define DEADLINE_NS (20ULL * 1000 * 1000)   // 20ms
#define PERIOD_NS   (20ULL * 1000 * 1000)   // 20ms

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

int main() {
    struct sched_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.size = sizeof(attr);
    attr.sched_policy = SCHED_DEADLINE;
    attr.sched_flags = SCHED_FLAG_DL_OVERRUN;
    attr.sched_runtime = RUNTIME_NS;
    attr.sched_deadline = DEADLINE_NS;
    attr.sched_period = PERIOD_NS;

    monitor();

    printf("sizeof(attr) = %zu\n", sizeof(attr));
    printf("🟢 SCHED_DEADLINE test starting (tid=%ld)\n", (long)syscall(SYS_gettid));
    //printf("🟢 SCHED_DEADLINE test starting (tid=%ld)\n", gettid());
    if (syscall(SYS_sched_setattr, 0, &attr, 0) < 0) {
        perror("sched_setattr");
        return 1;
    }

    // Get workload from environment variable, default to 300
    const char* workload_env = getenv("WORKLOAD");
    int workload = workload_env ? atoi(workload_env) : 300;
    printf("📊 Workload iterations: %d\n", workload);

    DummyTask eq_func("EQ_Function", workload);
    //DummyTask eq_func("EQ_Function", 300);

    std::cout << "-------[eq main start]-------" << std::endl;

    // Get number of iterations from environment variable, default to 500
    const char* iterations_env = getenv("ITERATIONS");
    int num_iterations = iterations_env ? atoi(iterations_env) : 500;
    printf("🔄 Total iterations: %d\n", num_iterations);

    std::ofstream cycle_file("eq_cycle_times.csv");
    cycle_file << "cycle_elapsed_ms\n";

    // 4. 주기적 루프 (예: 100번 반복)
    for (int i = 0; i < num_iterations; ++i) {
        auto cycle_start = current_time_ms();

        // Execute task directly in main thread (no std::thread!)
        eq_func.run_once();

        auto cycle_end = current_time_ms();
        auto cycle_elapsed = cycle_end - cycle_start;

        cycle_file << cycle_elapsed << "\n";
        cycle_file.flush();

        eq_func.reset();
        sched_yield();
        
    }

    cycle_file.close();

    return 0;
}
