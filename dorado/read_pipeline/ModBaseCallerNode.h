#pragma once
#include "ReadPipeline.h"

#include <memory>

class RemoraRunner;

class ModBaseCallerNode : public ReadSink {
public:
    ModBaseCallerNode(ReadSink& sink,
                      std::shared_ptr<RemoraRunner> model_runner,
                      size_t max_reads = 1000);
    ~ModBaseCallerNode();

private:
    void worker_thread();  // Worker thread performs calling asynchronously.

    ReadSink& m_sink;
    std::shared_ptr<RemoraRunner> m_model_runner;
    std::unique_ptr<std::thread> m_worker;
};