#pragma once
#include "htslib/sam.h"
#include "read_pipeline/ReadPipeline.h"
#include "utils/stats.h"

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>

namespace dorado {

class BarcodeDemuxerNode : public MessageSink {
public:
    BarcodeDemuxerNode(const std::string& output_dir,
                       size_t htslib_threads,
                       size_t num_reads,
                       bool write_fastq);
    ~BarcodeDemuxerNode();
    std::string get_name() const override { return "BarcodeDemuxerNode"; }
    stats::NamedStats sample_stats() const override;
    void terminate(const FlushOptions& flush_options) override { terminate_impl(); }
    void restart() override;

    void set_header(const sam_hdr_t* header);

private:
    void terminate_impl();
    void start_threads();
    std::filesystem::path m_output_dir;
    int m_htslib_threads;
    sam_hdr_t* m_header{nullptr};
    std::atomic<int> m_processed_reads{0};

    std::unordered_map<std::string, htsFile*> m_files;
    std::unique_ptr<std::thread> m_worker;
    void worker_thread();
    int write(bam1_t* record);
    size_t m_num_reads_expected;
    bool m_write_fastq{false};
};

}  // namespace dorado