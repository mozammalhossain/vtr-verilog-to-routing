/* This file defines the writing rr graph function in XML format */
#include <fstream>
#include <iostream>
#include <string.h>
#include <iomanip>
#include "vpr_error.h"
#include "globals.h"
#include "read_xml_arch_file.h"
#include "vtr_version.h"
#include "rr_graph_writer.h"

using namespace std;

/*********************** Subroutines local to this module *******************/
void write_rr_channel(fstream &fp);
void write_rr_node(fstream &fp);
void write_rr_switches(fstream &fp);
void write_rr_grid(fstream &fp);
void write_rr_edges(fstream &fp);
void write_rr_block_types(fstream &fp);
void write_rr_segments(fstream &fp, const t_segment_inf *segment_inf, const int num_seg_types);

/************************ Subroutine definitions ****************************/
/* This function is used to write the rr_graph into xml format into a a file with name: file_name */
void write_rr_graph(const char *file_name, const t_segment_inf *segment_inf, const int num_seg_types) {
    fstream fp;
    fp.open(file_name, fstream::out | fstream::trunc);

    /* Prints out general info for easy error checking*/
    if (!fp.is_open() || !fp.good()) {
        vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                "couldn't open file \"%s\" for generating RR graph file\n", file_name);
    }
    cout << "Writing RR graph" << endl;
    fp << "<rr_graph tool_name=\"vpr\" tool_version=\"" << vtr::VERSION <<
            "\" tool_comment=\"Generated from arch file "
            << get_arch_file_name() << "\">" << endl;

    /* Write out each individual component*/
    write_rr_channel(fp);
    write_rr_switches(fp);
    write_rr_segments(fp, segment_inf, num_seg_types);
    write_rr_block_types(fp);
    write_rr_grid(fp);
    write_rr_node(fp);
    write_rr_edges(fp);
    fp << "</rr_graph>";

    fp.close();

    cout << "Finished generating RR graph file named " << file_name << endl << endl;
}

/* Channel info in device_ctx.chan_width is written in xml format*/
void write_rr_channel(fstream &fp) {

    auto& device_ctx = g_vpr_ctx.device();
    fp << "\t<channels>" << endl;
    fp << "\t\t<channel chan_width_max =\"" << device_ctx.chan_width.max <<
            "\" x_min=\"" << device_ctx.chan_width.x_min <<
            "\" y_min=\"" << device_ctx.chan_width.y_min <<
            "\" x_max=\"" << device_ctx.chan_width.x_max <<
            "\" y_max=\"" << device_ctx.chan_width.y_max << "\"/>" << endl;

    int* list = device_ctx.chan_width.x_list;
    for (int i = 0; i <= device_ctx.ny; i++) {
        fp << "\t\t<x_list index =\"" << i << "\" info=\"" << list[i] << "\"/>" << endl;
    }
    list = device_ctx.chan_width.y_list;
    for (int i = 0; i <= device_ctx.nx; i++) {
        fp << "\t\t<y_list index =\"" << i << "\" info=\"" << list[i] << "\"/>" << endl;
    }
    fp << "\t</channels>" << endl;
}

/* All relevent rr node info is written out to the graph. 
 * This includes location, timing, and segment info*/
void write_rr_node(fstream &fp) {
    auto& device_ctx = g_vpr_ctx.device();

    fp << "\t<rr_nodes>" << endl;

    for (int inode = 0; inode < device_ctx.num_rr_nodes; inode++) {
        t_rr_node& node = device_ctx.rr_nodes[inode];
        fp << "\t\t<node id=\"" << inode << "\" type=\"" << node.type_string() <<
                "\" direction=\"" << node.direction_string() <<
                "\" capacity=\"" << node.capacity() << "\">" << endl;
        fp << "\t\t\t<loc xlow=\"" << node.xlow() << "\" ylow=\"" << node.ylow() <<
                "\" xhigh=\"" << node.xhigh() << "\" yhigh=\"" << node.yhigh() <<
                "\" ptc=\"" << node.ptc_num() << "\"/>" << endl;
        fp << "\t\t\t<timing R=\"" << setprecision(30)<< node.R() 
                << "\" C=\"" << setprecision(30)<<node.C() << "\"/>" << endl;
        
        if (device_ctx.rr_indexed_data[node.cost_index()].seg_index != -1) {
            fp << "\t\t\t<segment segment_id=\"" << device_ctx.rr_indexed_data[node.cost_index()].seg_index << "\"/>" << endl;
        }

        fp << "\t\t</node>" << endl;
    }

    fp << "\t</rr_nodes>" << endl << endl;
}

/* Segment information in the t_segment_inf data structure is written out.
 * Information includes segment id, name, and optional timing parameters*/
void write_rr_segments(fstream &fp, const t_segment_inf *segment_inf, const int num_seg_types) {
    fp << "\t<segments>" << endl;

    for (int iseg = 0; iseg < num_seg_types; iseg++) {
        fp << "\t\t<segment id=\"" << iseg <<
                "\" name=\"" << segment_inf[iseg].name << "\">" << endl;
        fp << "\t\t\t<timing R_per_meter=\"" << setprecision(30) <<segment_inf[iseg].Rmetal <<
                "\" C_per_meter=\"" <<setprecision(30) << segment_inf[iseg].Cmetal << "\"/>" << endl;
        fp << "\t\t</segment>" << endl;
    }
    fp << "\t</segments>" << endl << endl;
}

