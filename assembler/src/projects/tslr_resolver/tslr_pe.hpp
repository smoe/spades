#pragma once

#include "tslr_extension_chooser.hpp"
#include "tslr_visualizer.hpp"
#include "bounded_dijkstra.hpp"
#include "algorithms/dijkstra/dijkstra_helper.hpp"
#include "extenders.hpp"

using namespace path_extend;

namespace tslr_resolver {

    PathContainer FilterByLength(const PathContainer& seeds, size_t len_threshold) {
        PathContainer result;
        for (auto it = seeds.begin(); it != seeds.end(); ++it) {
            if (it->first->Length() > len_threshold) {
                result.AddPair(it->first, it->second);
            }
        }
        return result;
    }

    void LaunchBarcodePE (conj_graph_pack &gp) {
        path_extend::PathExtendParamsContainer params(cfg::get().pe_params,
                                                      cfg::get().output_dir,
                                                      "final_contigs_tslr",
                                                      "scaffolds_tslr",
                                                      cfg::get().mode,
                                                      cfg::get().uneven_depth,
                                                      cfg::get().avoid_rc_connections,
                                                      cfg::get().use_scaffolder);

        DefaultContigCorrector<ConjugateDeBruijnGraph> corrector(gp.g);
        DefaultContigConstructor<ConjugateDeBruijnGraph> constructor(gp.g, corrector);
        ContigWriter writer(gp.g, constructor, gp.components, params.mode == config::pipeline_type::plasmid);
        GraphCoverageMap cover_map(gp.g);
        const pe_config::ParamSetT &pset = params.pset;
        bool detect_repeats_online = false;

        PathExtendResolver resolver(gp.g);
        auto min_unique_length = pset.scaffolding2015.min_unique_length;
        auto unique_variaton = pset.scaffolding2015.unique_coverage_variation;
        ScaffoldingUniqueEdgeStorage main_unique_storage;
        auto dataset_info = cfg::get().ds;
        main_unique_storage = FillUniqueEdgeStorage(gp, dataset_info,
                                                    min_unique_length,
                                                    unique_variaton,
                                                    false);

        //mp extender
        INFO("SUBSTAGE = paired-end libraries")
        PathExtendStage exspander_stage = PathExtendStage::PEStage;
        vector<shared_ptr<PathExtender> > all_libs =
            MakeAllExtenders(exspander_stage, dataset_info, params, gp, cover_map, main_unique_storage);
        size_t max_is_right_quantile = max(FindOverlapLenForStage(exspander_stage, cfg::get().ds), gp.g.k() + 100);
        size_t min_edge_len = 100;

        shared_ptr<CompositeExtender> mainPE = make_shared<CompositeExtender>(gp.g, cover_map, all_libs,
                                                                              main_unique_storage,
                                                                              max_is_right_quantile,
                                                                              pset.extension_options.max_repeat_length,
                                                                              detect_repeats_online);
        auto seeds = resolver.makeSimpleSeeds();
        seeds.SortByLength();
        auto paths = resolver.extendSeeds(seeds, *mainPE);
        paths.SortByLength();
        FinalizePaths(params, paths, cover_map, min_edge_len, max_is_right_quantile);
        INFO("Coverage map size " << cover_map.size())
        seeds.DeleteAllPaths();
        all_libs.clear();


        exspander_stage = PathExtendStage::PEPolishing;
        all_libs = MakeAllExtenders(exspander_stage, dataset_info, params, gp, cover_map, main_unique_storage);
        mainPE = make_shared<CompositeExtender>(gp.g, cover_map, all_libs,
                                                main_unique_storage,
                                                max_is_right_quantile,
                                                pset.extension_options.max_repeat_length,
                                                detect_repeats_online);

        FinalizePaths(params, paths, cover_map, min_edge_len, max_is_right_quantile);
        DebugOutputPaths(gp, params, paths, "pe_before_traverse");
        if (params.traverse_loops) {
            TraverseLoops(paths, cover_map, mainPE);
            FinalizePaths(params, paths, cover_map, min_edge_len, max_is_right_quantile);
        }
        bool mp_exist = MPLibsExist(dataset_info);
        DebugOutputPaths(gp, params, paths, (mp_exist ? "pe_final_paths" : "final_paths"));
        writer.OutputPaths(paths, params.output_dir + (mp_exist ? "pe_scaffolds" : params.contigs_name));

        //tslr extender
        INFO("SUBSTAGE = TSLR Resolver")
        auto tslr_resolver_params = cfg::get().ts_res;
        size_t len_threshold = tslr_resolver_params.len_threshold;
        double relative_diff_threshold = tslr_resolver_params.diff_threshold;
        size_t distance_bound = tslr_resolver_params.distance_bound;
        double abs_threshold = tslr_resolver_params.abs_threshold;
        bool join_paths = tslr_resolver_params.join_paths;

        max_is_right_quantile = gp.g.k() + 10000;
        auto extension = make_shared<TrivialTSLRExtensionChooser>(gp,
                                                                  len_threshold,
                                                                  relative_diff_threshold,
                                                                  main_unique_storage);
        auto tslr_extender = make_shared<InconsistentTSLRExtender>(gp, cover_map,
                                                             extension,
                                                             2500 /*insert size*/,
                                                             0 /*max loops*/,
                                                             false, /*investigate short loops*/
                                                             false /*use short loop coverage resolver*/,
                                                             distance_bound,
                                                             abs_threshold,
                                                             len_threshold,
                                                             join_paths,
                                                             main_unique_storage,
                                                             gp.barcode_mapper -> AverageBarcodeCoverage());
        all_libs.push_back(tslr_extender);
        shared_ptr<PathJoiner> tslrPE = make_shared<PathJoiner>
                (gp.g, cover_map, all_libs,
                 main_unique_storage,
                 max_is_right_quantile,
                 pset.extension_options.max_repeat_length,
                 detect_repeats_online);


        INFO(paths.size() << " paths from previous stage")
        auto filtered_paths = FilterByLength(paths, len_threshold);
        INFO(filtered_paths.size() << " filtered paths")

        auto final_paths = resolver.extendSeeds(filtered_paths, *tslrPE);

        FinalizePaths(params, final_paths, cover_map, min_edge_len, max_is_right_quantile);

        debruijn_graph::GenomeConsistenceChecker genome_checker (gp, main_unique_storage, 1000, 0.2);
        DebugOutputPaths(gp, params, final_paths, "final_tslr_paths");

        writer.OutputPaths(final_paths, params.output_dir + params.contigs_name);
        if (gp.genome.size() > 0)
            CountMisassembliesWithReference(genome_checker, final_paths);
    };

} //tslr_resolver
