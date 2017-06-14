#include <cstring>
#include <vector>
using namespace std;

#include "vtr_assert.h"
#include "vtr_util.h"
#include "vtr_random.h"
#include "vtr_log.h"
#include "vtr_memory.h"

#include "vpr_types.h"
#include "vpr_error.h"

#include "globals.h"
#include "read_xml_arch_file.h"
#include "SetupVPR.h"
#include "pb_type_graph.h"
#include "pack_types.h"
#include "lb_type_rr_graph.h"
#include "rr_graph_area.h"
#include "echo_arch.h"
#include "read_options.h"
#include "echo_files.h"

static void SetupNetlistOpts(const t_options& Options, t_netlist_opts& NetlistOpts);
static void SetupPackerOpts(const t_options& Options,
		const t_arch& Arch, const char *net_file,
		t_packer_opts *PackerOpts);
static void SetupPlacerOpts(const t_options& Options,
		t_placer_opts *PlacerOpts);
static void SetupAnnealSched(const t_options& Options,
		t_annealing_sched *AnnealSched);
static void SetupRouterOpts(const t_options& Options, t_router_opts *RouterOpts);
static void SetupRoutingArch(const t_arch& Arch, t_det_routing_arch *RoutingArch);
static void SetupTiming(const t_options& Options, const t_arch& Arch,
		const bool TimingEnabled,
		t_timing_inf * Timing);
static void SetupSwitches(const t_arch& Arch,
		t_det_routing_arch *RoutingArch,
		const t_arch_switch_inf *ArchSwitches, int NumArchSwitches);
static void SetupAnalysisOpts(const t_options& Options, t_analysis_opts& analysis_opts);
static void SetupPowerOpts(const t_options& Options, t_power_opts *power_opts,
		t_arch * Arch);

/* Sets VPR parameters and defaults. Does not do any error checking
 * as this should have been done by the various input checkers */
