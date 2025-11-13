#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <fstream>
#include <mutex>

// Global variables for SIGXCPU tracking
static int sigxcpu_counter = 0;
static std::ofstream sigxcpu_log;
static std::mutex sigxcpu_mutex;

// Signal handler for SIGXCPU
static void sigxcpu_handler(int sig) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    
    sigxcpu_counter++;
    
    // Lock to prevent concurrent writes
    std::lock_guard<std::mutex> lock(sigxcpu_mutex);
    
    // Log to console
    printf("[SIGXCPU #%d] Received at %ld.%09ld (absolute time)\n", 
           sigxcpu_counter, ts.tv_sec, ts.tv_nsec);
    
    // Log to CSV if file is open
    if (sigxcpu_log.is_open()) {
        sigxcpu_log << sigxcpu_counter << "," 
                    << ts.tv_sec << "." << ts.tv_nsec 
                    << "\n";
        sigxcpu_log.flush();
    }
}

// Initialize monitoring: setup signal handler and open CSV
void monitor_init(const char* csv_filename = "sigxcpu_log.csv") {
    // Open CSV file
    sigxcpu_log.open(csv_filename);
    if (sigxcpu_log.is_open()) {
        sigxcpu_log << "sigxcpu_count,timestamp\n";
        sigxcpu_log.flush();
    }
    
    // Setup signal handler
    signal(SIGXCPU, sigxcpu_handler);
    
    printf("📊 SIGXCPU monitoring initialized, logging to %s\n", csv_filename);
}

// Cleanup: close CSV file
void monitor_cleanup() {
    if (sigxcpu_log.is_open()) {
        sigxcpu_log.close();
    }
    printf("📊 SIGXCPU monitoring stopped. Total signals: %d\n", sigxcpu_counter);
}

// Get current SIGXCPU count
int get_sigxcpu_count() {
    return sigxcpu_counter;
}