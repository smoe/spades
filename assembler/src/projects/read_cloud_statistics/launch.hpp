#pragma once

#include "common/pipeline/stage.hpp"
#include "projects/spades/repeat_resolving.hpp"
#include "read_cloud_statistics_extractor.hpp"

namespace spades {
    void run_statistics_extractor() {
        INFO("Read cloud statistics extractor started");
        debruijn_graph::conj_graph_pack conj_gp(cfg::get().K,
                                                cfg::get().tmp_dir,
                                                cfg::get().ds.reads.lib_count(),
                                                cfg::get().ds.reference_genome,
                                                cfg::get().flanking_range,
                                                cfg::get().pos.max_mapping_gap,
                                                cfg::get().pos.max_gap_diff);
        StageManager statistics_extractor({cfg::get().developer_mode,
                              cfg::get().load_from,
                              cfg::get().output_saves});
        statistics_extractor.add(new debruijn_graph::RepeatResolution())
                .add(new debruijn_graph::ReadCloudStatisticsStage());
        INFO("Output directory: " << cfg::get().output_dir);
        statistics_extractor.run(conj_gp, cfg::get().entry_point.c_str());
        INFO("Read cloud statistics extractor finished.");
    }
} //spades