void SetupVPR(t_options *Options, 
              const bool TimingEnabled,
              const bool readArchFile, 
              t_file_name_opts *FileNameOpts,
              t_arch * Arch,
              t_model ** user_models, 
              t_model ** library_models,
              t_netlist_opts* NetlistOpts,
              t_packer_opts *PackerOpts,
              t_placer_opts *PlacerOpts,
              t_annealing_sched *AnnealSched,
              t_router_opts *RouterOpts,
              t_analysis_opts* AnalysisOpts,
              t_det_routing_arch *RoutingArch,
              vector <t_lb_type_rr_node> **PackerRRGraphs,
              t_segment_inf ** Segments, t_timing_inf * Timing,
              bool * ShowGraphics, int *GraphPause,
              t_power_opts * PowerOpts) {
	int i, j, len;
    using argparse::Provenance;

    auto& device_ctx = g_vpr_ctx.mutable_device();

	if (!Options->CircuitName) {
        vpr_throw(VPR_ERROR_BLIF_F,__FILE__, __LINE__, 
                  "No blif file found in arguments (did you specify an architecture file?)\n");
    }

    std::string cct_base_name = vtr::basename(Options->CircuitName.value());
    std::string default_output_name = cct_base_name;

	/* init default filenames */
	if (Options->BlifFile == NULL ) {
		len = strlen(Options->CircuitName) + 6; /* circuit_name.blif/0*/
		if (Options->out_file_prefix != NULL ) {
			len += strlen(Options->out_file_prefix);
		}
		Options->BlifFile.set((char*) vtr::calloc(len, sizeof(char)), Provenance::INFERRED);
		if (Options->out_file_prefix == NULL ) {
			sprintf(Options->BlifFile.value(), "%s.blif", Options->CircuitName.value());
		} else {
			sprintf(Options->BlifFile.value(), "%s%s.blif", Options->out_file_prefix.value(),
					Options->CircuitName.value());
		}
	}

	if (Options->NetFile == NULL ) {
		len = strlen(default_output_name.c_str()) + 5; /* circuit_name.net/0*/
		if (Options->out_file_prefix != NULL ) {
			len += strlen(Options->out_file_prefix);
		}
		Options->NetFile.set((char*) vtr::calloc(len, sizeof(char)), Provenance::INFERRED);
		if (Options->out_file_prefix == NULL ) {
			sprintf(Options->NetFile.value(), "%s.net", default_output_name.c_str());
		} else {
			sprintf(Options->NetFile.value(), "%s%s.net", Options->out_file_prefix.value(), default_output_name.c_str());
		}
	}

	if (Options->PlaceFile == NULL ) {
		len = strlen(default_output_name.c_str()) + 7; /* circuit_name.place/0*/
		if (Options->out_file_prefix != NULL ) {
			len += strlen(Options->out_file_prefix);
		}
		Options->PlaceFile.set((char*) vtr::calloc(len, sizeof(char)), Provenance::INFERRED);
		if (Options->out_file_prefix == NULL ) {
			sprintf(Options->PlaceFile.value(), "%s.place", default_output_name.c_str());
		} else {
			sprintf(Options->PlaceFile.value(), "%s%s.place", Options->out_file_prefix.value(), default_output_name.c_str());
		}
	}

	if (Options->RouteFile == NULL ) {
		len = strlen(Options->CircuitName) + 7; /* circuit_name.route/0*/
		if (Options->out_file_prefix != NULL ) {
			len += strlen(Options->out_file_prefix);
		}
		Options->RouteFile.set((char*) vtr::calloc(len, sizeof(char)), Provenance::INFERRED);
		if (Options->out_file_prefix == NULL ) {
			sprintf(Options->RouteFile.value(), "%s.route", default_output_name.c_str());
		} else {
			sprintf(Options->RouteFile.value(), "%s%s.route", Options->out_file_prefix.value(), default_output_name.c_str());
		}
	}
	if (Options->ActFile == NULL ) {
		len = strlen(default_output_name.c_str()) + 7; /* circuit_name.route/0*/
		if (Options->out_file_prefix != NULL ) {
			len += strlen(Options->out_file_prefix);
		}
		Options->ActFile.set((char*) vtr::calloc(len, sizeof(char)), Provenance::INFERRED);
		if (Options->out_file_prefix == NULL ) {
			sprintf(Options->ActFile.value(), "%s.act", default_output_name.c_str());
		} else {
			sprintf(Options->ActFile.value(), "%s%s.act", Options->out_file_prefix.value(), default_output_name.c_str());
		}
	}

	if (Options->PowerFile == NULL ) {
		len = strlen(default_output_name.c_str()) + 7; /* circuit_name.route/0*/
		if (Options->out_file_prefix != NULL ) {
			len += strlen(Options->out_file_prefix);
		}
		Options->PowerFile.set((char*) vtr::calloc(len, sizeof(char)), Provenance::INFERRED);
		if (Options->out_file_prefix == NULL ) {
			sprintf(Options->PowerFile.value(), "%s.power", default_output_name.c_str());
		} else {
			sprintf(Options->ActFile.value(), "%s%s.power", Options->out_file_prefix.value(), default_output_name.c_str());
		}
	}

	alloc_and_load_output_file_names(default_output_name.c_str());

    //TODO: Move FileNameOpts setup into separate function
	FileNameOpts->CircuitName = Options->CircuitName;
	FileNameOpts->ArchFile = Options->ArchFile;
	FileNameOpts->BlifFile = Options->BlifFile;
	FileNameOpts->NetFile = Options->NetFile;
	FileNameOpts->PlaceFile = Options->PlaceFile;
	FileNameOpts->RouteFile = Options->RouteFile;
	FileNameOpts->ActFile = Options->ActFile;
	FileNameOpts->PowerFile = Options->PowerFile;
	FileNameOpts->CmosTechFile = Options->CmosTechFile;
	FileNameOpts->out_file_prefix = Options->out_file_prefix;

    FileNameOpts->verify_file_digests = Options->verify_file_digests;

    SetupNetlistOpts(*Options, *NetlistOpts);
	SetupPlacerOpts(*Options, PlacerOpts);
	SetupAnnealSched(*Options, AnnealSched);
	SetupRouterOpts(*Options, RouterOpts);
	SetupAnalysisOpts(*Options, *AnalysisOpts);
	SetupPowerOpts(*Options, PowerOpts, Arch);

	if (readArchFile == true) {
		XmlReadArch(Options->ArchFile, TimingEnabled, Arch, &device_ctx.block_types,
				&device_ctx.num_block_types);
	}

	*user_models = Arch->models;
	*library_models = Arch->model_library;

	/* TODO: this is inelegant, I should be populating this information in XmlReadArch */
	device_ctx.EMPTY_TYPE = NULL;
	device_ctx.FILL_TYPE = NULL;
	device_ctx.IO_TYPE = NULL;
	for (i = 0; i < device_ctx.num_block_types; i++) {
		if (strcmp(device_ctx.block_types[i].name, "<EMPTY>") == 0) {
			device_ctx.EMPTY_TYPE = &device_ctx.block_types[i];
		} else if (strcmp(device_ctx.block_types[i].name, "io") == 0) {
			device_ctx.IO_TYPE = &device_ctx.block_types[i];
		} else {
			for (j = 0; j < device_ctx.block_types[i].num_grid_loc_def; j++) {
				if (device_ctx.block_types[i].grid_loc_def[j].grid_loc_type == FILL) {
					VTR_ASSERT(device_ctx.FILL_TYPE == NULL);
					device_ctx.FILL_TYPE = &device_ctx.block_types[i];
				}
			}
		}
	}
	VTR_ASSERT(device_ctx.EMPTY_TYPE != NULL && device_ctx.FILL_TYPE != NULL && device_ctx.IO_TYPE != NULL);

	*Segments = Arch->Segments;
	RoutingArch->num_segment = Arch->num_segments;

	SetupSwitches(*Arch, RoutingArch, Arch->Switches, Arch->num_switches);
	SetupRoutingArch(*Arch, RoutingArch);
	SetupTiming(*Options, *Arch, TimingEnabled, Timing);
	SetupPackerOpts(*Options, *Arch, Options->NetFile, PackerOpts);
	RoutingArch->dump_rr_structs_file = nullptr;

    //Setup the default flow
    if (   !Options->do_packing
        && !Options->do_placement
        && !Options->do_routing
        && !Options->do_analysis) {

        //run all stages if none specified
        PackerOpts->doPacking = true;
        PlacerOpts->doPlacement = true;
        RouterOpts->doRouting = true;
        AnalysisOpts->doAnalysis = true;
    }

    //By default run analysis after routing
    if(RouterOpts->doRouting) {
        AnalysisOpts->doAnalysis = true;
    }

	/* init global variables */
    vtr::out_file_prefix = Options->out_file_prefix;

	/* Set seed for pseudo-random placement, default seed to 1 */
	PlacerOpts->seed =  Options->Seed;
	vtr::srandom(PlacerOpts->seed);

	vtr::printf_info("Building complex block graph.\n");
	alloc_and_load_all_pb_graphs(PowerOpts->do_power);
	*PackerRRGraphs = alloc_and_load_all_lb_type_rr_graph();
	if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_LB_TYPE_RR_GRAPH)) {
		echo_lb_type_rr_graphs(getEchoFileName(E_ECHO_LB_TYPE_RR_GRAPH),*PackerRRGraphs);
	}

	if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_PB_GRAPH)) {
		echo_pb_graph(getEchoFileName(E_ECHO_PB_GRAPH));
	}

	*GraphPause = Options->GraphPause;

	*ShowGraphics = Options->show_graphics;

	if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_ARCH)) {
		EchoArch(getEchoFileName(E_ECHO_ARCH), device_ctx.block_types, device_ctx.num_block_types,
				Arch);
	}

}

