#include <future>
#include <span>
#include <thread>
#include "dynaplex/error.h"
namespace DynaPlex {
    namespace Parallel {
        std::vector<std::tuple<int64_t, int64_t>> get_splits(size_t total, size_t num_splits) {
            if (num_splits == 0)
                throw DynaPlex::Error("policyevaluator: get_splits. num_splits must not be zero.");
            if (total == 0)
                throw DynaPlex::Error("policyevaluator: get_splits. total must not be zero.");

            size_t base_span_size = total / num_splits;
            size_t num_larger_spans = total % num_splits;
            std::vector<std::tuple<int64_t, int64_t>> sub_spans;
            size_t offset = 0;
            for (size_t i = 0; i < num_splits; ++i) {
                size_t current_span_size = base_span_size + (i < num_larger_spans);
                sub_spans.emplace_back(offset, offset + current_span_size);
                offset += current_span_size;
            }
            return sub_spans;
        }
        template <typename T>
        void parallel_compute(std::vector<T>& output_data,
            const std::function<void(std::span<T>, int64_t)>& work, 
            int64_t num_threads_to_use) {
            auto sub_spans_indices = get_splits(output_data.size(), num_threads_to_use);

            std::vector<std::future<void>> futures;
            std::vector<std::promise<void>> promises(num_threads_to_use);
            std::vector<std::jthread> threads;

            for (int64_t ThreadId = 0; ThreadId < num_threads_to_use; ThreadId++) {
                auto [start, end] = sub_spans_indices[ThreadId];
                futures.push_back(promises[ThreadId].get_future());

                threads.emplace_back(
                    [&output_data, &promises, &work, ThreadId, start, end]() {
                        try {
                            auto span = std::span<T>( &output_data[start], end - start );
                            work(span, start);
                            promises[ThreadId].set_value();
                        }
                        catch (...) {
                            promises[ThreadId].set_exception(std::current_exception());
                        }
                    }
                );
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

