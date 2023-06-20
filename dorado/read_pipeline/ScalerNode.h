#pragma once
#include "ReadPipeline.h"
#include "nn/CRFModel.h"
#include "utils/stats.h"

#include <torch/torch.h>

#include <atomic>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace dorado {

class ScalerNode : public MessageSink {
public:
    ScalerNode(const SignalNormalisationParams& config,
               int num_worker_threads = 5,
               size_t max_reads = 1000);
    ~ScalerNode() { terminate_impl(); }
    std::string get_name() const override { return "ScalerNode"; }
    stats::NamedStats sample_stats() const override;
    void terminate() override { terminate_impl(); }

private:
    void terminate_impl();
    void worker_thread();  // Worker thread performs scaling and trimming asynchronously.
    std::vector<std::unique_ptr<std::thread>> worker_threads;
    std::atomic<int> m_num_worker_threads;

    SignalNormalisationParams m_scaling_params;

    std::pair<float, float> normalisation(const torch::Tensor& x);
};

}  // namespace dorado