static void SetupTiming(const t_options& Options, const t_arch& Arch,
		const bool TimingEnabled,
		t_timing_inf * Timing) {

	/* Don't do anything if they don't want timing */
	if (false == TimingEnabled) {
		memset(Timing, 0, sizeof(t_timing_inf));
		Timing->timing_analysis_enabled = false;
		return;
	}

	Timing->C_ipin_cblock = Arch.C_ipin_cblock;
	Timing->T_ipin_cblock = Arch.T_ipin_cblock;
	Timing->timing_analysis_enabled = TimingEnabled;

	/* If the user specified an SDC filename on the command line, look for specified_name.sdc, otherwise look for circuit_name.sdc*/
	if (Options.SDCFile == NULL ) {
		Timing->SDCFile = (char*) vtr::calloc(strlen(Options.CircuitName) + 5,
				sizeof(char)); /* circuit_name.sdc/0*/
		sprintf(Timing->SDCFile, "%s.sdc", Options.CircuitName.value());
	} else {
		Timing->SDCFile = (char*) vtr::strdup(Options.SDCFile);
	}

    if (Options.SlackDefinition != '\0') {
        Timing->slack_definition = Options.SlackDefinition;
        VTR_ASSERT(Timing->slack_definition == std::string("R") || Timing->slack_definition == std::string("I") ||
               Timing->slack_definition == std::string("S") || Timing->slack_definition == std::string("G") ||
               Timing->slack_definition == std::string("C") || Timing->slack_definition == std::string("N"));
    } else {
        Timing->slack_definition = "R"; // default
    }
}

