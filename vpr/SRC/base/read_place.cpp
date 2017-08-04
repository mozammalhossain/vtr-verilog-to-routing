#include <cstdio>
#include <cstring>
#include <fstream>

#include "vtr_assert.h"
#include "vtr_util.h"
#include "vtr_log.h"
#include "vtr_digest.h"

#include "vpr_types.h"
#include "vpr_error.h"

#include "globals.h"
#include "hash.h"
#include "read_place.h"
#include "read_xml_arch_file.h"

t_block* find_block(t_block* blocks, int num_blocks, std::string name);

void read_place(const char* net_file, 
                const char* place_file,
                bool verify_file_digests,
                const int L_nx, const int L_ny,
		        const int L_num_blocks, t_block block_list[]) {
    std::ifstream fstream(place_file); 
    if (!fstream) {
        vpr_throw(VPR_ERROR_PLACE_F, __FILE__, __LINE__, 
                        "'%s' - Cannot open place file.\n", 
                        place_file);
    }

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.mutable_placement();

    std::string line;
    int lineno = 0;
    bool seen_netlist_id = false;
    bool seen_grid_dimensions = false;
    while (std::getline(fstream, line)) { //Parse line-by-line
        ++lineno;

        std::vector<std::string> tokens = vtr::split(line);

        if (tokens.empty()) {
            continue; //Skip blank lines

        } else if (tokens[0][0] == '#') {
            continue; //Skip commented lines

        } else if (tokens.size() == 4 
                   && tokens[0] == "Netlist_File:" 
                   && tokens[2] == "Netlist_ID:") {
            //Check that the netlist used to generate this placement matches the one loaded
            //
            //NOTE: this is an optional check which causes no errors if this line is missing.
            //      This ensures other tools can still generate placement files which can be loaded 
            //      by VPR.

            if (seen_netlist_id) {
                vpr_throw(VPR_ERROR_PLACE_F, place_file, lineno, 
                          "Duplicate Netlist_File/Netlist_ID specification");
            }
            
            std::string place_netlist_id = tokens[3];
            std::string place_netlist_file = tokens[1];

            if (place_netlist_id != cluster_ctx.clbs_nlist.netlist_id) {
                auto msg = vtr::string_fmt("The packed netlist file that generated placement (File: '%s' ID: '%s')"
                                           " does not match current netlist (File: '%s' ID: '%s')", 
                                           place_netlist_file.c_str(), place_netlist_id.c_str(), 
                                           net_file, cluster_ctx.clbs_nlist.netlist_id.c_str());
                if (verify_file_digests) {
                    vpr_throw(VPR_ERROR_PLACE_F, place_file, lineno, msg.c_str());
                } else {
                    vtr::printf_warning(place_file, lineno, "%s\n", msg.c_str());
                }
            }

            seen_netlist_id = true;

        } else if (tokens.size() == 7 
                   && tokens[0] == "Array"
                   && tokens[1] == "size:"
                   && tokens[3] == "x"
                   && tokens[5] == "logic"
                   && tokens[6] == "blocks") {
            //Load the device grid dimensions

            if (seen_grid_dimensions) {
                vpr_throw(VPR_ERROR_PLACE_F, place_file, lineno, 
                          "Duplicate device grid dimensions specification");
            }

            int place_file_nx = vtr::atoi(tokens[2]);
            int place_file_ny = vtr::atoi(tokens[4]);
            if (L_nx != place_file_nx || L_ny != place_file_ny) {
                vpr_throw(VPR_ERROR_PLACE_F, place_file, lineno, 
                        "Current FPGA size (%d x %d) is different from size when placement generated (%d x %d)", 
                        L_nx, L_ny, place_file_nx, place_file_ny);
            }

            seen_grid_dimensions = true;

        } else if (tokens.size() == 4 || (tokens.size() == 5 && tokens[4][0] == '#')) {
            //Load the block location
            //
            //We should have 4 tokens of actual data, with an optional 5th (commented) token indicating VPR's
            //internal block number

            //Grid dimensions are required by this point
            if (!seen_grid_dimensions) {
                vpr_throw(VPR_ERROR_PLACE_F, place_file, lineno, 
                        "Missing device grid size specification");
            }

            std::string block_name = tokens[0];
            int block_x = vtr::atoi(tokens[1]);
            int block_y = vtr::atoi(tokens[2]);
            int block_z = vtr::atoi(tokens[3]);

            t_block* blk = find_block(block_list, L_num_blocks, block_name);

            if (!blk) {
                vpr_throw(VPR_ERROR_PLACE_F, place_file, lineno, 
                          "Block '%s' in placement file does not exist in netlist.",
                          block_name.c_str());
            }

            if (place_ctx.block_locs.size() != static_cast<size_t>(L_num_blocks)) {
                //Resize if needed
                place_ctx.block_locs.resize(L_num_blocks);
            }

            int iblk = blk - block_list;
            VTR_ASSERT(iblk >= 0 && iblk < L_num_blocks);

            //Set the location
            place_ctx.block_locs[iblk].x = block_x;
            place_ctx.block_locs[iblk].y = block_y;
            place_ctx.block_locs[iblk].z = block_z;

        } else {
            //Unrecognized
            vpr_throw(VPR_ERROR_PLACE_F, place_file, lineno, 
                      "Invalid line '%s' in placement file",
                      line.c_str());
        }
    }

    place_ctx.placement_id = vtr::secure_digest_file(place_file);
}

