
#include <stdio.h>
#include <inttypes.h>
#include <cstdlib>
#include <cstring>
#include <unordered_map>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cxxabi.h>
#include <map>
#include <stack>
#include <cuda_profiler_api.h>
#include "kp_nvprof_region_connector_domain.h"


static toolsNVProfRegionConnectorInfo* currentRegion;
static std::unordered_map<std::string, toolsNVProfRegionConnectorInfo*> domain_map;
static uint64_t nextRegionID;
static std::map<std::string, int> regionMap;
static std::stack<std::string> regionStack;
static int sActive = false;

extern "C" void kokkosp_init_library(const int loadSeq,
	const uint64_t interfaceVer,
	const uint32_t devInfoCount,
	void* deviceInfo) {

	printf("-----------------------------------------------------------\n");
	printf("KokkosP: NVProf Analyzer Region Connector (sequence is %d, version: %llu)\n", loadSeq, interfaceVer);
	printf("-----------------------------------------------------------\n");

        const char * region_config = std::getenv("KOKKOS_REGION_CONFIG");
        std::string regionFileName = region_config;
      
        std::ifstream fRegConf;
        fRegConf.open(regionFileName);
        if (fRegConf.is_open())
        {
           std::string sLine;
           while( std::getline(fRegConf,sLine))
           {
               std::string regName;
               std::string regFilter; 
               std::stringstream ss(sLine);
               if (std::getline(ss,regName,'='))
               {
                   if (std::getline(ss,regFilter,'='))
                   {
                      int filter = -1;
                      sscanf(regFilter.c_str(), "%d", &filter);
                      if (filter >= 0)
                      {
                          regionMap[regName] = filter;  
                          printf("Adding region to map [%s, %d] \n", regName.c_str(), filter);
                      }
                   }
               }
           }
        }
        cudaProfilerStart();
        cudaProfilerStop();
	nextRegionID = 0;
}

toolsNVProfRegionConnectorInfo* getRegionConnectorInfo(const char* name) {
	std::string nameStr(name);
	auto kDomain = domain_map.find(nameStr);
	currentRegion = NULL;

	if(kDomain == domain_map.end()) {
		currentRegion = new toolsNVProfRegionConnectorInfo(name);
                auto check_map = regionMap.find((std::string)name);
                if (check_map != regionMap.end())
                {
                   currentRegion->setRegFilter( regionMap[name] );
                }
		domain_map.insert(std::pair<std::string, toolsNVProfRegionConnectorInfo*>(nameStr,
			currentRegion));
	} else {
		currentRegion = kDomain->second;
	}

	return currentRegion;
}

void regionConnectorExecuteStart() {
  regionStack.push(currentRegion->getName());
  bool bStart = currentRegion->enterRegion();
  if (!sActive && bStart)
  {
     //printf("staring profilier: %s \n", currentRegion->getName().c_str());
     cudaProfilerStart();
     sActive = true;
  }
  
  if (bStart)
  {
     //printf("tracking region: %s \n", currentRegion->getName().c_str());
     currentRegion->startRange();
  }
  else
  {
     //printf("skipping region: %s \n", currentRegion->getName().c_str());
  }
}

void regionConnectorExecuteEnd() {
  if (regionStack.size() > 0)
      regionStack.pop();
  
  if ( sActive && currentRegion->isActive() )
  {
     //printf("exit region: %s \n", currentRegion->getName().c_str());
     currentRegion->endRange();
  }

  if (regionStack.size() > 0)
  {
     currentRegion = getRegionConnectorInfo(((std::string)regionStack.top()).c_str());
  }
  else
  {
     currentRegion = NULL;
  }
  if (sActive && ( currentRegion == NULL || !currentRegion->isActive()) )
  {
     //printf("stop profiling \n");
     cudaProfilerStop();
     sActive = false;
  }
}

extern "C" void kokkosp_finalize_library() {
	printf("-----------------------------------------------------------\n");
	printf("KokkosP: Finalization of NVProf Region Connector. Complete.\n");
	printf("-----------------------------------------------------------\n");

}


extern "C" void kokkosp_push_profile_region(const char* name) {
	currentRegion = getRegionConnectorInfo(name);
	regionConnectorExecuteStart();
}

extern "C" void kokkosp_pop_profile_region() {
	regionConnectorExecuteEnd();
}