/* This loads up VPR's arch_switch_inf data by combining the switches from 
 * the arch file with the special switches that VPR needs. */
static void SetupSwitches(const t_arch& Arch,
		t_det_routing_arch *RoutingArch,
		const t_arch_switch_inf *ArchSwitches, int NumArchSwitches) {

    auto& device_ctx = g_vpr_ctx.mutable_device();

	int switches_to_copy = NumArchSwitches;
	device_ctx.num_arch_switches = NumArchSwitches;

	/* If ipin cblock info has not been read in from a switch, then we will
	   create a new switch for it. Otherwise, the switch already exists */
	if (NULL == Arch.ipin_cblock_switch_name){
		/* Depends on device_ctx.num_arch_switches */
		RoutingArch->wire_to_arch_ipin_switch = device_ctx.num_arch_switches;
		++device_ctx.num_arch_switches;
		++NumArchSwitches;
	} else {
		/* need to find the index of the input cblock switch */
		int ipin_cblock_switch_index = -1;
		char *ipin_cblock_switch_name = Arch.ipin_cblock_switch_name;
		for (int iswitch = 0; iswitch < device_ctx.num_arch_switches; iswitch++){
			char *iswitch_name = ArchSwitches[iswitch].name;
			if (0 == strcmp(ipin_cblock_switch_name, iswitch_name)){
				ipin_cblock_switch_index = iswitch;
				break;
			}
		}
		if (ipin_cblock_switch_index == -1){
			vpr_throw(VPR_ERROR_OTHER,__FILE__, __LINE__, "Could not find arch switch matching name %s\n", ipin_cblock_switch_name);
		}

		RoutingArch->wire_to_arch_ipin_switch = ipin_cblock_switch_index;
	}

	/* Depends on device_ctx.num_arch_switches */
	RoutingArch->delayless_switch = device_ctx.num_arch_switches;
	RoutingArch->global_route_switch = RoutingArch->delayless_switch;
	++device_ctx.num_arch_switches;

	/* Alloc the list now that we know the final num_arch_switches value */
	device_ctx.arch_switch_inf = new t_arch_switch_inf[device_ctx.num_arch_switches];
	for (int iswitch = 0; iswitch < switches_to_copy; iswitch++){
		device_ctx.arch_switch_inf[iswitch] = ArchSwitches[iswitch];
	}

	/* Delayless switch for connecting sinks and sources with their pins. */
	device_ctx.arch_switch_inf[RoutingArch->delayless_switch].buffered = true;
	device_ctx.arch_switch_inf[RoutingArch->delayless_switch].R = 0.;
	device_ctx.arch_switch_inf[RoutingArch->delayless_switch].Cin = 0.;
	device_ctx.arch_switch_inf[RoutingArch->delayless_switch].Cout = 0.;
	device_ctx.arch_switch_inf[RoutingArch->delayless_switch].Tdel_map[UNDEFINED] = 0.;
	device_ctx.arch_switch_inf[RoutingArch->delayless_switch].power_buffer_type = POWER_BUFFER_TYPE_NONE;
	device_ctx.arch_switch_inf[RoutingArch->delayless_switch].mux_trans_size = 0.;

	/* If ipin cblock info has *not* been read in from a switch, then we have
	   created a new switch for it, and now need to set its values */
	if (NULL == Arch.ipin_cblock_switch_name){
		/* The wire to ipin switch for all types. Curently all types
		 * must share ipin switch. Some of the timing code would
		 * need to be changed otherwise. */
		device_ctx.arch_switch_inf[RoutingArch->wire_to_arch_ipin_switch].buffered = true;
		device_ctx.arch_switch_inf[RoutingArch->wire_to_arch_ipin_switch].R = 0.;
		device_ctx.arch_switch_inf[RoutingArch->wire_to_arch_ipin_switch].Cin = Arch.C_ipin_cblock;
		device_ctx.arch_switch_inf[RoutingArch->wire_to_arch_ipin_switch].Cout = 0.;
		device_ctx.arch_switch_inf[RoutingArch->wire_to_arch_ipin_switch].Tdel_map[UNDEFINED] = Arch.T_ipin_cblock;
		device_ctx.arch_switch_inf[RoutingArch->wire_to_arch_ipin_switch].power_buffer_type = POWER_BUFFER_TYPE_NONE;
		device_ctx.arch_switch_inf[RoutingArch->wire_to_arch_ipin_switch].mux_trans_size = Arch.ipin_mux_trans_size;

		/* Assume the ipin cblock output to lblock input buffer below is 4x     *
		 * minimum drive strength (enough to drive a fanout of up to 16 pretty  * 
		 * nicely) -- should cover a reasonable wiring C plus the fanout.       */
		device_ctx.arch_switch_inf[RoutingArch->wire_to_arch_ipin_switch].buf_size = trans_per_buf(Arch.R_minW_nmos / 4., Arch.R_minW_nmos, Arch.R_minW_pmos);
	}


}

