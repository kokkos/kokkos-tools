//@HEADER
// ************************************************************************
//
//                        Kokkos v. 3.0
//       Copyright (2020) National Technology & Engineering
//               Solutions of Sandia, LLC (NTESS).
//
// Under the terms of Contract DE-NA0003525 with NTESS,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY NTESS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL NTESS OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact David Poliakoff (dzpolia@sandia.gov)
//
// ************************************************************************
//@HEADER

//Modified by Zach Frye at LLNL
//Contact: frye7@llnl.gov 
//Organization: CASC at LLNL

#include <stdio.h>
#include <inttypes.h>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <unordered_set>
#include <string>
#include <regex>
#include <cxxabi.h>
#include <dlfcn.h>
#include <ctime>
#include <chrono>
#include <iostream>

//variorum trial run
extern "C" {
#include <variorum.h>
}

bool filterKernels;
uint64_t nextKernelID;
std::vector<std::regex> kernelNames;
std::unordered_set<uint64_t> activeKernels;

typedef void (*initFunction)(const int, const uint64_t, const uint32_t, void*);
typedef void (*finalizeFunction)();
typedef void (*beginFunction)(const char*, const uint32_t, uint64_t*);
typedef void (*endFunction)(uint64_t);

static initFunction initProfileLibrary = NULL;
static finalizeFunction finalizeProfileLibrary = NULL;
static beginFunction beginForCallee = NULL;
static beginFunction beginScanCallee = NULL;
static beginFunction beginReduceCallee = NULL;
static endFunction endForCallee = NULL;
static endFunction endScanCallee = NULL;
static endFunction endReduceCallee = NULL;

//Function: variorum_print_power_call()
//Description: function that will call print power and handle the execution errors
//Pre: None
//Post: Will print an error message if variorum print power fails. No return value. 

void variorum_print_power_call() {
    int ret;
    //Pre: No Arguments required for print power
    //Post: An integer value that represents the return success/error code
    //Description: variorum_print_power prints out the current architecture specific power usage measurements
    //at the moment that it is called to standard out. 
	ret = variorum_print_power();
    if (ret != 0) {
        printf("Print power failed!\n");
    }
}

//variables for simple timer
time_t start_time;


extern "C" void kokkosp_init_library(const int loadSeq,
	const uint64_t interfaceVer, const uint32_t devInfoCount, void* deviceInfo) {
    
    int ret;
    //Simple timer code to keep track of the general amount of time the application ran for.
    time(&start_time);
    std::cout << "Start Time: " << start_time << "\n";

    //print variorum power when Kokkos is initialized called
	ret = variorum_print_power();
    variorum_print_power_call();
}

extern "C" void kokkosp_finalize_library() {
    //print variorum power when kokkos finalize is called 
    variorum_print_power_call();
    time_t end_time;
    time(&end_time);
    std::cout << "End Time: " << end_time << "\nStart Time: " << start_time << std::endl;
    time_t total_time = end_time - start_time;

    std::cout << "The kokkos library was alive for " << total_time << " seconds." << "\n";
}

extern "C" void kokkosp_begin_parallel_for(const char* name, const uint32_t devID, uint64_t* kID) {

    int ret;
    //print variorum power whenever parallelfor is called
	variorum_print_power_call();
}

extern "C" void kokkosp_end_parallel_for(const uint64_t kID) {

    //print variorum power ever time a parallel for has finished executing 
    variorum_print_power_call();
}

extern "C" void kokkosp_begin_parallel_scan(const char* name, const uint32_t devID, uint64_t* kID) {
	
    int ret;
    //print variorum power when parallel scan begins 
	variorum_print_power_call();
}

extern "C" void kokkosp_end_parallel_scan(const uint64_t kID) {
    
    int ret;
    //print power when a parallel scan completes
	variorum_print_power_call();
	
}

extern "C" void kokkosp_begin_parallel_reduce(const char* name, const uint32_t devID, uint64_t* kID) {
    
    int ret;
    //print power when a parallel reduce begins
	variorum_print_power_call();
}

extern "C" void kokkosp_end_parallel_reduce(const uint64_t kID) {
    int ret;

    //print power when a parallel reduce completes 
	variorum_print_power_call();
}


