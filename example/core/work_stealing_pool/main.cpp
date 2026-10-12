#include "mino/core/thread/work_stealing_thread_pool.hpp"

#include <chrono>
#include <future>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>

int main() {
    //-----------------------------------------------------
    // 1. 4개의 스레드로 구성된 작업 훔침 스레드 풀 생성

    namespace mct = mino::core::thread;
    using work_stealing_thread_pool = mct::work_stealing_thread_pool;
    work_stealing_thread_pool pool(4); 

    auto thread_count = pool.thread_count();
    std::cout << "[1] [mino] work stealing thread pool started with " << thread_count << " threads\n";

    //-----------------------------------------------------
    // 2. 일반 비동기 작업 제출

    std::cout << "[2] [mino] submitting 8 tasks to the pool...\n";

    // 제곱(square) 작업을 생성하는 람다 함수. (로직 함수)
    // 이 람다 함수는 외부 종속성 없이 독립적으로 실행 가능.
    auto make_square_task = [](int value) {
        return [value]() -> int {
            using namespace std::chrono_literals;
            using std::this_thread::sleep_for;
            sleep_for(10ms); // 잠깐 대기 (쓰레드 풀의 동시성 테스트를 위해)
            return value * value; // 제곱 계산값 반환
        };
    };

    // 8개의 작업을 제출하고 결과를 수집
    // 8은 임의의 숫자이며, 실제로는 더 많은 작업도 제출할 수 있음.
    // 보통 CPU 코어 수보다 많은 작업을 제출하여 스레드 풀의 효율성을 테스트함.
    // 현재 쓰레드 풀의 쓰레드는 4개이므로, 8개의 작업을 동시에 제출하면 일부 작업은 대기 상태가 됨.
    std::vector<std::future<int>> results;
    for (int i = 0; i < 8; ++i) {
        auto square_future = pool.submit(make_square_task(i));
        results.push_back(std::move(square_future));
    }

    // 8개의 결과를 가져와 출력
    for (std::size_t i = 0; i < results.size(); ++i) {
        std::cout << "task " << i << " result: " << results[i].get() << '\n';
    }

    //-----------------------------------------------------
    // 3. 워커 내부에서 하위 작업을 분기(Fork-Join)하는 패턴

    std::cout << "[3] [mino] submitting a task that spawns sub-tasks...\n";

    // 상위 작업을 제출하고, 그 내부에서 5개의 하위 작업을 생성하여 10배로 계산 후 합산
    auto total_sum_future = pool.submit(
        [&pool]() {
            // 하위 작업을 생성하는 람다 함수. (10배 계산)
            auto make_ten_times_task = [](int value) { return value * 10; };

            std::vector<std::future<int>> sub_results;
            for (int i = 1; i <= 5; ++i) { // 1부터 5까지의 값을 하위 작업으로 제출
                // 하위 작업(10배로 계산)을 제출하고, 그 결과를 future로 수집
                auto ten_times_future = pool.submit(make_ten_times_task, i);
                sub_results.push_back(std::move(ten_times_future));
            }

            // 모든 하위 작업이 완료될 때까지 기다리고, 결과를 합산
            int sub_total = 0;
            for (auto& f : sub_results) {
                sub_total += f.get(); // f.get() 호출 시 해당 하위 작업이 완료될 때까지 블로킹됨
            }
            return sub_total;
        }
    );

    auto total_sum = total_sum_future.get(); // 상위 작업이 완료될 때까지 기다리고, 최종 합산 결과를 가져옴. 
    std::cout << "sub-tasks total: " << total_sum << '\n';

    pool.stop();
    return 0;
}