/* Sets up routing structures. Since checks are already done, this
 * just copies values across */
static void SetupRoutingArch(const t_arch& Arch,
		t_det_routing_arch *RoutingArch) {

	RoutingArch->switch_block_type = Arch.SBType;
	RoutingArch->R_minW_nmos = Arch.R_minW_nmos;
	RoutingArch->R_minW_pmos = Arch.R_minW_pmos;
	RoutingArch->Fs = Arch.Fs;
	RoutingArch->directionality = BI_DIRECTIONAL;
	if (Arch.Segments){
		RoutingArch->directionality = Arch.Segments[0].directionality;
	}

	/* copy over the switch block information */
	RoutingArch->switchblocks = Arch.switchblocks;
}

static void SetupRouterOpts(const t_options& Options, t_router_opts *RouterOpts) {
	RouterOpts->astar_fac = Options.astar_fac;
	RouterOpts->bb_factor = Options.bb_factor;
	RouterOpts->criticality_exp = Options.criticality_exp;
	RouterOpts->max_criticality = Options.max_criticality;
	RouterOpts->max_router_iterations = Options.max_router_iterations;
	RouterOpts->min_incremental_reroute_fanout = Options.min_incremental_reroute_fanout;
	RouterOpts->pres_fac_mult = Options.pres_fac_mult;
	RouterOpts->route_type = Options.RouteType;

	RouterOpts->full_stats = Options.full_stats;

    //TODO document these?
	RouterOpts->congestion_analysis = Options.full_stats;
	RouterOpts->fanout_analysis = Options.full_stats;
    RouterOpts->switch_usage_analysis = Options.full_stats;

	RouterOpts->verify_binary_search = Options.verify_binary_search;
	RouterOpts->router_algorithm = Options.RouterAlgorithm;
	RouterOpts->fixed_channel_width = Options.RouteChanWidth;
	RouterOpts->min_channel_width_hint = Options.min_route_chan_width_hint;

    //TODO document these?
	RouterOpts->trim_empty_channels = false; /* DEFAULT */
	RouterOpts->trim_obs_channels = false; /* DEFAULT */

	RouterOpts->initial_pres_fac = Options.initial_pres_fac;
	RouterOpts->base_cost_type = Options.base_cost_type;
	RouterOpts->first_iter_pres_fac = Options.first_iter_pres_fac;
	RouterOpts->acc_fac = Options.acc_fac;
	RouterOpts->bend_cost = Options.bend_cost;
	RouterOpts->doRouting = Options.do_routing;
	RouterOpts->routing_failure_predictor = Options.routing_failure_predictor;
	RouterOpts->write_rr_graph_name = Options.write_rr_graph_file;
    RouterOpts->read_rr_graph_name = Options.read_rr_graph_file;
}

