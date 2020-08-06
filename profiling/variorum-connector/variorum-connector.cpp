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

//variables for simple timer
time_t start_time;


bool kokkospFilterMatch(const char* name) {
	bool matched = false;
	std::string nameStr(name);

	for(auto nextRegex : kernelNames) {
		if(std::regex_match(nameStr, nextRegex)) {
			matched = true;
			break;
		}
	}

	return matched;
}

bool kokkospReadLine(FILE* kernelFile, char* lineBuffer) {
	bool readData = false;
	bool continueReading = !feof(kernelFile);
	int nextIndex = 0;

	while(continueReading) {
		const int nextChar = fgetc(kernelFile);

		if(EOF == nextChar) {
			continueReading = false;
		} else {
			readData = true;

			if(nextChar == '\n' || nextChar == '\f' || nextChar == '\r') {
				continueReading = false;
			} else {
				lineBuffer[nextIndex++] = (char) nextChar;
			}
		}
	}

	lineBuffer[nextIndex] = '\0';
	return readData;
}

extern "C" void kokkosp_init_library(const int loadSeq,
	const uint64_t interfaceVer, const uint32_t devInfoCount, void* deviceInfo) {
        int ret;
        //Zach Frye Test code begins here 
        time(&start_time);
        std::cout << "Start Time: " << start_time << std::endl;
        //variourm monitoring code
        //FILE *output = fopen("out.txt", "w");
        //ret = variorum_monitoring(output);
        
	const char* kernelFilterPath = getenv("KOKKOSP_KERNEL_FILTER");
	nextKernelID = 0;

	if(NULL == kernelFilterPath) {
		filterKernels = false;
		printf("============================================================\n");
		printf("KokkosP: No kernel filtering data provided, disabled from this tool onwards\n");
		printf("============================================================\n");
	} else {
		printf("============================================================\n");
		printf("KokkosP: Filter File: %s\n", kernelFilterPath);
		printf("============================================================\n");   



		FILE* kernelFilterFile = fopen(kernelFilterPath, "rt");

		if(NULL == kernelFilterFile) {
			fprintf(stderr, "Unable to open kernel filter: %s\n",
				kernelFilterPath);
			exit(-1);
		} else {
			char* lineBuffer = (char*) malloc( sizeof(char) * 65536 );

			while(kokkospReadLine(kernelFilterFile, lineBuffer)) {
				printf("KokkosP: Filter [%s]\n", lineBuffer);

				std::regex nextRegEx(lineBuffer, std::regex::optimize);
				kernelNames.push_back(nextRegEx);
			}

			free(lineBuffer);
		}

		filterKernels = (kernelNames.size() > 0);

		printf("KokkosP: Kernel Filtering is %s\n", (filterKernels ?
			"enabled" : "disabled"));

		if(filterKernels) {
			char* profileLibrary = getenv("KOKKOS_PROFILE_LIBRARY");
			char* envBuffer = (char*) malloc( sizeof(char) * (strlen(profileLibrary) + 1 ));
			sprintf(envBuffer, "%s", profileLibrary);

			char* nextLibrary = strtok(envBuffer, ";");

			for(int i = 0; i < loadSeq; i++) {
				nextLibrary = strtok(NULL, ";");
			}

			nextLibrary = strtok(NULL, ";");

			if(NULL == nextLibrary) {
				printf("KokkosP: No child library to call in %s\n", profileLibrary);
			} else {
				printf("KokkosP: Next library to call: %s\n", nextLibrary);
				printf("KokkosP: Loading child library ..\n");

				void* childLibrary = dlopen(nextLibrary, RTLD_NOW | RTLD_GLOBAL);

				if(NULL == childLibrary) {
					fprintf(stderr, "KokkosP: Error: Unable to load: %s (Error=%s)\n",
						nextLibrary, dlerror());
				} else {
					beginForCallee = (beginFunction) dlsym(childLibrary, "kokkosp_begin_parallel_for");
	               	beginScanCallee = (beginFunction) dlsym(childLibrary, "kokkosp_begin_parallel_scan");
	               	beginReduceCallee = (beginFunction) dlsym(childLibrary, "kokkosp_begin_parallel_reduce");

        	        endScanCallee = (endFunction) dlsym(childLibrary, "kokkosp_end_parallel_scan");
       	        	endForCallee = (endFunction) dlsym(childLibrary, "kokkosp_end_parallel_for");
    	           	endReduceCallee = (endFunction) dlsym(childLibrary, "kokkosp_end_parallel_reduce");

   	            	initProfileLibrary = (initFunction) dlsym(childLibrary, "kokkosp_init_library");
	               	finalizeProfileLibrary = (finalizeFunction) dlsym(childLibrary, "kokkosp_finalize_library");

					if(NULL != initProfileLibrary) {
						(*initProfileLibrary)(loadSeq + 1, interfaceVer, devInfoCount, deviceInfo);
					}
				}

			}

			free(envBuffer);
		}

		printf("============================================================\n");
	}
}