void read_user_pad_loc(const char *pad_loc_file) {

	/* Reads in the locations of the IO pads from a file. */

	t_hash **hash_table, *h_ptr;
	int iblk, xtmp, ytmp;
	FILE *fp;
	char buf[vtr::bufsize], bname[vtr::bufsize], *ptr;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& device_ctx = g_vpr_ctx.device();
    auto& place_ctx = g_vpr_ctx.mutable_placement();

	vtr::printf_info("\n");
	vtr::printf_info("Reading locations of IO pads from '%s'.\n", pad_loc_file);
	fp = fopen(pad_loc_file, "r");
	if (!fp) vpr_throw(VPR_ERROR_PLACE_F, __FILE__, __LINE__, 
				"'%s' - Cannot find IO pads location file.\n", 
				pad_loc_file);
		
	hash_table = alloc_hash_table();
	for (iblk = 0; iblk < cluster_ctx.num_blocks; iblk++) {
		if (cluster_ctx.blocks[iblk].type == device_ctx.IO_TYPE) {
			insert_in_hash_table(hash_table, cluster_ctx.blocks[iblk].name, iblk);
			place_ctx.block_locs[iblk].x = OPEN; /* Mark as not seen yet. */
		}
	}

	for (size_t i = 0; i < device_ctx.grid.width(); i++) {
		for (size_t j = 0; j < device_ctx.grid.height(); j++) {
			if (device_ctx.grid[i][j].type == device_ctx.IO_TYPE) {
				for (int k = 0; k < device_ctx.IO_TYPE->capacity; k++) {
					if (place_ctx.grid_blocks[i][j].blocks[k] != INVALID_BLOCK) {
						place_ctx.grid_blocks[i][j].blocks[k] = EMPTY_BLOCK; /* Flag for err. check */
					}
				}
			}
		}
	}

	ptr = vtr::fgets(buf, vtr::bufsize, fp);

	while (ptr != NULL) {
		ptr = vtr::strtok(buf, TOKENS, fp, buf);
		if (ptr == NULL) {
			ptr = vtr::fgets(buf, vtr::bufsize, fp);
			continue; /* Skip blank or comment lines. */
		}

        if(strlen(ptr) + 1 < vtr::bufsize) {
            strcpy(bname, ptr);
        } else {
			vpr_throw(VPR_ERROR_PLACE_F, pad_loc_file, vtr::get_file_line_number_of_last_opened_file(), 
					"Block name exceeded buffer size of %zu characters", vtr::bufsize);
        }

		ptr = vtr::strtok(NULL, TOKENS, fp, buf);
		if (ptr == NULL) {
			vpr_throw(VPR_ERROR_PLACE_F, pad_loc_file, vtr::get_file_line_number_of_last_opened_file(), 
					"Incomplete.\n");
		}
		sscanf(ptr, "%d", &xtmp);

		ptr = vtr::strtok(NULL, TOKENS, fp, buf);
		if (ptr == NULL) {
			vpr_throw(VPR_ERROR_PLACE_F, pad_loc_file, vtr::get_file_line_number_of_last_opened_file(), 
					"Incomplete.\n");
		}
		sscanf(ptr, "%d", &ytmp);

		ptr = vtr::strtok(NULL, TOKENS, fp, buf);
		if (ptr == NULL) {
			vpr_throw(VPR_ERROR_PLACE_F, pad_loc_file, vtr::get_file_line_number_of_last_opened_file(), 
					"Incomplete.\n");
		}
        int k;
		sscanf(ptr, "%d", &k);

		ptr = vtr::strtok(NULL, TOKENS, fp, buf);
		if (ptr != NULL) {
			vpr_throw(VPR_ERROR_PLACE_F, pad_loc_file, vtr::get_file_line_number_of_last_opened_file(), 
					"Extra characters at end of line.\n");
		}

		h_ptr = get_hash_entry(hash_table, bname);
		if (h_ptr == NULL) {
			vtr::printf_warning(__FILE__, __LINE__, 
					"[Line %d] Block %s invalid, no such IO pad.\n", vtr::get_file_line_number_of_last_opened_file(), bname);
			ptr = vtr::fgets(buf, vtr::bufsize, fp);
			continue;
		}
		int bnum = h_ptr->index;
		int i = xtmp;
		int j = ytmp;

		if (place_ctx.block_locs[bnum].x != OPEN) {
			vpr_throw(VPR_ERROR_PLACE_F, pad_loc_file, vtr::get_file_line_number_of_last_opened_file(), 
					"Block %s is listed twice in pad file.\n", bname);
		}

		if (i < 0 || i > device_ctx.nx + 1 || j < 0 || j > device_ctx.ny + 1) {
			vpr_throw(VPR_ERROR_PLACE_F, pad_loc_file, 0, 
					"Block #%d (%s) location, (%d,%d) is out of range.\n", bnum, bname, i, j);
		}

        place_ctx.block_locs[bnum].x = i; /* Will be reloaded by initial_placement anyway. */
        place_ctx.block_locs[bnum].y = j; /* We need to set .x only as a done flag.         */
        place_ctx.block_locs[bnum].z = k;
        place_ctx.block_locs[bnum].is_fixed = true;

		if (device_ctx.grid[i][j].type != device_ctx.IO_TYPE) {
			vpr_throw(VPR_ERROR_PLACE_F, pad_loc_file, 0, 
					"Attempt to place IO block %s at illegal location (%d, %d).\n", bname, i, j);
		}

		if (k >= device_ctx.IO_TYPE->capacity || k < 0) {
			vpr_throw(VPR_ERROR_PLACE_F, pad_loc_file, vtr::get_file_line_number_of_last_opened_file(), 
					"Block %s subblock number (%d) is out of range.\n", bname, k);
		}
		place_ctx.grid_blocks[i][j].blocks[k] = bnum;
		place_ctx.grid_blocks[i][j].usage++;

		ptr = vtr::fgets(buf, vtr::bufsize, fp);
	}

	for (iblk = 0; iblk < cluster_ctx.num_blocks; iblk++) {
		if (cluster_ctx.blocks[iblk].type == device_ctx.IO_TYPE && place_ctx.block_locs[iblk].x == OPEN) {
			vpr_throw(VPR_ERROR_PLACE_F, pad_loc_file, 0, 
					"IO block %s location was not specified in the pad file.\n", cluster_ctx.blocks[iblk].name);
		}
	}

	fclose(fp);
	free_hash_table(hash_table);
	vtr::printf_info("Successfully read %s.\n", pad_loc_file);
	vtr::printf_info("\n");
}

