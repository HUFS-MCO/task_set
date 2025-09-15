// dummy_task.hpp
#pragma once
#include <thread>
#include <string>
#include <vector>
#include <condition_variable>
#include <mutex>
#include "time_util.hpp"
#include <random>
#include <limits>
#include <algorithm>

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

        // MATLAB randfixedsum 포팅: [a,b]^n에서 합이 s인 조건부 균일 샘플
        auto randfixedsum = [&](int n, int m, double s, double a, double b, std::mt19937& rng){
          if(m < 0 || n < 1) throw std::runtime_error("invalid n or m");
          if(s < n*a || s > n*b || !(a < b)) throw std::runtime_error("constraints n*a <= s <= n*b, a<b must hold");
          // Rescale to unit cube
          s = (s - n*a) / (b - a);
          int k = std::max(std::min(static_cast<int>(std::floor(s)), n-1), 0);
          s = std::max(std::min(s, static_cast<double>(k+1)), static_cast<double>(k));
          std::vector<double> s1(n), s2(n);
          for(int i=0;i<n;i++){
            s1[i] = s - (k - i);
            s2[i] = (k + n - i) - s;
          }
          const double realmax = std::numeric_limits<double>::max();
          const double tiny = std::numeric_limits<double>::denorm_min();
          std::vector<std::vector<double>> w(n, std::vector<double>(n+1, 0.0));
          w[0][1] = realmax;
          std::vector<std::vector<double>> t(n-1, std::vector<double>(n, 0.0));
          for(int i=1;i<n;i++){
            // tmp1 = w(i-1,2:i+1).*s1(1:i)/i;
            // tmp2 = w(i-1,1:i).*s2(n-i+1:n)/i;
            for(int j=0;j<=i;j++){
              double tmp1 = (j>0 ? w[i-1][j] * s1[j-1] / static_cast<double>(i) : 0.0);
              double tmp2 = w[i-1][j] * s2[n-i+j-1] / static_cast<double>(i);
              w[i][j+1] = tmp1 + tmp2;
            }
            std::vector<double> tmp3(i+1, 0.0);
            for(int j=0;j<=i;j++) tmp3[j] = w[i][j+1] + tiny;
            for(int j=0;j<i;j++){
              bool left = (s2[n-i+j-1] > s1[j]);
              double part = (w[i-1][j] * s2[n-i+j-1] / static_cast<double>(i)) / tmp3[j+1];
              double part2 = 1.0 - (w[i-1][j+1] * s1[j] / static_cast<double>(i)) / tmp3[j+1];
              t[i-1][j] = (left ? part : part2);
            }
          }
          // volume v omitted
          std::vector<std::vector<double>> x(n, std::vector<double>(m, 0.0));
          if(m == 0) return x;
          std::uniform_real_distribution<double> uni(0.0, 1.0);
          std::vector<std::vector<double>> rt(n-1, std::vector<double>(m, 0.0));
          std::vector<std::vector<double>> rs(n-1, std::vector<double>(m, 0.0));
          for(int i=0;i<n-1;i++){
            for(int j=0;j<m;j++){ rt[i][j] = uni(rng); rs[i][j] = uni(rng); }
          }
          std::vector<double> sj(m, s);
          std::vector<int> jj(m, k+1);
          std::vector<double> sm(m, 0.0), pr(m, 1.0);
          for(int ii=n-1; ii>=1; --ii){
            int row = n - ii;
            for(int col=0; col<m; ++col){
              bool e = (rt[row-1][col] <= t[ii-1][jj[col]-1]);
              double sx = std::pow(rs[row-1][col], 1.0/static_cast<double>(ii));
              sm[col] += (1.0 - sx) * pr[col] * sj[col] / static_cast<double>(ii+1);
              pr[col] = sx * pr[col];
              x[row-1][col] = sm[col] + pr[col] * (e ? 1.0 : 0.0);
              if(e){ sj[col] -= 1.0; jj[col] -= 1; }
            }
          }
          for(int col=0; col<m; ++col){
            x[n-1][col] = sm[col] + pr[col] * sj[col];
          }
          // Randomly permute rows in each column
          for(int col=0; col<m; ++col){
            std::vector<std::pair<double,int>> key(n);
            for(int i=0;i<n;i++) key[i] = {uni(rng), i};
            std::sort(key.begin(), key.end());
            std::vector<double> colvals(n);
            for(int i=0;i<n;i++) colvals[i] = x[i][col];
            for(int i=0;i<n;i++) x[i][col] = colvals[key[i].second];
          }
          // Rescale
          for(int i=0;i<n;i++) for(int j=0;j<m;j++) x[i][j] = (b - a) * x[i][j] + a;
          return x;
        };

        // 각 행의 합 3*5.5=16.5, 구간 [1,10]
        const double row_sum = 16.5;
        for(int k=0; k<3; ++k){
          auto XA = randfixedsum(3, 1, row_sum, 1.0, 10.0, gen);
          auto XB = randfixedsum(3, 1, row_sum, 1.0, 10.0, gen);
          for(int j=0; j<3; ++j){
            A[k][j] = XA[j][0];
            B[k][j] = XB[j][0];
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