extern "C" void kokkosp_finalize_library() {
	if(NULL != finalizeProfileLibrary) {
		(*finalizeProfileLibrary)();
	}

    // Set all profile hooks to NULL to prevent
  	// any additional calls. Once we are told to
    // finalize, we mean it
    beginForCallee = NULL;
    beginScanCallee = NULL;
    beginReduceCallee = NULL;
    endScanCallee = NULL;
    endForCallee = NULL;
    endReduceCallee = NULL;
    initProfileLibrary = NULL;
    finalizeProfileLibrary = NULL;

    time_t total_time;
    time_t end_time;
    time(&end_time);
    std::cout << "End Time: " << end_time << "\nStart Time: " << start_time << std::endl;
    total_time = end_time - start_time;

    std::cout << "The kokkos library was alive for " << total_time << " seconds." << std::endl;

	printf("============================================================\n");
	printf("KokkosP: Kernel filtering library, finalized.\n");
	printf("============================================================\n");
}

extern "C" void kokkosp_begin_parallel_for(const char* name, const uint32_t devID, uint64_t* kID) {
    int ret;

    //print variorum power whenever parallel for is called
	ret = variorum_print_power();
    if (ret != 0) {
        printf("Print power failed!\n");
    }
    
    if(filterKernels) {
		if(kokkospFilterMatch(name)) {
			if(NULL != beginForCallee) {
				(*beginForCallee)(name, devID, kID);
				activeKernels.insert(*kID);
			} else {
				*kID = nextKernelID++;
			}
		} else {
			*kID = nextKernelID++;
		}
	} else {
		if(NULL != beginForCallee) {
        	(*beginForCallee)(name, devID, kID);
			activeKernels.insert(*kID);
        } else {
			*kID = nextKernelID++;
		}
	}
}

extern "C" void kokkosp_end_parallel_for(const uint64_t kID) {
	auto findKernel = activeKernels.find(kID);
    //print variorum power ever time a parallel for has finished executing 
    int ret;
	ret = variorum_print_power();
    if (ret != 0) {
        printf("Print power failed!\n");
    }
	if(activeKernels.end() != findKernel) {
		if(NULL != endForCallee) {
			(*endForCallee)(kID);
		}

		activeKernels.erase(findKernel);
	}
}

extern "C" void kokkosp_begin_parallel_scan(const char* name, const uint32_t devID, uint64_t* kID) {
	    int ret;

    //print variorum power when parallel scan begins 
	ret = variorum_print_power();
    if (ret != 0) {
        printf("Print power failed!\n");
    }
    
    if(filterKernels) {
		if(kokkospFilterMatch(name)) {
			if(NULL != beginScanCallee) {
				(*beginScanCallee)(name, devID, kID);
				activeKernels.insert(*kID);
			} else {
				*kID = nextKernelID++;
			}
		} else {
			*kID = nextKernelID++;
		}
	} else {
		if(NULL != beginScanCallee) {
        	(*beginScanCallee)(name, devID, kID);
			activeKernels.insert(*kID);
        } else {
			*kID = nextKernelID++;
		}
	}
}

extern "C" void kokkosp_end_parallel_scan(const uint64_t kID) {

    //print power when a parallel scan completes
	auto findKernel = activeKernels.find(kID);
        int ret;
	ret = variorum_print_power();
    if (ret != 0) {
        printf("Print power failed!\n");
    }
	if(activeKernels.end() != findKernel) {
		if(NULL != endScanCallee) {
			(*endScanCallee)(kID);
		}

		activeKernels.erase(findKernel);
	}
}

extern "C" void kokkosp_begin_parallel_reduce(const char* name, const uint32_t devID, uint64_t* kID) {
    int ret;

    //print power when a parallel reduce begins
	ret = variorum_print_power();
    if (ret != 0) {
        printf("Print power failed!\n");
    }

	if(filterKernels) {
        if(kokkospFilterMatch(name)) {
            if(NULL != beginScanCallee) {
               	(*beginReduceCallee)(name, devID, kID);
				activeKernels.insert(*kID);
            } else {
				*kID = nextKernelID++;
			}
		} else {
			*kID = nextKernelID++;
		}
    } else {
       	if(NULL != beginScanCallee) {
       	    (*beginReduceCallee)(name, devID, kID);
			activeKernels.insert(*kID);
       	} else {
			*kID = nextKernelID++;
		}
    }
}

extern "C" void kokkosp_end_parallel_reduce(const uint64_t kID) {
	auto findKernel = activeKernels.find(kID);
    int ret;

    //print power when a parallel reduce completes
	ret = variorum_print_power();
    if (ret != 0) {
        printf("Print power failed!\n");
    }
	if(activeKernels.end() != findKernel) {
		if(NULL != endReduceCallee) {
			(*endReduceCallee)(kID);
		}

		activeKernels.erase(findKernel);
	}
}

