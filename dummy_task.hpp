// dummy_task.hpp
#pragma once
#include <thread>
#include <string>
#include <vector>
#include <condition_variable>
#include <mutex>
#include "time_util.hpp"
#include <random>

class DummyTask {
public:
    std::string name;
    int iteration;
    std::vector<DummyTask*> dependencies;
    std::condition_variable cv;
    std::mutex mtx;
    bool ready = false;
    long long iteration_count = 0;  // iteration 카운트 추가

    // 랜덤 엔진과 분포를 멤버로 추가
    std::mt19937 gen;
    std::uniform_real_distribution<> dis;

    DummyTask(std::string n, int itr)
        : name(std::move(n)), iteration(itr),
          gen(std::random_device{}()), dis(1.0, 10.0) {}

    void add_dependency(DummyTask* task) {
        dependencies.push_back(task);
    }

    void run_once() {
      auto start_time = current_time_ms();
      iteration_count = 0;

      double A[3][3];
      double B[3][3];
      double C[3][3];

      for(int i=0; i<iteration; i++){
        iteration_count++;

        // A, B를 매번 랜덤 값으로 채움
        for(int k=0; k<3; ++k){
          for(int j=0; j<3; ++j){
            A[k][j] = dis(gen);
            B[k][j] = dis(gen);
          }
        }

        for(int k=0; k<3; ++k){
          for(int j=0; j<3; ++j){
            C[k][j] = 0;
            for(int e=0; e<3; ++e){
              C[k][j] += A[k][e] * B[e][j];
            }
          }
        }
      }
      /*while(current_time_ms() - start_time < duration_ms) {
            iteration_count++;  // iteration 카운트 증가
        }
      */
        auto end_time = current_time_ms();
        auto actual_duration = end_time - start_time;
        /* log_event(name, "ended - actual duration: " + std::to_string(actual_duration) + 
                       "ms (target: " + std::to_string(duration_ms) + 
                       "ms), iterations: " + std::to_string(iteration_count)); */
        {
            std::lock_guard<std::mutex> lock(mtx);
            ready = true;
        }
        cv.notify_all();
    }

    void wait_until_ready() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&]{ return ready; });
    }

    void reset() {
        std::lock_guard<std::mutex> lock(mtx);
        ready = false;
    }

    long long get_iteration_count() const {
        return iteration_count;
    }
};
