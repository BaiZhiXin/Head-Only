#include <chrono>

class TimeCal {
public:
    // 使用自动推导的时间点类型
    void start() {
        start_time = std::chrono::high_resolution_clock::now();
    }

    double stop() {
        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end_time - start_time;
        return elapsed.count(); // 返回秒数
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
};