#ifndef SETUPGRID_H
#define SETUPGRID_H

/*
 Author: Jason Luu
 Date: October 8, 2008

 Initializes and allocates the physical logic block grid for VPR.

 */
#include <vector>
#include "physical_types.h"

//Find the device satisfying the specified minimum resources
DeviceGrid create_device_grid(std::string layout_name, std::vector<t_grid_def> grid_layouts, std::map<t_type_ptr,size_t> minimum_instance_counts);

//Find the device close in size to the specified dimensions
DeviceGrid create_device_grid(std::string layout_name, std::vector<t_grid_def> grid_layouts, size_t min_width, size_t min_height);


#endif

