#ifndef VPR_UTILS_H
#define VPR_UTILS_H

#include "netlist.h"
class DeviceGrid;

const t_model* find_model(const t_model* models, const std::string& name, bool required=true);
const t_model_ports* find_model_port(const t_model* model, const std::string& name, bool required=true);

void print_tabs(FILE * fpout, int num_tab);

bool is_clb_external_pin(int clb, int pb_pin_id);

bool is_opin(int ipin, t_type_ptr type);

int get_unique_pb_graph_node_id(const t_pb_graph_node *pb_graph_node);

void get_class_range_for_block(const int iblk, int *class_low,
		int *class_high);

void sync_grid_to_blocks();

/**************************************************************
* Intra-Logic Block Utility Functions
**************************************************************/

//Class for looking up pb graph pins from block pin indicies
class IntraLbPbPinLookup {
    public:
        IntraLbPbPinLookup(t_type_descriptor* block_types, int num_block_types);
        IntraLbPbPinLookup(const IntraLbPbPinLookup& rhs);
        IntraLbPbPinLookup& operator=(IntraLbPbPinLookup rhs);
        ~IntraLbPbPinLookup();


        //Returns the pb graph pin associated with the specified type (index into block types array) and
        // pb pin index (index into block[].pb_route)
        const t_pb_graph_pin* pb_gpin(int itype, int ipin) const;

        friend void swap(IntraLbPbPinLookup& lhs, IntraLbPbPinLookup& rhs);
    private:
        t_type_descriptor* block_types_;
        int num_types_;

        t_pb_graph_pin*** intra_lb_pb_pin_lookup_; 
};

//Find the atom pins (driver or sinks) connected to the specified top-level CLB pin
std::vector<AtomPinId> find_clb_pin_connected_atom_pins(int clb, int clb_pin, const IntraLbPbPinLookup& pb_gpin_lookup);

//Find the atom pin driving to the specified top-level CLB pin
AtomPinId find_clb_pin_driver_atom_pin(int clb, int clb_pin, const IntraLbPbPinLookup& pb_gpin_lookup);

//Find the atom pins driven by the specified top-level CLB pin
std::vector<AtomPinId> find_clb_pin_sink_atom_pins(int clb, int clb_pin, const IntraLbPbPinLookup& pb_gpin_lookup);

const t_net_pin* find_pb_route_clb_input_net_pin(int clb, int sink_pb_route_id);

//Return the pb pin index corresponding to the pin clb_pin on block clb
int find_clb_pb_pin(int clb, int clb_pin);

//Return the clb_pin corresponding to the pb_pin on the specified block
int find_pb_pin_clb_pin(int clb, int pb_pin);

//Returns the port matching name within pb_gnode
const t_port* find_pb_graph_port(const t_pb_graph_node* pb_gnode, std::string port_name);

//Returns the graph pin matching name at pin index
const t_pb_graph_pin* find_pb_graph_pin(const t_pb_graph_node* pb_gnode, std::string port_name, int index);

AtomPinId find_atom_pin(int iblk, const t_pb_graph_pin* pb_gpin);

//Returns the block type matching name, or nullptr (if not found)
t_type_descriptor* find_block_type_by_name(std::string name, t_type_descriptor* types, int num_types);

t_type_ptr find_most_common_block_type(const DeviceGrid& grid);

//Returns true if the specified block type contains the specified blif model name
bool block_type_contains_blif_model(t_type_ptr type, std::string blif_model_name);

//Returns true of a pb_type (or it's children) contain the specified blif model name
bool pb_type_contains_blif_model(const t_pb_type* pb_type, const std::string& blif_model_name);

int get_max_primitives_in_pb_type(t_pb_type *pb_type);
int get_max_depth_of_pb_type(t_pb_type *pb_type);
int get_max_nets_in_pb_type(const t_pb_type *pb_type);
bool primitive_type_feasible(AtomBlockId blk_id, const t_pb_type *cur_pb_type);
t_pb_graph_pin* get_pb_graph_node_pin_from_model_port_pin(const t_model_ports *model_port, const int model_pin, const t_pb_graph_node *pb_graph_node);
const t_pb_graph_pin* find_pb_graph_pin(const AtomNetlist& netlist, const AtomLookup& netlist_lookup, const AtomPinId pin_id);
t_pb_graph_pin* get_pb_graph_node_pin_from_g_clbs_nlist_pin(const t_net_pin& pin);
t_pb_graph_pin* get_pb_graph_node_pin_from_g_clbs_nlist_net(int inet, int ipin);
t_pb_graph_pin* get_pb_graph_node_pin_from_block_pin(int iblock, int ipin);
t_pb_graph_pin** alloc_and_load_pb_graph_pin_lookup_from_index(t_type_ptr type);
void free_pb_graph_pin_lookup_from_index(t_pb_graph_pin** pb_graph_pin_lookup_from_type);
t_pb ***alloc_and_load_pin_id_to_pb_mapping();
void free_pin_id_to_pb_mapping(t_pb ***pin_id_to_pb_mapping);


float compute_primitive_base_cost(const t_pb_graph_node *primitive);
int num_ext_inputs_atom_block(AtomBlockId blk_id);

vtr::Matrix<int> alloc_and_load_net_pin_index();

void get_port_pin_from_blk_pin(int blk_type_index, int blk_pin, int * port,
		int * port_pin);
void free_port_pin_from_blk_pin(void);

void get_blk_pin_from_port_pin(int blk_type_index, int port,int port_pin, 
		int * blk_pin);
void free_blk_pin_from_port_pin(void);

void alloc_and_load_idirect_from_blk_pin(t_direct_inf* directs, int num_directs,
		int *** idirect_from_blk_pin, int *** direct_type_from_blk_pin);

void parse_direct_pin_name(char * src_string, int line, int * start_pin_index, 
		int * end_pin_index, char * pb_type_name, char * port_name);


void free_pb_stats(t_pb *pb);
void free_pb(t_pb *pb);
void revalid_molecules(const t_pb* pb, const std::multimap<AtomBlockId,t_pack_molecule*>& atom_molecules);

void print_switch_usage();
void print_usage_by_wire_length();

AtomBlockId find_tnode_atom_block(int inode);
AtomBlockId find_memory_sibling(const t_pb* pb);


void place_sync_all_external_block_connections();
void place_sync_external_block_connections(int iblk);
#endif