static void SetupAnnealSched(const t_options& Options,
		t_annealing_sched *AnnealSched) {
	AnnealSched->alpha_t = Options.PlaceAlphaT;
	if (AnnealSched->alpha_t >= 1 || AnnealSched->alpha_t <= 0) {
		vpr_throw(VPR_ERROR_OTHER,__FILE__, __LINE__, "alpha_t must be between 0 and 1 exclusive.\n");
	}

	AnnealSched->exit_t = Options.PlaceExitT;
	if (AnnealSched->exit_t <= 0) {
		vpr_throw(VPR_ERROR_OTHER,__FILE__, __LINE__, "exit_t must be greater than 0.\n");
	}

	AnnealSched->init_t = Options.PlaceInitT;
	if (AnnealSched->init_t <= 0) {
		vpr_throw(VPR_ERROR_OTHER,__FILE__, __LINE__, "init_t must be greater than 0.\n");
	}

	if (AnnealSched->init_t < AnnealSched->exit_t) {
		vpr_throw(VPR_ERROR_OTHER,__FILE__, __LINE__, "init_t must be greater or equal to than exit_t.\n");
	}

	AnnealSched->inner_num = Options.PlaceInnerNum;
	if (AnnealSched->inner_num <= 0) {
		vpr_throw(VPR_ERROR_OTHER,__FILE__, __LINE__, "inner_num must be greater than 0.\n");
	}

	AnnealSched->type = Options.anneal_sched_type;
}

/* Sets up the s_packer_opts structure baesd on users inputs and on the architecture specified.  
 * Error checking, such as checking for conflicting params is assumed to be done beforehand 
 */
