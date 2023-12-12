#pragma once

#include "CRFModel.h"
#include "ModelRunner.h"

#include <ATen/core/TensorBody.h>
#include <c10/cuda/CUDAStream.h>

#include <filesystem>
#include <memory>
#include <vector>

namespace dorado::basecall {

struct CRFModelConfig;
class CudaCaller;

std::shared_ptr<CudaCaller> create_cuda_caller(const CRFModelConfig& model_config,
                                               int chunk_size,
                                               int batch_size,
                                               const std::string& device,
                                               float memory_limit_fraction,
                                               bool exclusive_gpu_access);

class CudaModelRunner final : public ModelRunnerBase {
public:
    explicit CudaModelRunner(std::shared_ptr<CudaCaller> caller);
    void accept_chunk(int chunk_idx, const at::Tensor& chunk) final;
    std::vector<DecodedChunk> call_chunks(int num_chunks) final;
    const CRFModelConfig& config() const final;
    size_t model_stride() const final;
    size_t chunk_size() const final;
    size_t batch_size() const final;
    void terminate() final;
    void restart() final;
    std::string get_name() const final;
    stats::NamedStats sample_stats() const final;

private:
    std::shared_ptr<CudaCaller> m_caller;
    c10::cuda::CUDAStream m_stream;
    at::Tensor m_input;
    at::Tensor m_output;

    // Performance monitoring stats.
    std::atomic<int64_t> m_num_batches_called = 0;
};

}  // namespace dorado::basecall
