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

#ifndef KOKKOS_PROFILING_C_INTERFACE_HPP
#define KOKKOS_PROFILING_C_INTERFACE_HPP

#ifdef __cplusplus
#include <cstddef>
#include <cstdint>
#else
#include <stddef.h>
#include <stdint.h>
#endif

#define KOKKOSP_INTERFACE_VERSION 20191080

struct Kokkos_Profiling_KokkosPDeviceInfo {
  size_t deviceID;
};

struct Kokkos_Profiling_SpaceHandle {
  const char* name;
};

struct Kokkos_Tuning_VariableValue;  // forward declaration

struct Kokkos_Tuning_ValueSet {
  size_t id;
  size_t size;
  struct Kokkos_Tuning_VariableValue* values;
};

struct Kokkos_Tuning_ValueRange {
  size_t id;
  Kokkos_Tuning_VariableValue*
      lower;  // pointer because I'm used in Kokkos_VariableValue_ValueUnion,
              // defined below
  Kokkos_Tuning_VariableValue* upper;
  Kokkos_Tuning_VariableValue* step;
  bool openLower;
  bool openUpper;
};

union Kokkos_Tuning_VariableValue_ValueUnion {
  bool bool_value;
  int int_value;
  double double_value;
  const char* string_value;
  Kokkos_Tuning_ValueRange range_value;
  Kokkos_Tuning_ValueSet set_value;
};

struct Kokkos_Tuning_VariableValue {
  size_t id;
  union Kokkos_Tuning_VariableValue_ValueUnion value;
};

enum Kokkos_Tuning_VariableInfo_ValueType {
  kokkos_value_floating_point,  // TODO DZP: single and double? One or the
  kokkos_value_integer,
  kokkos_value_text,
  kokkos_value_boolean,
  kokkos_value_floating_point_tuple,
  kokkos_value_integer_tuple,
  kokkos_value_text_tuple,
  kokkos_value_boolean_tuple,
  kokkos_value_floating_point_range,  // TODO DZP: prove to myself that a
                                      // boolean or text range is stupid and
                                      // shouldn't exist
  kokkos_value_integer_range,
};

enum Kokkos_Tuning_VariableInfo_StatisticalCategory {
  kokkos_value_categorical,  // unordered distinct objects
  kokkos_value_ordinal,      // ordered distinct objects
  kokkos_value_interval,  // ordered distinct objects for which distance matters
  kokkos_value_ratio  // ordered distinct objects for which distance matters,
                      // division matters, and the concept of zero exists
};

enum Kokkos_Tuning_VariableInfo_CandidateValueType {
  kokkos_value_set,       // I am one of [2,3,4,5]
  kokkos_value_range,     // I am somewhere in [2,12)
  kokkos_value_unbounded  // I am [text/int/float], but we don't know at
                          // declaration time what values are appropriate. Only
                          // valid for Context Variables
  // TODO DZP: not handled: 1 + 3x, sets of ranges, range with hole (zero). Do
  // these matter?
};

union Kokkos_Tuning_VariableInfo_SetOrRange {
  struct Kokkos_Tuning_ValueSet set;
  struct Kokkos_Tuning_ValueRange range;
};

struct Kokkos_Tuning_VariableInfo {
  enum Kokkos_Tuning_VariableInfo_ValueType type;
  enum Kokkos_Tuning_VariableInfo_StatisticalCategory category;
  enum Kokkos_Tuning_VariableInfo_CandidateValueType valueQuantity;
};

typedef void (*Kokkos_Profiling_initFunction)(
    const int, const uint64_t, const uint32_t,
    Kokkos_Profiling_KokkosPDeviceInfo*);
typedef void (*Kokkos_Profiling_finalizeFunction)();
typedef void (*Kokkos_Profiling_beginFunction)(const char*, const uint32_t,
                                               uint64_t*);
typedef void (*Kokkos_Profiling_endFunction)(uint64_t);

typedef void (*Kokkos_Profiling_pushFunction)(const char*);
typedef void (*Kokkos_Profiling_popFunction)();

typedef void (*Kokkos_Profiling_allocateDataFunction)(
    const Kokkos_Profiling_SpaceHandle, const char*, const void*,
    const uint64_t);
typedef void (*Kokkos_Profiling_deallocateDataFunction)(
    const Kokkos_Profiling_SpaceHandle, const char*, const void*,
    const uint64_t);

typedef void (*Kokkos_Profiling_createProfileSectionFunction)(const char*,
                                                              uint32_t*);
typedef void (*Kokkos_Profiling_startProfileSectionFunction)(const uint32_t);
typedef void (*Kokkos_Profiling_stopProfileSectionFunction)(const uint32_t);
typedef void (*Kokkos_Profiling_destroyProfileSectionFunction)(const uint32_t);

typedef void (*Kokkos_Profiling_profileEventFunction)(const char*);

typedef void (*Kokkos_Profiling_beginDeepCopyFunction)(
    Kokkos_Profiling_SpaceHandle, const char*, const void*,
    Kokkos_Profiling_SpaceHandle, const char*, const void*, uint64_t);
typedef void (*Kokkos_Profiling_endDeepCopyFunction)();

typedef void (*Kokkos_Tuning_tuningVariableDeclarationFunction)(
    const char*, const size_t, Kokkos_Tuning_VariableInfo info);
typedef void (*Kokkos_Tuning_contextVariableDeclarationFunction)(
    const char*, const size_t, Kokkos_Tuning_VariableInfo info,
    Kokkos_Tuning_VariableInfo_SetOrRange);
typedef void (*Kokkos_Tuning_tuningVariableValueFunction)(
    const size_t, const size_t, const size_t*,
    const Kokkos_Tuning_VariableValue*, const size_t count,
    const size_t* uniqIds, Kokkos_Tuning_VariableValue*,
    Kokkos_Tuning_VariableInfo_SetOrRange*);
typedef void (*Kokkos_Tuning_contextVariableValueFunction)(
    const size_t contextId, const size_t count, const size_t* uniqIds,
    Kokkos_Tuning_VariableValue* values);
typedef void (*Kokkos_Tuning_contextEndFunction)(const size_t);

#endif  // KOKKOS_PROFILING_C_INTERFACE_HPP