void SetupPackerOpts(const t_options& Options,
		const t_arch& Arch, const char *net_file,
		t_packer_opts *PackerOpts) {

	if (Arch.clb_grid.IsAuto) {
		PackerOpts->aspect = Arch.clb_grid.Aspect;
	} else {
		PackerOpts->aspect = (float) Arch.clb_grid.H / (float) Arch.clb_grid.W;
	}
	PackerOpts->output_file = net_file;

	PackerOpts->blif_file_name = Options.BlifFile;

	PackerOpts->doPacking = Options.do_packing;

    //TODO: document?
	PackerOpts->global_clocks = true; /* DEFAULT */
	PackerOpts->hill_climbing_flag = false; /* DEFAULT */

	PackerOpts->allow_unrelated_clustering = Options.allow_unrelated_clustering;
	PackerOpts->connection_driven = Options.connection_driven_clustering;
	PackerOpts->timing_driven = Options.timing_driven_clustering;
	PackerOpts->cluster_seed_type = Options.cluster_seed_type;
	PackerOpts->alpha = Options.alpha_clustering;
	PackerOpts->beta = Options.beta_clustering;

    //TODO: document?
	PackerOpts->inter_cluster_net_delay = 1.0; /* DEFAULT */
	PackerOpts->auto_compute_inter_cluster_net_delay = true;
	PackerOpts->packer_algorithm = PACK_GREEDY; /* DEFAULT */
}

static void SetupNetlistOpts(const t_options& Options, t_netlist_opts& NetlistOpts) {
    
    NetlistOpts.absorb_buffer_luts = Options.absorb_buffer_luts;
    NetlistOpts.sweep_dangling_primary_ios = Options.sweep_dangling_primary_ios;
    NetlistOpts.sweep_dangling_nets = Options.sweep_dangling_nets;
    NetlistOpts.sweep_dangling_blocks = Options.sweep_dangling_blocks;
    NetlistOpts.sweep_constant_primary_outputs = Options.sweep_constant_primary_outputs;
}

/* Sets up the s_placer_opts structure based on users input. Error checking,
 * such as checking for conflicting params is assumed to be done beforehand */
static void SetupPlacerOpts(const t_options& Options, t_placer_opts *PlacerOpts) {

	PlacerOpts->doPlacement = Options.do_placement;

	PlacerOpts->inner_loop_recompute_divider = Options.inner_loop_recompute_divider;

    //TODO: document?
	PlacerOpts->place_cost_exp = 1;

	PlacerOpts->td_place_exp_first = Options.place_exp_first;

	PlacerOpts->td_place_exp_last = Options.place_exp_last;

	PlacerOpts->place_algorithm = Options.PlaceAlgorithm;

	PlacerOpts->pad_loc_file = Options.pad_loc_file;
	PlacerOpts->pad_loc_type = Options.pad_loc_type;

	PlacerOpts->place_chan_width = Options.PlaceChanWidth;

	PlacerOpts->recompute_crit_iter = Options.RecomputeCritIter;

	PlacerOpts->timing_tradeoff = Options.PlaceTimingTradeoff;

	/* Depends on PlacerOpts->place_algorithm */
	PlacerOpts->enable_timing_computations = Options.ShowPlaceTiming;

    //TODO: document?
	PlacerOpts->place_freq = PLACE_ONCE; /* DEFAULT */
}

static void SetupAnalysisOpts(const t_options& Options, t_analysis_opts& analysis_opts) {
	analysis_opts.doAnalysis = Options.do_analysis;

    analysis_opts.gen_post_synthesis_netlist = Options.Generate_Post_Synthesis_Netlist;
}

static void SetupPowerOpts(const t_options& Options, t_power_opts *power_opts,
		t_arch * Arch) {

    auto& device_ctx = g_vpr_ctx.mutable_device();

    power_opts->do_power = Options.do_power;

	if (power_opts->do_power) {
		if (!Arch->power)
			Arch->power = (t_power_arch*) vtr::malloc(sizeof(t_power_arch));
		if (!Arch->clocks)
			Arch->clocks = (t_clock_arch*) vtr::malloc(sizeof(t_clock_arch));
		device_ctx.clock_arch = Arch->clocks;
	} else {
		Arch->power = NULL;
		Arch->clocks = NULL;
		device_ctx.clock_arch = NULL;
	}
}
