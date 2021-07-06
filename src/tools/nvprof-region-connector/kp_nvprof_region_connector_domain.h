
#ifndef _H_KOKKOSP_KERNEL_NVPROF_CONNECTOR_INFO
#define _H_KOKKOSP_KERNEL_NVPROF_CONNECTOR_INFO

#include <stdio.h>
#include <sys/time.h>
#include <cstring>

#include "nvToolsExt.h"

class toolsNVProfRegionConnectorInfo {
	public:
		toolsNVProfRegionConnectorInfo(std::string rName) : regionFilter(1), curRegCnt(0), bActive(false) {

		  domainNameHandle = rName;
		  char* domainName = (char*) malloc( sizeof(char*) * (rName.size()+8) );
		  sprintf(domainName, "Region.%s", rName.c_str());
		  domain = nvtxDomainCreateA(domainName);
		  currentRange = 0;
		}

		nvtxRangeId_t startRange() {
		  nvtxEventAttributes_t eventAttrib = {0};
		  eventAttrib.version = NVTX_VERSION;
		  eventAttrib.size = NVTX_EVENT_ATTRIB_STRUCT_SIZE;
		  eventAttrib.messageType = NVTX_MESSAGE_TYPE_ASCII;
		  eventAttrib.message.ascii = "my range";
		  currentRange = nvtxDomainRangeStartEx(domain,&eventAttrib);
                  bActive = true;
		  return currentRange;
		}

		nvtxRangeId_t getCurrentRange() {
		  return currentRange;
		}

		void endRange() {
		  nvtxDomainRangeEnd(domain,currentRange);
                  bActive = false;
		}

		nvtxDomainHandle_t getDomain() {
			return domain;
		}

		std::string getDomainNameHandle() {
			return domainNameHandle;
		}

		~toolsNVProfRegionConnectorInfo() {
		  nvtxDomainDestroy(domain);
		}

        bool enterRegion() {
           if (regionFilter == 0)
              return false;

           curRegCnt = ( curRegCnt >= INT_MAX ) ? 0 : curRegCnt+1;
           if ((curRegCnt)%regionFilter==0)
              return true;

           return false;
        }
        bool isActive() 
        {
           return bActive;
        }

        void setRegFilter(int flt) { regionFilter = flt; }
        int getCurRegCnt() { return curRegCnt; }
        std::string getName() { return domainNameHandle; }
	private:
                bool bActive;
                int curRegCnt;
                int regionFilter;
		std::string domainNameHandle;
		nvtxRangeId_t currentRange;
		nvtxDomainHandle_t domain;
};

#endif
