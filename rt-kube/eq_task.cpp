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

// Global variable for SIGXCPU counting when monitoring is disabled
int sigxcpu_counter1 = 0;

// Simple SIGXCPU handler for non-monitoring mode
void simple_sigxcpu_handler(int sig) {
    sigxcpu_counter1++;
}

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
    // Get runtime from environment variable, default to 4ms
    const char* runtime_env = getenv("RUNTIME_MS");
    int runtime_ms = runtime_env ? atoi(runtime_env) : 4;
    uint64_t runtime_ns = runtime_ms * 1000ULL * 1000ULL;  // Convert ms to ns
    
    // Get deadline from environment variable, default to 20ms
    const char* deadline_env = getenv("DEADLINE_MS");
    int deadline_ms = deadline_env ? atoi(deadline_env) : 20;
    uint64_t deadline_ns = deadline_ms * 1000ULL * 1000ULL;  // Convert ms to ns
    
    // Get period from environment variable, default to 20ms
    const char* period_env = getenv("PERIOD_MS");
    int period_ms = period_env ? atoi(period_env) : 20;
    uint64_t period_ns = period_ms * 1000ULL * 1000ULL;  // Convert ms to ns

    struct sched_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.size = sizeof(attr);
    attr.sched_policy = SCHED_DEADLINE;
    attr.sched_flags = SCHED_FLAG_DL_OVERRUN;
    attr.sched_runtime = runtime_ns;
    attr.sched_deadline = deadline_ns;
    attr.sched_period = period_ns;

    // Check if monitoring is enabled
    const char* monitoring_env = getenv("ENABLE_MONITORING");
    bool enable_monitoring = monitoring_env && (strcmp(monitoring_env, "true") == 0 || strcmp(monitoring_env, "1") == 0);
    
    if (enable_monitoring) {
        printf("📊 Monitoring enabled\n");
        monitor();
    } else {
        printf("📊 Monitoring disabled (simple SIGXCPU counting only)\n");
        signal(SIGXCPU, simple_sigxcpu_handler);
    }

    printf("sizeof(attr) = %zu\n", sizeof(attr));
    printf("🟢 SCHED_DEADLINE test starting (tid=%ld)\n", (long)syscall(SYS_gettid));
    printf("⚙️  Runtime: %dms, Deadline: %dms, Period: %dms\n", 
           runtime_ms, deadline_ms, period_ms);
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

    sched_yield();
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
