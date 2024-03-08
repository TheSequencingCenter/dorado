#include "AlignerNode.h"

#include "ClientInfo.h"
#include "alignment/Minimap2Aligner.h"
#include "alignment/Minimap2Index.h"

#include <htslib/sam.h>
#include <minimap.h>
#include <spdlog/spdlog.h>

#include <cassert>
#include <filesystem>
#include <string>
#include <vector>

namespace {

std::shared_ptr<const dorado::alignment::Minimap2Index> load_and_get_index(
        dorado::alignment::IndexFileAccess& index_file_access,
        const std::string& filename,
        const dorado::alignment::Minimap2Options& options,
        const int threads) {
    int num_index_construction_threads{options.print_aln_seq ? 1 : static_cast<int>(threads)};
    switch (index_file_access.load_index(filename, options, num_index_construction_threads)) {
    case dorado::alignment::IndexLoadResult::reference_file_not_found:
        throw std::runtime_error("AlignerNode reference path does not exist: " + filename);
    case dorado::alignment::IndexLoadResult::validation_error:
        throw std::runtime_error("AlignerNode validation error checking minimap options");
    case dorado::alignment::IndexLoadResult::split_index_not_supported:
        throw std::runtime_error(
                "Dorado doesn't support split index for alignment. Please re-run with larger index "
                "size.");
    case dorado::alignment::IndexLoadResult::success:
        break;
    }
    return index_file_access.get_index(filename, options);
}

}  // namespace

namespace dorado {

AlignerNode::AlignerNode(std::shared_ptr<alignment::IndexFileAccess> index_file_access,
                         const std::string& index_file,
                         const std::string& bed_file,
                         const alignment::Minimap2Options& options,
                         int threads)
        : MessageSink(10000, threads),
          m_index_for_bam_messages(
                  load_and_get_index(*index_file_access, index_file, options, threads)),
          m_index_file_access(std::move(index_file_access)) {
    auto header_sequence_records = m_index_for_bam_messages->get_sequence_records_for_header();
    for (const auto& entry : header_sequence_records) {
        m_header_sequences_for_bam_messages.emplace_back(entry.first);
    }
    if (!bed_file.empty()) {
        m_bed_file_for_bam_messages.load(bed_file);
    }
    start_input_processing(&AlignerNode::input_thread_fn, this);
}

AlignerNode::AlignerNode(std::shared_ptr<alignment::IndexFileAccess> index_file_access, int threads)
        : MessageSink(10000, threads), m_index_file_access(std::move(index_file_access)) {
    start_input_processing(&AlignerNode::input_thread_fn, this);
}

std::shared_ptr<const alignment::Minimap2Index> AlignerNode::get_index(
        const ReadCommon& read_common) {
    auto& align_info = read_common.client_info->alignment_info();
    if (align_info.reference_file.empty()) {
        return {};
    }
    auto index =
            m_index_file_access->get_index(align_info.reference_file, align_info.minimap_options);
    if (!index) {
        if (read_common.client_info->is_disconnected()) {
            // Unlikely but ... may have disconnected since last checked and caused a
            // an unload of the index file.
            return {};
        }
        throw std::runtime_error(
                "Cannot align read. Expected alignment reference file is not loaded: " +
                align_info.reference_file);
    }

    return index;
}

alignment::HeaderSequenceRecords AlignerNode::get_sequence_records_for_header() const {
    assert(m_index_for_bam_messages != nullptr &&
           "get_sequence_records_for_header only valid if AlignerNode constructed with index file");
    return alignment::Minimap2Aligner(m_index_for_bam_messages).get_sequence_records_for_header();
}

void AlignerNode::align_read_common(ReadCommon& read_common, mm_tbuf_t* tbuf) {
    if (read_common.client_info->is_disconnected()) {
        return;
    }

    auto index = get_index(read_common);
    if (!index) {
        return;
    }

    alignment::Minimap2Aligner(index).align(read_common, tbuf);
}

void AlignerNode::input_thread_fn() {
    Message message;
    mm_tbuf_t* tbuf = mm_tbuf_init();
    auto align_read = [this, tbuf](auto&& read) {
        align_read_common(read->read_common, tbuf);
        send_message_to_sink(std::move(read));
    };
    while (get_input_message(message)) {
        if (std::holds_alternative<BamPtr>(message)) {
            auto read = std::get<BamPtr>(std::move(message));
            auto records =
                    alignment::Minimap2Aligner(m_index_for_bam_messages).align(read.get(), tbuf);
            for (auto& record : records) {
                if (!m_bed_file_for_bam_messages.filename().empty() &&
                    !(record->core.flag & BAM_FUNMAP)) {
                    auto ref_id = record->core.tid;
                    add_bed_hits_to_record(m_header_sequences_for_bam_messages.at(ref_id), record);
                }
                send_message_to_sink(std::move(record));
            }
        } else if (std::holds_alternative<SimplexReadPtr>(message)) {
            align_read(std::get<SimplexReadPtr>(std::move(message)));
        } else if (std::holds_alternative<DuplexReadPtr>(message)) {
            align_read(std::get<DuplexReadPtr>(std::move(message)));
        } else {
            send_message_to_sink(std::move(message));
            continue;
        }
    }
    mm_tbuf_destroy(tbuf);
}

stats::NamedStats AlignerNode::sample_stats() const { return stats::from_obj(m_work_queue); }

void AlignerNode::add_bed_hits_to_record(const std::string& genome, dorado::BamPtr& record) {
    size_t genome_start = record->core.pos;
    size_t genome_end = bam_endpos(record.get());
    char direction = (bam_is_rev(record.get())) ? '-' : '+';
    int bed_hits = 0;
    for (const auto& entry : m_bed_file_for_bam_messages.entries(genome)) {
        if (!(entry.start > genome_end || entry.end < genome_start) &&
            (entry.strand == direction || entry.strand == '.')) {
            bed_hits++;
        }
    }
    // update the record.
    bam_aux_append(record.get(), "bh", 'i', sizeof(bed_hits), (uint8_t*)&bed_hits);
}

}  // namespace dorado
