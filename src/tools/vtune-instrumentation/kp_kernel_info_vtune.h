
#ifndef _H_KOKKOSP_KERNEL_INFO_VTUNE
#define _H_KOKKOSP_KERNEL_INFO_VTUNE

#include <stdio.h>
#include <sys/time.h>
#include <cstring>
#include "kp_kernel_info.h"
#include "ittnotify.h"

class KernelPerformanceInfoVtune:public KernelPerformanceInfo {
	public:
		KernelPerformanceInfoVtune(std::string kName, KernelExecutionType kernelType):
                    KernelPerformanceInfo(kName,kernelType) {
                  itt_handle = __itt_domain_create(kName.c_str());
                }

		~KernelPerformanceInfoVtune() {
		}
                void start_frame() {
                  __itt_frame_begin_v3(itt_handle, NULL);
                }

                void end_frame() {
                  __itt_frame_end_v3(itt_handle, NULL);
                }
	private:
           __itt_domain* itt_handle;
};

#endif
