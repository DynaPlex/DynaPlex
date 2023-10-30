#include "dynaplex/parallel_execute.h"

namespace DynaPlex {
    namespace Parallel {
        std::vector<std::tuple<int64_t, int64_t>> get_splits(size_t total, size_t num_splits) {
            if (num_splits == 0)
                throw DynaPlex::Error("policyevaluator: get_splits. num_splits must not be zero.");
            if (total == 0)
                throw DynaPlex::Error("policyevaluator: get_splits. total must not be zero.");
            if(num_splits > total)
                throw DynaPlex::Error("policyevaluator: get_splits. num_splits should not exceed total.");



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

        std::vector<std::tuple<int64_t, int64_t>> get_chunks(size_t total, size_t max_chunk_size) {
            if (max_chunk_size == 0)
                throw DynaPlex::Error("policyevaluator: get_chunks. max_chunk_size must not be zero.");
            if (total == 0)
                throw DynaPlex::Error("policyevaluator: get_chunks. total must not be zero.");

            size_t base_num_chunks = (total + max_chunk_size - 1) / max_chunk_size;
            return get_splits(total, base_num_chunks);
        }
    }
}