void print_place(const char* net_file, 
                 const char* net_id, 
                 const char* place_file) {

	/* Prints out the placement of the circuit. The architecture and     *
	 * netlist files used to generate this placement are recorded in the *
	 * file to avoid loading a placement with the wrong support files    *
	 * later.                                                            */

	FILE *fp;
	int i;

    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.mutable_placement();

	fp = fopen(place_file, "w");

	fprintf(fp, "Netlist_File: %s Netlist_ID: %s\n", 
            net_file,
            net_id);
	fprintf(fp, "Array size: %d x %d logic blocks\n\n", device_ctx.nx, device_ctx.ny);
	fprintf(fp, "#block name\tx\ty\tsubblk\tblock number\n");
	fprintf(fp, "#----------\t--\t--\t------\t------------\n");

	for (i = 0; i < cluster_ctx.num_blocks; i++) {
		fprintf(fp, "%s\t", cluster_ctx.blocks[i].name);
		if (strlen(cluster_ctx.blocks[i].name) < 8)
			fprintf(fp, "\t");

		fprintf(fp, "%d\t%d\t%d", place_ctx.block_locs[i].x, place_ctx.block_locs[i].y, place_ctx.block_locs[i].z);
		fprintf(fp, "\t#%d\n", i);
	}
	fclose(fp);

    //Calculate the ID of the placement
    place_ctx.placement_id = vtr::secure_digest_file(place_file);
}

t_block* find_block(t_block* blocks, int nblocks, std::string name) {
    t_block* blk = NULL;
    for (int i = 0; i < nblocks; ++i) {
        if (blocks[i].name == name) {
            blk = (blocks + i);
            break;
        }
    }
    return blk;
}
