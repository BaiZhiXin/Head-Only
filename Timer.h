#include <thread>
#include <functional>
#include <chrono>

class Timer {
public:
    Timer() : m_running(false) {}

    // 启动定时器，以毫秒级别为间隔执行任务
    void startMilliseconds(int64_t intervalMs, std::function<void()> task) {
        start<std::chrono::milliseconds>(intervalMs, task);
    }

    // 纳秒级别的定时器接口
    void startMicroseconds(int64_t intervalNs, std::function<void()> task) {
        start<std::chrono::microseconds>(intervalNs, task);
    }

    // 纳秒级别的定时器接口
    void startNanoseconds(int64_t intervalNs, std::function<void()> task) {
        start<std::chrono::nanoseconds>(intervalNs, task);
    }

    // 停止定时器
    void stop() {
        m_running = false;
        if (m_thread.joinable()) {
            m_thread.join();  // 等待线程结束
        }
    }

    ~Timer() {
        stop();  // 确保定时器析构时停止
    }

private:
    std::thread m_thread;         // 定时器线程
    std::atomic<bool> m_running;  // 用于控制定时器的状态

    // 通用的定时器实现函数，使用模板参数来适应不同时间单位
    template<typename DurationType>
    void start(int64_t interval, std::function<void()> task) {
        if (m_running) {
            std::cout << "Timer is already running!" << std::endl;
            return;
        }

        m_running = true;
        m_thread = std::thread([=]() {
            auto next_time = std::chrono::steady_clock::now() + DurationType(interval);
            while (m_running) {
                std::this_thread::sleep_until(next_time);
                next_time += DurationType(interval);  // 下次执行的时间点

                if (m_running) {
                    task();  // 每隔指定的时间执行任务
                }
            }
        });
    }
};