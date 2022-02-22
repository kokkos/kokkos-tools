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
#include <fstream>


//variorum trial run
extern "C" {
#include <variorum.h>
#include <jansson.h>
}

#include "kp_core.hpp"

#if USE_MPI
#include <mpi.h>
#endif

namespace KokkosTools {
namespace VariorumConnector {

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

//the output path for ranekd output files
std::string printpath = "./";



//variables for simple timer
time_t start_time;

int type_of_profiling = 0; // 0 is for both print power & json, 1 is for print power, 2 is for json
bool usingMPI = false;
bool verbosePrint = false;
bool mpiOutPut = false;


//Function: variorum_print_power_call
//Description: Prints out power data in two ways: Verbose and non verbose.
//             verbose will print out each component of the systems power draw in the sierra architecture.
//             non-verbose will print the node power.
//Pre: None
//Post: Will print an error message if variorum print power fails. No return value. 
std::string variorum_print_power_call() {
    std::string outputString;
    json_t *power_obj  = json_object(); 
    double power_node, power_sock0, power_mem0, power_gpu0; 
    double power_sock1, power_mem1, power_gpu1;
    int ret;
	ret = variorum_get_node_power_json(power_obj);
    if (ret != 0) {
        return "Print power failed!\n";
    }
    //total node measurment
    power_node =  json_real_value(json_object_get(power_obj, "power_node"));
    const char * hostnameChar = json_string_value(json_object_get(power_obj, "hostname"));
    std::string hostname(hostnameChar);
    //print informatin to screen
    if(verbosePrint) {
        //socket 1 measurements
        power_sock0 =  json_real_value(json_object_get(power_obj, "power_cpu_socket_0"));
        power_mem0 =  json_real_value(json_object_get(power_obj, "power_mem_socket_0"));
        power_gpu0 =  json_real_value(json_object_get(power_obj, "power_gpu_socket_0"));
        //socket 2 measurements
        power_sock1 =  json_real_value(json_object_get(power_obj, "power_cpu_socket_1"));
        power_mem1 =  json_real_value(json_object_get(power_obj, "power_mem_socket_1"));
        power_gpu1 =  json_real_value(json_object_get(power_obj, "power_gpu_socket_1"));
        
        outputString += "HostName "+hostname+"\n";
        outputString += "Total Node Power: "+ std::to_string(power_node);
        outputString += "\n Socket 1 Power";
        outputString += "\n CPU Socket 1: "+ std::to_string(power_sock0);
        outputString += "\n Mem Socket 1: "+ std::to_string(power_mem0);
        outputString += "\n GPU Socket 1: "+ std::to_string(power_gpu0);
        outputString += "\n Socket 2 Power";
        outputString += "\n CPU Socket 2: "+ std::to_string(power_sock1);
        outputString += "\n Mem Socket 2: "+ std::to_string(power_mem1);
        outputString += "\n GPU Socket 2: "+ std::to_string(power_gpu1)+"\n"; 
    } else {
        outputString += hostname + ": " + std::to_string(power_node) + "\n";
    }

    return outputString;
}

//Function: variorum_json_call()
//Description: function that will call variorum print json and handle the execution errors
//Pre: None
//Post: Will print an error message if variorum print json fails. No return value. 
char * variorum_json_call() {
    int ret;
	json_t *my_power_obj = NULL;
	my_power_obj =
        json_object();
    ret = variorum_get_node_power_json(my_power_obj);
    if (ret != 0)
    {
        printf("First run: JSON get node power failed!\n");
    }
    char *s = json_dumps(my_power_obj, 0);
    return s;
}


//Function: variorum_call_mpi
//Description: This function will call the variourm helper functions and either write them to
//             output files or to std::cout depending on what options are selected
//Pre: None
//Post: An output message if variourum returned an error or if it functioned correctly
void variorum_call_mpi() {
#if USE_MPI
     if(usingMPI == true) {
        int rank;
        std::string output;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        std::ofstream file; 
        std::ofstream pfile;
        if (type_of_profiling == 0) { //if both print power and json options selected
            if (mpiOutPut) {
                std::string filenamejson = printpath+"variorum-output-mpi-rank-"+std::to_string(rank)+"-json.txt";
                std::string filenameprintp = printpath+"variorum-output-mpi-rank-"+std::to_string(rank)+".txt";
                
                file.open(filenamejson,std::ios_base::app);
                std::ofstream pfile;
                pfile.open(filenameprintp,std::ios_base::app);
                char *s = variorum_json_call();
                file << s;
                pfile << variorum_print_power_call();
            }
            else {
                std::cout << "MPI Rank " << rank << "\n";
                output = variorum_print_power_call();
                char *s = variorum_json_call();
                puts(s);
                std::cout << s << std::endl;
            }


        }
        else if (type_of_profiling == 1) { //if only print power is selected
            if (mpiOutPut) {
                std::string filenameprintp = printpath+"variorum-output-mpi-rank-"+std::to_string(rank)+".txt";
                std::ofstream pfile;
                pfile.open(filenameprintp,std::ios_base::app);
                pfile << variorum_print_power_call();
            }
            else {
                std::cout << "MPI Rank " << rank << "\n";
                output = variorum_print_power_call();
                std::cout << output << std::endl;
            }
        }
        else if (type_of_profiling == 2) { //if only json is selecte 
            if (mpiOutPut) {
                std::string filenamejson = printpath+"variorum-output-mpi-rank-"+std::to_string(rank)+"-json.txt";
                std::ofstream file; 
                file.open(filenamejson,std::ios_base::app);
                char *s = variorum_json_call();
                file << s;
            }
            else {
                std::cout << "MPI Rank " << rank << "\n";
                char *s = variorum_json_call();
                puts(s);
                std::cout << s << std::endl;
            }
        }
        file.close();
    }
#endif
}

//Function: variorum_call
//Description: The function determines what profiling options are selected and prints the profoiling data out to std out
//Pre: None
//Post: An output message if variourum returned an error or if it functioned correctly 

void variorum_call() {
    std::string output;
     if (type_of_profiling == 0) { 
        output = variorum_print_power_call();
        char *s = variorum_json_call();
        std::cout << s << "\n";
        std::cout << output << std::endl;
    }
    else if (type_of_profiling == 1) {
        output = variorum_print_power_call();
        std::cout << output << std::endl;
    }
    else if (type_of_profiling == 2) {
        char *s = variorum_json_call();
        std::cout << s << std::endl;
    }
}


void kokkosp_init_library(const int loadSeq, const uint64_t interfaceVer,
    const uint32_t devInfoCount, Kokkos_Profiling_KokkosPDeviceInfo* deviceInfo) {
    
    char * outputPathChar;
    try {
        outputPathChar = getenv("VARIORUM_OUTPUT_PATH");
        if (outputPathChar==NULL) {
            throw 10;
        }
        std::string outputPathStr(outputPathChar);
        printpath = outputPathStr;
        std::cout << "Output Path set to" << outputPathChar << "\n";
    } catch (int e) {
        if( e == 10) {
            printpath = "./";
            std::cout << "No output path provided, the application will output to the default path \n";
        }
    }

    //Profiling options are read in from their enviornment variables and the options are set
    char* profiling_type;
    try {
        profiling_type = getenv("KOKKOS_VARIORUM_FUNC_TYPE");
        if (profiling_type==NULL) {
            throw 10;
        }
        if(strcmp(profiling_type, "ppower") == 0) {
        type_of_profiling = 1;
        std::cout << "Variorum print power will be called\n";
        if(verbosePrint == true) {
            std::cout << "Power Format: \n Hostname: total node power, cpuSocket1, memScoket1, gpuSocket1, cpuSocket2, memScoket2, gpuSocket2 \n";
        } 
        }
        else if(strcmp(profiling_type, "json") == 0) {
            type_of_profiling = 2;
        }
        else if(strcmp(profiling_type, "both") == 0) {
            type_of_profiling = 0;
        }
    } catch (int e) {
        if( e == 10) {
            type_of_profiling = 0;
            std::cout << "No profiling options provided, profiling tool will call variorum print power and json \n";
        }
    }
    try {
        char* verbosePrintStr = getenv("VERBOSE");
        if (verbosePrintStr == NULL) {
            throw 10;
        }
        if (strcmp(verbosePrintStr, "false") == 0|| strcmp(verbosePrintStr, "") == 0 ) {
            throw 20;
        }
        else if (strcmp(verbosePrintStr, "true") == 0) {
            std::cout << "Verbose power option set" << "\n";
    		verbosePrint = true;
	    }
    } catch (int e) {
        verbosePrint = false;
        std::cout << "No verbose options provided, power information outut will not be verbose \n Format - Hosntame : Node power value\n";

    }
    try {
        char* usingMPIstr = getenv("VARIORUM_USING_MPI");
        if (usingMPIstr == NULL) {
            throw 10;
        }
        if (strcmp(usingMPIstr, "false") == 0|| strcmp(usingMPIstr, "") == 0 ) {
            throw 20;
        }
        if (strcmp(usingMPIstr, "true") == 0) {
#if USE_MPI
    		usingMPI = true;
            try{ 
                char* perRankOutput = getenv("RANKED_OUTPUT");
                if (strcmp(perRankOutput, "false") == 0|| strcmp(perRankOutput, "") == 0 ) {
                    mpiOutPut = false;
                }
                else if (strcmp(perRankOutput, "true") == 0 ) {
                    mpiOutPut = true;
                }
                else {
                    mpiOutPut = false;
                }

            } 
            catch(int f) {
                std::cout << "Ranked output will no be used, error setting paramters" << std::endl;
                mpiOutPut = false;
            }
#else
    		usingMPI = false;
            std::cout << "Ignoring MPI enabled in Variorum: the connector was built without MPI support" << std::endl;
#endif
	    }
    } catch (int e) {
        std::cout << "No MPI Option provided, not using per rank output" << std::endl;    
        usingMPI = false;
    }
    //Simple timer code to keep track of the general amount of time the application ran for.
    time(&start_time);
    std::cout << "Start Time: " << start_time << "\n";
    
    //variorum_call();
    
}

void kokkosp_finalize_library() {

    if(usingMPI) {
        variorum_call_mpi();
    }
    else {
        variorum_call();
    }
    time_t total_time;
    time_t end_time;
    time(&end_time);
    std::cout << "End Time: " << end_time << "\nStart Time: " << start_time << "\n";
    total_time = end_time - start_time;

    std::cout << "The kokkos library was alive for " << total_time << " seconds." << std::endl;
}

void kokkosp_begin_parallel_for(const char* name, const uint32_t devID, uint64_t* kID) {
    std::cout << "Device ID: " << devID << "\n";
    if(usingMPI) {
        variorum_call_mpi();
    }
    else {
        variorum_call();
    }
}

void kokkosp_end_parallel_for(const uint64_t kID) {
    if(usingMPI) {
        variorum_call_mpi();
    }
    else {
        variorum_call();
    }
}

void kokkosp_begin_parallel_scan(const char* name, const uint32_t devID, uint64_t* kID) {
	std::cout << "Device ID: " << devID << "\n";
    if(usingMPI) {
        variorum_call_mpi();
    }
    else {
        variorum_call();
    }
}

void kokkosp_end_parallel_scan(const uint64_t kID) {
    if(usingMPI) {
        variorum_call_mpi();
    }
    else {
        variorum_call();
    }	
}

void kokkosp_begin_parallel_reduce(const char* name, const uint32_t devID, uint64_t* kID) {
    std::cout << "Device ID: " << devID << "\n";
    if(usingMPI) {
        variorum_call_mpi();
    }
    else {
        variorum_call();
    }
}

void kokkosp_end_parallel_reduce(const uint64_t kID) {
    if(usingMPI) {
        variorum_call_mpi();
    }
    else {
        variorum_call();
    }
}

Kokkos::Tools::Experimental::EventSet get_event_set() {
    Kokkos::Tools::Experimental::EventSet my_event_set;
    memset(&my_event_set, 0, sizeof(my_event_set)); // zero any pointers not set here
    my_event_set.init = kokkosp_init_library;
    my_event_set.finalize = kokkosp_finalize_library;
    my_event_set.begin_parallel_for = kokkosp_begin_parallel_for;
    my_event_set.begin_parallel_reduce = kokkosp_begin_parallel_reduce;
    my_event_set.begin_parallel_scan = kokkosp_begin_parallel_scan;
    my_event_set.end_parallel_for = kokkosp_end_parallel_for;
    my_event_set.end_parallel_reduce = kokkosp_end_parallel_reduce;
    my_event_set.end_parallel_scan = kokkosp_end_parallel_scan;
    return my_event_set;
}

}} // namespace KokkosTools::VariorumConnector

extern "C" {

namespace impl = KokkosTools::VariorumConnector;

EXPOSE_INIT(impl::kokkosp_init_library)
EXPOSE_FINALIZE(impl::kokkosp_finalize_library)
EXPOSE_BEGIN_PARALLEL_FOR(impl::kokkosp_begin_parallel_for)
EXPOSE_END_PARALLEL_FOR(impl::kokkosp_end_parallel_for)
EXPOSE_BEGIN_PARALLEL_SCAN(impl::kokkosp_begin_parallel_scan)
EXPOSE_END_PARALLEL_SCAN(impl::kokkosp_end_parallel_scan)
EXPOSE_BEGIN_PARALLEL_REDUCE(impl::kokkosp_begin_parallel_reduce)
EXPOSE_END_PARALLEL_REDUCE(impl::kokkosp_end_parallel_reduce)

} // extern "C"