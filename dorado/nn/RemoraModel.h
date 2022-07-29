#pragma once

#include <torch/torch.h>

#include <array>
#include <filesystem>
#include <string>
#include <vector>

namespace utils {
struct BaseModInfo;
}

torch::nn::ModuleHolder<torch::nn::AnyModule> load_remora_model(
        const std::filesystem::path& model_path,
        torch::TensorOptions options);

struct BaseModParams {
    std::vector<std::string> mod_long_names;  ///< The long names of the modified bases.
    std::string motif;                        ///< The motif to look for modified bases within.
    size_t base_mod_count;                    ///< The number of modifications for the base.
    size_t motif_offset;  ///< The position of the canonical base within the motif.
    size_t context_before;  ///< The number of context samples in the signal the network looks at around a candidate base.
    size_t context_after;  ///< The number of context samples in the signal the network looks at around a candidate base.
    int bases_before;  ///< The number of bases before the primary base of a kmer.
    int bases_after;   ///< The number of bases after the primary base of a kmer.
    int offset;
    std::string mod_bases;
    std::vector<float> refine_kmer_levels;  ///< Expected kmer levels for rough rescaling
    size_t refine_kmer_len;                 ///< Length of the kmers for the specified kmer_levels
    size_t refine_kmer_center_idx;  ///< The position in the kmer at which to check the levels
    bool refine_do_rough_rescale;   ///< Whether to perform rough rescaling
};

class RemoraCaller {
    constexpr static torch::ScalarType dtype = torch::kFloat32;
    torch::nn::ModuleHolder<torch::nn::AnyModule> m_module{nullptr};
    torch::TensorOptions m_options;
    torch::Tensor m_input_sigs;
    torch::Tensor m_input_seqs;

    BaseModParams m_params;
    const int m_batch_size;

    std::vector<size_t> get_motif_hits(const std::string& seq) const;

public:
    RemoraCaller(const std::filesystem::path& model_path,
                 const std::string& device,
                 int batch_size);
    std::pair<torch::Tensor, std::vector<size_t>> call(torch::Tensor signal,
                                                       const std::string& seq,
                                                       const std::vector<uint8_t>& moves,
                                                       size_t block_stride);
    const BaseModParams& params() const { return m_params; }
};

class RemoraRunner {
    using BaseModInfo = ::utils::BaseModInfo;

    // one caller per model
    std::vector<std::shared_ptr<RemoraCaller>> m_callers;
    // The offsets to the canonical bases in the modbase alphabet
    std::array<size_t, 4> m_base_prob_offsets;
    size_t m_num_states;

    std::shared_ptr<BaseModInfo> m_base_mod_info;

public:
    RemoraRunner(const std::vector<std::filesystem::path>& model_paths,
                 const std::string& device,
                 int batch_size = 1000);
    torch::Tensor run(torch::Tensor signal,
                      const std::string& seq,
                      const std::vector<uint8_t>& moves,
                      size_t block_stride);

    std::shared_ptr<const BaseModInfo> base_mod_info() const {
        return std::const_pointer_cast<const BaseModInfo>(m_base_mod_info);
    }
};
