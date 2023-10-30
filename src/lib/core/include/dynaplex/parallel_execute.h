#pragma once
#include <future>
#include <span>
#include <thread>
#include <vector>
#include "dynaplex/error.h"
namespace DynaPlex {
    namespace Parallel {

        using ProgressReporter = std::function<void(const std::atomic<bool>&)>;

        std::vector<std::tuple<int64_t, int64_t>> get_splits(size_t total, size_t num_splits);

        std::vector<std::tuple<int64_t, int64_t>> get_chunks(size_t total, size_t max_chunk_size);


        template <typename T>
        void parallel_compute(std::vector<T>& output_data,
            const std::function<void(std::span<T>, int64_t)>& work, 
            int64_t num_threads_to_use,
            const ProgressReporter& reporter = nullptr) {

            if (num_threads_to_use > output_data.size())
                num_threads_to_use = output_data.size();
            
            auto sub_spans_indices = get_splits(output_data.size(), num_threads_to_use);

            std::atomic<bool> error_occurred = false;

            std::vector<std::future<void>> futures;
            std::vector<std::promise<void>> promises(num_threads_to_use);
            std::vector<std::jthread> threads;

            for (int64_t ThreadId = 0; ThreadId < num_threads_to_use; ThreadId++) {
                auto [start, end] = sub_spans_indices[ThreadId];
                futures.push_back(promises[ThreadId].get_future());

                threads.emplace_back(
                    [&output_data, &promises, &work, &error_occurred, ThreadId, start, end]() {
                        try {
                            auto span = std::span<T>( &output_data[start], end - start );
                            work(span, start);
                            promises[ThreadId].set_value();
                        }
                        catch (...) {              
                            promises[ThreadId].set_exception(std::current_exception());
                            error_occurred = true;
                        }
                    }
                );
            }
            if (reporter)
            {
                reporter(error_occurred);
            }
            // Wait for all futures to complete. If any thread threw an exception, it'll be rethrown here.
            for (auto& future : futures) {
                future.get();
            }
            // Destroying the threads joins them.
            threads.clear();
        }


    } // namespace Parallel
} // namespace DynaPlex

