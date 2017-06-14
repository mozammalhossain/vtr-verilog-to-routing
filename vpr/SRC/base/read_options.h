#ifndef READ_OPTIONS_H
#define READ_OPTIONS_H

#include "vpr_types.h"
#include "argparse_value.hpp"

struct t_options {
	/* File names */
	argparse::ArgValue<char*> ArchFile;
	argparse::ArgValue<char*> CircuitName;
	argparse::ArgValue<char*> NetFile;
	argparse::ArgValue<char*> PlaceFile;
	argparse::ArgValue<char*> RouteFile;
	argparse::ArgValue<char*> BlifFile;
	argparse::ArgValue<char*> ActFile;
	argparse::ArgValue<char*> PowerFile;
	argparse::ArgValue<char*> CmosTechFile;
	argparse::ArgValue<char*> out_file_prefix;
	argparse::ArgValue<char*> SDCFile;
	argparse::ArgValue<char*> pad_loc_file;
	argparse::ArgValue<char*> write_rr_graph_file;
    argparse::ArgValue<char*> read_rr_graph_file;
    argparse::ArgValue<char*> dump_rr_structs_file;

    /* Stage Options */
    argparse::ArgValue<bool> do_packing;
    argparse::ArgValue<bool> do_placement;
    argparse::ArgValue<bool> do_routing;
    argparse::ArgValue<bool> do_analysis;
    argparse::ArgValue<bool> do_power;

    /* Graphics Options */
    argparse::ArgValue<bool> show_graphics; //Enable argparse::ArgValue<int>eractive graphics?
	argparse::ArgValue<int> GraphPause;
	/* General options */
    argparse::ArgValue<bool> show_help;
    argparse::ArgValue<bool> show_version;
	argparse::ArgValue<bool> timing_analysis;
    argparse::ArgValue<const char*> SlackDefinition;
	argparse::ArgValue<bool> CreateEchoFile;
    argparse::ArgValue<bool> verify_file_digests;

    /* Atom netlist options */
	argparse::ArgValue<bool> absorb_buffer_luts;
	argparse::ArgValue<bool> sweep_dangling_primary_ios;
	argparse::ArgValue<bool> sweep_dangling_nets;
	argparse::ArgValue<bool> sweep_dangling_blocks;
	argparse::ArgValue<bool> sweep_constant_primary_outputs;

	/* Clustering options */
	//argparse::ArgValue<bool> global_clocks;
	//argparse::ArgValue<int> cluster_size;
	//argparse::ArgValue<int> inputs_per_cluster;
	//argparse::ArgValue<int> lut_size;
	//argparse::ArgValue<bool> hill_climbing_flag;
	//argparse::ArgValue<bool> timing_driven;
	argparse::ArgValue<bool> connection_driven_clustering;
	argparse::ArgValue<bool> allow_unrelated_clustering;
	argparse::ArgValue<float> alpha_clustering;
	argparse::ArgValue<float> beta_clustering;
    argparse::ArgValue<bool> timing_driven_clustering;
    argparse::ArgValue<e_cluster_seed> cluster_seed_type;
	//argparse::ArgValue<int> recompute_timing_after;
	//argparse::ArgValue<float> block_delay;
	//argparse::ArgValue<float> argparse::ArgValue<int>er_cluster_net_delay;
	//argparse::ArgValue<bool> skip_clustering;
	//e_packer_algorithm packer_algorithm;

	/* Placement options */
	argparse::ArgValue<int> Seed;
    argparse::ArgValue<bool> ShowPlaceTiming;
	argparse::ArgValue<float> PlaceInnerNum;
	argparse::ArgValue<float> PlaceInitT;
	argparse::ArgValue<float> PlaceExitT;
	argparse::ArgValue<float> PlaceAlphaT;
    argparse::ArgValue<sched_type> anneal_sched_type; 
	argparse::ArgValue<e_place_algorithm> PlaceAlgorithm;
    argparse::ArgValue<e_pad_loc_type> pad_loc_type;
	argparse::ArgValue<int> PlaceChanWidth;
	//argparse::ArgValue<float> place_cost_exp;

	/* Timing-driven placement options only */
	argparse::ArgValue<float> PlaceTimingTradeoff;
	argparse::ArgValue<int> RecomputeCritIter;
	argparse::ArgValue<int> inner_loop_recompute_divider;
	argparse::ArgValue<float> place_exp_first;
	argparse::ArgValue<float> place_exp_last;

	/* Router Options */
	argparse::ArgValue<int> max_router_iterations;
	argparse::ArgValue<float> first_iter_pres_fac;
	argparse::ArgValue<float> initial_pres_fac;
	argparse::ArgValue<float> pres_fac_mult;
	argparse::ArgValue<float> acc_fac;
	argparse::ArgValue<int> bb_factor;
	argparse::ArgValue<e_base_cost_type> base_cost_type;
	argparse::ArgValue<float> bend_cost;
	argparse::ArgValue<e_route_type> RouteType;
	argparse::ArgValue<int> RouteChanWidth;
	argparse::ArgValue<int> min_route_chan_width_hint; //Hint to binary search router about what the min chan width is
    argparse::ArgValue<bool> verify_binary_search;
	argparse::ArgValue<e_router_algorithm> RouterAlgorithm;
	argparse::ArgValue<int> min_incremental_reroute_fanout;

	//argparse::ArgValue<bool> congestion_analysis;
	//argparse::ArgValue<bool> fanout_analysis;
    //argparse::ArgValue<bool> switch_usage_analysis;
	//argparse::ArgValue<bool> TrimEmptyChan;
	//argparse::ArgValue<bool> TrimObsChan;

	/* Timing-driven router options only */
	argparse::ArgValue<float> astar_fac;
	argparse::ArgValue<float> max_criticality;
	argparse::ArgValue<float> criticality_exp;
	argparse::ArgValue<e_routing_failure_predictor> routing_failure_predictor;

	//argparse::ArgValue<float> constant_net_delay;
    argparse::ArgValue<bool> full_stats;
	argparse::ArgValue<bool> Generate_Post_Synthesis_Netlist;

};

t_options read_options(int argc, const char** argv);

#endif
