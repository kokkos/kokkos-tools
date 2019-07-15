
#ifndef _H_KOKKOSP_KERNEL_PAPI_CONNECTOR_INFO
#define _H_KOKKOSP_KERNEL_PAPI_CONNECTOR_INFO

#include <stdio.h>
#include <sys/time.h>
#include <cstring>

#include "papi.h"

struct SpaceHandle {
   char name[64];
};

/* stack for parallel_for */
std::stack<std::string> parallel_for_name;

/* stack for parallel_reduce */
std::stack<std::string> parallel_reduce_name;

/* stack for parallel_scan */
std::stack<std::string> parallel_scan_name;

/* stack for region names */
std::stack<std::string> region_name;

/* map of profile sections */
std::map<uint32_t, std::string> profile_section;


#endif
