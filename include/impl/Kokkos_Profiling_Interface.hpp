/*
 //@HEADER
 // ************************************************************************
 //
 //                        Kokkos v. 2.0
 //              Copyright (2014) Sandia Corporation
 //
 // Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
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
 // THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
 // EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 // IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 // PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
 // CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 // EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 // PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 // PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 // LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 // NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 // SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 //
 // Questions? Contact Christian R. Trott (crtrott@sandia.gov)
 //
 // ************************************************************************
 //@HEADER
 */

#ifndef KOKKOSP_INTERFACE_HPP
#define KOKKOSP_INTERFACE_HPP

#include <cinttypes>
#include <cstddef>
#include <string>

#include <iostream>
#include <cstdlib>

#include <impl/Kokkos_Profiling_DeviceInfo.hpp>
#include <impl/Kokkos_Profiling_DeviceInfo.hpp>
#include <impl/Kokkos_Profiling_C_Interface.h>
#include <impl/Kokkos_Profiling_C_Interface.h>

namespace Kokkos {
namespace Profiling {

using SpaceHandle = Kokkos_Profiling_SpaceHandle;

}  // end namespace Profiling

namespace Tuning {

using ValueSet            = Kokkos_Tuning_ValueSet;
using ValueRange          = Kokkos_Tuning_ValueRange;
using StatisticalCategory = Kokkos_Tuning_VariableInfo_StatisticalCategory;
using ValueType           = Kokkos_Tuning_VariableInfo_ValueType;
using CandidateValueType  = Kokkos_Tuning_VariableInfo_CandidateValueType;
using SetOrRange          = Kokkos_Tuning_VariableInfo_SetOrRange;
using VariableInfo        = Kokkos_Tuning_VariableInfo;
// TODO DZP: VariableInfo subclasses to automate some of this

using VariableValue = Kokkos_Tuning_VariableValue;

VariableValue make_variable_value(size_t id, bool val);
VariableValue make_variable_value(size_t id, int val);
VariableValue make_variable_value(size_t id, double val);
VariableValue make_variable_value(size_t id, const char* val);

}  // end namespace Tuning

namespace Profiling {

using initFunction           = Kokkos_Profiling_initFunction;
using finalizeFunction       = Kokkos_Profiling_finalizeFunction;
using beginFunction          = Kokkos_Profiling_beginFunction;
using endFunction            = Kokkos_Profiling_endFunction;
using pushFunction           = Kokkos_Profiling_pushFunction;
using popFunction            = Kokkos_Profiling_popFunction;
using allocateDataFunction   = Kokkos_Profiling_allocateDataFunction;
using deallocateDataFunction = Kokkos_Profiling_deallocateDataFunction;
using createProfileSectionFunction =
    Kokkos_Profiling_createProfileSectionFunction;
using startProfileSectionFunction =
    Kokkos_Profiling_startProfileSectionFunction;
using stopProfileSectionFunction = Kokkos_Profiling_stopProfileSectionFunction;
using destroyProfileSectionFunction =
    Kokkos_Profiling_destroyProfileSectionFunction;
using profileEventFunction  = Kokkos_Profiling_profileEventFunction;
using beginDeepCopyFunction = Kokkos_Profiling_beginDeepCopyFunction;
using endDeepCopyFunction   = Kokkos_Profiling_endDeepCopyFunction;

}  // end namespace Profiling

namespace Tuning {
using tuningVariableDeclarationFunction =
    Kokkos_Tuning_tuningVariableDeclarationFunction;
using contextVariableDeclarationFunction =
    Kokkos_Tuning_contextVariableDeclarationFunction;
using tuningVariableValueFunction  = Kokkos_Tuning_tuningVariableValueFunction;
using contextVariableValueFunction = Kokkos_Tuning_contextVariableValueFunction;
using contextEndFunction           = Kokkos_Tuning_contextEndFunction;

}  // end namespace Tuning

}  // namespace Kokkos

#endif