/* Switch info is written out into xml format. This includes
 * general, sizing, and optional timing information*/
void write_rr_switches(fstream &fp) {
    auto& device_ctx = g_vpr_ctx.device();
    fp << "\t<switches>" << endl;

    for (int iSwitch = 0; iSwitch < device_ctx.num_rr_switches; iSwitch++) {
        t_rr_switch_inf rr_switch = device_ctx.rr_switch_inf[iSwitch];

        fp << "\t\t<switch id=\"" << iSwitch;
        if (rr_switch.name) {
            fp << "\" name=\"" << rr_switch.name;
        }
        fp << "\" buffered=\"" << (int) rr_switch.buffered << "\">" << endl;
        fp << "\t\t\t<timing R=\"" << setprecision(30) <<rr_switch.R << 
                "\" Cin=\"" << setprecision(30) <<rr_switch.Cin <<
                "\" Cout=\"" << setprecision(30) <<rr_switch.Cout <<
                "\" Tdel=\"" << setprecision(30) <<rr_switch.Tdel << "\"/>" << endl;
        fp << "\t\t\t<sizing mux_trans_size=\"" << setprecision(30) <<rr_switch.mux_trans_size <<
                "\" buf_size=\"" << setprecision(30) <<rr_switch.buf_size << "\"/>" << endl;
        fp << "\t\t</switch>" << endl;
    }
    fp << "\t</switches>" << endl << endl;
}

/* Block information is printed out in xml format. This includes general,
 * pin class, and pins */
void write_rr_block_types(fstream &fp) {
    auto& device_ctx = g_vpr_ctx.device();
    fp << "\t<block_types>" << endl;

    for (int iBlock = 0; iBlock < device_ctx.num_block_types; iBlock++) {
        auto& btype = device_ctx.block_types[iBlock];

        fp << "\t\t<block_type id=\"" << btype.index;

        /*since the < symbol is not allowed in xml format, converted to only print "EMPTY"*/
        if (btype.name) {
            if (strcmp(btype.name, "<EMPTY>") == 0) {
                fp << "\" name=\"EMPTY";
            } else {
                fp << "\" name=\"" << btype.name;
            }
        }

        fp << "\" width=\"" << btype.width << "\" height=\"" << btype.height << "\">" << endl;

        for (int iClass = 0; iClass < btype.num_class; iClass++) {
            auto& class_inf = btype.class_inf[iClass];

            char const* pin_type;
            switch (class_inf.type) {
                case -1:
                    pin_type = "OPEN";
                    break;
                case 0:
                    pin_type = "OUTPUT"; //driver
                    break;
                case 1:
                    pin_type = "INPUT"; //receiver
                    break;
                default:
                    pin_type = "NONE";
                    break;
            }

            fp << "\t\t\t<pin_class type=\"" << pin_type << "\">";
            for (int iPin = 0; iPin < class_inf.num_pins; iPin++) {
                fp << class_inf.pinlist[iPin] << " ";
            }
            fp << "</pin_class>" << endl;
        }
        fp << "\t\t</block_type>" << endl;

    }
    fp << "\t</block_types>" << endl << endl;
}

/* Grid information is printed out in xml format. Each grid location
 * and its relevant information is included*/
void write_rr_grid(fstream &fp) {
    auto& device_ctx = g_vpr_ctx.device();

    fp << "\t<grid>" << endl;

    for (int x = 0; x < device_ctx.nx + 1; x++) {
        for (int y = 0; y < device_ctx.ny + 1; y++) {
            t_grid_tile grid_tile = device_ctx.grid[x][y];

            fp << "\t\t<grid_loc x=\"" << x << "\" y=\"" << y <<
                    "\" block_type_id=\"" << grid_tile.type->index <<
                    "\" width_offset=\"" << grid_tile.width_offset <<
                    "\" height_offset=\"" << grid_tile.height_offset << "\"/>" << endl;
        }
    }
    fp << "\t</grid>" << endl << endl;
}

/* Edges connecting to each rr node is printed out. The two nodes
 * it connects to are also printed*/
void write_rr_edges(fstream &fp) {
    auto& device_ctx = g_vpr_ctx.device();
    fp << "\t<rr_edges>" << endl;

    for (int inode = 0; inode < device_ctx.num_rr_nodes; inode++) {
        t_rr_node& node = device_ctx.rr_nodes[inode];
        for (int iedge = 0; iedge < node.num_edges(); iedge++) {
            fp << "\t\t<edge src_node=\"" << inode <<
                    "\" sink_node=\"" << node.edge_sink_node(iedge) <<
                    "\" switch_id=\"" << node.edge_switch(iedge) << "\"/>" << endl;
        }
    }
    fp << "\t</rr_edges>" << endl << endl;
}
