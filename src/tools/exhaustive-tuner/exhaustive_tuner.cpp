
#include <cassert>
#include <cinttypes>
#include <cstdio>
#include <cstring>

#include <map>
#include <queue>
#include <set>
#include <string>
#include <vector>

#include <impl/Kokkos_Profiling_Interface.hpp>

template <typename... Args>
using MapType = std::map<Args...>;
template <typename... Args>
using SetType = std::set<Args...>;

static MapType<size_t, Kokkos::Tuning::ValueType> var_info;

namespace std {
template <>
struct less<Kokkos::Tuning::VariableValue> {
        bool operator()(const Kokkos::Tuning::VariableValue& l,
                        const Kokkos::Tuning::VariableValue& r) {
                assert(var_info[l.id] == var_info[r.id]);
                switch (var_info[l.id]) {
                        case kokkos_value_boolean:
                                return l.value.bool_value < r.value.bool_value;
                        case kokkos_value_integer:
                                return l.value.int_value < r.value.int_value;
                        case kokkos_value_floating_point:
                                return l.value.double_value <
                                       r.value.double_value;
                        case kokkos_value_text:
                                return strncmp(l.value.string_value,
                                               r.value.string_value, 1024);
                }
        }
};
}  // namespace std

std::vector<std::string> regions;
static uint64_t uniqID;

extern "C" void kokkosp_init_library(const int loadSeq,
                                     const uint64_t interfaceVer,
                                     const uint32_t devInfoCount,
                                     void* deviceInfo) {
        printf(
            "KokkosP: Example Tuner Library Initialized (sequence is %d, "
            "version: %lu)\n",
            loadSeq, interfaceVer);
        uniqID = 0;
}

extern "C" void kokkosp_finalize_library() {
        printf("KokkosP: Kokkos library finalization called.\n");
}

extern "C" void kokkosp_begin_parallel_for(const char* name,
                                           const uint32_t devID,
                                           uint64_t* kID) {}

extern "C" void kokkosp_end_parallel_for(const uint64_t kID) {}

extern "C" void kokkosp_begin_parallel_scan(const char* name,
                                            const uint32_t devID,
                                            uint64_t* kID) {}

extern "C" void kokkosp_end_parallel_scan(const uint64_t kID) {}

extern "C" void kokkosp_begin_parallel_reduce(const char* name,
                                              const uint32_t devID,
                                              uint64_t* kID) {}

extern "C" void kokkosp_end_parallel_reduce(const uint64_t kID) {}

extern "C" void kokkosp_push_profile_region(char* regionName) {}

extern "C" void kokkosp_pop_profile_region() {}

extern "C" void kokkosp_allocate_data(Kokkos::Profiling::SpaceHandle handle,
                                      const char* name, void* ptr,
                                      uint64_t size) {}

extern "C" void kokkosp_deallocate_data(Kokkos::Profiling::SpaceHandle handle,
                                        const char* name, void* ptr,
                                        uint64_t size) {}

extern "C" void kokkosp_begin_deep_copy(
    Kokkos::Profiling::SpaceHandle dst_handle, const char* dst_name,
    const void* dst_ptr, Kokkos::Profiling::SpaceHandle src_handle,
    const char* src_name, const void* src_ptr, uint64_t size) {}

std::string getName(Kokkos_Tuning_VariableValue in) {
        size_t id = in.id;
        Kokkos::Tuning::ValueType type = var_info[id];
        if (type == kokkos_value_boolean) {
                return "Boolean";
        } else if (type == kokkos_value_integer) {
                return std::to_string(in.value.int_value);
        } else if (type == kokkos_value_floating_point) {
                return std::to_string(in.value.double_value);
        } else if (type == kokkos_value_text) {
                return in.value.string_value;
        }
        return "";  // TODO DZP: some kind of "unreachable." Also booleans.
}

#define FEATURE_ID_BITWIDTH (5)
#define MAX_FEATURE_ID (2 << FEATURE_ID_BITWIDTH)

/** Notes
 *
 * I am tuning these N parameters based on these X features
 *
 * First, hash all N's ID's
 *
 * Now I'm looking at feature values (will the features always be the same?
 * Assume no)
 *
 * For each set of feature values, I have a set of performance results for the
 * values of N
 *
 * Store those performance results in a priority_queue sorted by lowest number
 * of encounters
 *
 * If the lowest is equal to the threshold, I'm done, use the fastest, otherwise
 * use the lowest number of encounters
 *
 */

struct FeatureSet {
        size_t count;
        const size_t* ids;
        bool operator<(const FeatureSet& other) const {
                if (count < other.count) {
                        return true;
                } else if (count > other.count) {
                        return false;
                } else {
                        for (int x = 0; x < count; ++x) {
                                if (ids[x] < other.ids[x]) {
                                        return true;
                                }
                                if (ids[x] > other.ids[x]) {
                                        return false;
                                }
                        }
                }
                return false;
        }
};

using TuningParameterSet = FeatureSet;

struct TuningParameterValues {
        size_t count;
        Kokkos::Tuning::VariableValue* values;
        int times_encountered;
        double time_value;
        bool operator<(const TuningParameterValues& r) const {
                return times_encountered < r.times_encountered;
        }
};

bool Kokkos_Value_Less(const size_t l_id,
                       const Kokkos::Tuning::VariableValue& l,
                       const size_t r_id,
                       const Kokkos::Tuning::VariableValue& r) {
        assert(var_info[l_id] == var_info[r_id]);
        switch (var_info[l_id]) {
                case kokkos_value_boolean:  // TODO: should we allow boolean
                                            // less?
                        return l.value.bool_value < r.value.bool_value;
                case kokkos_value_integer:
                        return l.value.int_value < r.value.int_value;
                case kokkos_value_floating_point:
                        return l.value.double_value < r.value.double_value;
                case kokkos_value_text:
                        return strncmp(l.value.string_value,
                                       r.value.string_value, 1024);
        }
}

struct FeatureValues {
        const size_t count;
        const size_t* ids;
        Kokkos::Tuning::VariableValue* values;
        bool done;
        TuningParameterValues ideal;
        bool operator<(const FeatureValues& other) const {
                if (count < other.count) {
                        return true;
                } else if (count > other.count) {
                        return false;
                } else {
                        for (int x = 0; x < count; ++x) {
                                if (Kokkos_Value_Less(ids[x], values[x],
                                                      other.ids[x],
                                                      other.values[x])) {
                                        return true;
                                } else if (Kokkos_Value_Less(
                                               other.ids[x], other.values[x],
                                               ids[x], values[x])) {
                                        return false;
                                }
                        }
                }
                return false;
        }
};

MapType<
    TuningParameterSet,
    MapType<FeatureSet,
            MapType<FeatureValues, std::priority_queue<TuningParameterValues>>>>
    performance_data;

MapType<size_t, SetType<Kokkos::Tuning::VariableValue>> candidate_values;
MapType<size_t, bool> candidate_is_set;  // TODO DZP: rewrite all these to be
                                         // one size_t -> VariableInfo map

Kokkos::Tuning::SetOrRange copy_candidate_values(
    const Kokkos::Tuning::SetOrRange& in, bool isSet) {
        Kokkos::Tuning::SetOrRange ret;
        if (isSet) {
                ret.set.size = in.set.size;
                ret.set.values =
                    new Kokkos::Tuning::VariableValue[ret.set.size];
                std::copy(in.set.values, in.set.values + ret.set.size,
                          ret.set.values);
        } else {
                ret.range = in.range;
        }
        return ret;
}

extern "C" void kokkosp_declare_tuning_variable(
    const char* name, const size_t id, Kokkos::Tuning::VariableInfo info) {
        printf("New variable named %s with id %zu\n", name, id);
        var_info[id] = info.type;
        candidate_is_set[id] = info.valueQuantity == kokkos_value_set;
        // candidate_values[id] = copy_candidate_values(candidate_value,
        // (info.valueQuantity == kokkos_value_set));
}
extern "C" void kokkosp_declare_context_variable(
    const char* name, const size_t id, Kokkos::Tuning::VariableInfo info,
    Kokkos::Tuning::SetOrRange candidate_value) {
        printf("New variable named %s with id %zu\n", name, id);
        var_info[id] = info.type;
}

template <typename... Args>
using VectorType = std::vector<Args...>;

using WorkingSet = VectorType<VectorType<Kokkos::Tuning::VariableValue>>;

WorkingSet make_tuning_set_impl(WorkingSet& in, WorkingSet& add,
                                int debug = 0) {
        for (int x = debug * 2; x > 0; --x) {
                std::cout << " ";
        }
        if (add.empty()) {
                std::cout << "[mtsi] add_nothing {in.size=" << in.size()
                          << "}\n";
                return in;
        }
        if (in.empty()) {
                std::vector<std::vector<Kokkos::Tuning::VariableValue>>
                    start_set = {*add.erase(add.begin())};
                std::cout << "[mtsi] in_nothing {start_set.size="<<start_set.size()<<"}\n";
                // int foo = *start_set;
                const auto& x = make_tuning_set_impl(start_set, add, debug + 1);
                for (int x = debug * 2; x > 0; --x) {
                        std::cout << " ";
                }
                std::cout << "[mtsi] in_nothing {return_size=" << x.size()
                          << "}\n";

                return x;
        }
        std::cout << "[mtsi] merging\n";
        WorkingSet working;
        std::vector<Kokkos::Tuning::VariableValue>& features =
            *add.erase(add.begin());
        for (auto& vec : in) {
                for (auto& feature : features) {
                        std::vector<Kokkos::Tuning::VariableValue> build_me{
                            feature};
                        std::copy(vec.begin(), vec.end(), build_me.begin());
                        working.push_back(std::move(build_me));
                }
        }
        return make_tuning_set_impl(working, add, debug + 1);
}

constexpr int num_samples = 5;

// TODO DZP: this function is much truncated, is it still necessary?
std::vector<Kokkos::Tuning::VariableValue> make_feature_vector(
    const SetType<Kokkos::Tuning::VariableValue>& in) {
        std::vector<Kokkos::Tuning::VariableValue> ret;
        std::copy(in.begin(), in.end(), std::back_inserter(ret));
        return ret;
}

TuningParameterValues values_from_vector(
    std::vector<Kokkos::Tuning::VariableValue> in);

TuningParameterValues makeTuningParameters(
    const std::vector<Kokkos::Tuning::VariableValue>& in) {
        TuningParameterValues ret;
        ret.count = in.size();
        ret.time_value = 0.0;
        ret.times_encountered = 0;
        ret.values = new Kokkos::Tuning::VariableValue[ret.count];

        std::copy(in.begin(), in.end(), ret.values);
        return ret;
}

std::priority_queue<TuningParameterValues> make_tuning_set(const size_t count,
                                                           const size_t* ids) {
        std::priority_queue<TuningParameterValues> ret;
        WorkingSet in;
        for (auto x = 0; x < count; ++x) {
                auto candidate_values_for_feature = candidate_values[ids[x]];
                int debug =0;
                for(auto item: candidate_values_for_feature){
                    std::cout << debug << "," <<getName(item)<<"\n";
                }
                std::cout <<"[mts] candidate_values for {"<<ids[x]<<"}, number of values {"<<candidate_values_for_feature.size()<<"}\n";
                in.push_back(make_feature_vector(candidate_values_for_feature));
        }
        WorkingSet temp;  // TODO DZP: refactor
        std::cout << "[mts] def not about to segault\n";
        for (auto candidate : make_tuning_set_impl(temp, in)) {
                ret.push(makeTuningParameters(candidate));
        }
        std::cout << "[mts] see, didn't segfault\n";

        return ret;
}

extern "C" void kokkosp_request_tuning_variable_values(
    const size_t contextId, const size_t numContextVariables,
    const size_t* contextVariableIds,
    const Kokkos::Tuning::VariableValue* contextVariableValues,
    const size_t numTuningVariables, const size_t* tuningVariableIds,
    Kokkos::Tuning::VariableValue* tuningVariableValues,
    Kokkos::Tuning::SetOrRange* request_candidate_values) {
        TuningParameterSet request_parameters = {numTuningVariables,
                                                 tuningVariableIds};
        FeatureSet features = {numContextVariables, contextVariableIds};
        FeatureValues values = {numTuningVariables, tuningVariableIds,
                                tuningVariableValues};
        for (int x = 0; x < numTuningVariables; ++x) {
                printf("Have value for context variable %zu, value is %s\n",
                       contextVariableIds[x],
                       getName(contextVariableValues[x]).c_str());
        }
        for (int x = 0; x < numTuningVariables; ++x) {
                printf("Getting value for tuning variable %zu, value is %s\n",
                       tuningVariableIds[x],
                       getName(tuningVariableValues[x]).c_str());
        }
        for (int x = 0; x < numTuningVariables; ++x) {
                const size_t variableId = tuningVariableIds[x];
                if (candidate_is_set[variableId]) {
                        bool newResults = false;
                        for (auto iter = request_candidate_values[x].set.values;
                             iter < request_candidate_values[x].set.values +
                                        request_candidate_values[x].set.size;
                             ++iter) {
                                newResults |= candidate_values[variableId]
                                                  .insert(*iter)
                                                  .second;
                                std::cout <<"[rtvv] insert on {"<<variableId<<"}\n";
                        }  // TODO DZP: if newResults, add in the new values to
                           // the training set
                        if(newResults){
                          // TODO handle actual extension, not just single values
                          
                        }
                } else {
                        // TODO DZP: handle ranges
                }
        }
        auto& relevant_tuning = performance_data[request_parameters][features];
        if (relevant_tuning.find(values) == relevant_tuning.end()) {
                relevant_tuning[values] =
                    make_tuning_set(numTuningVariables, tuningVariableIds);
                size_t iter = 0;
                while (!relevant_tuning[values].empty()) {
                        ++iter;
                        auto feature_values = relevant_tuning[values].top();
                        relevant_tuning[values].pop();
                        std::cout << iter << " number of features = "
                                  << feature_values.count << " ";
                        for (int inner_iter = 0;
                             inner_iter < feature_values.count; ++inner_iter) {
                                std::cout
                                    << getName(
                                           feature_values.values[inner_iter])
                                    << ", ";
                        }
                        std::cout << "\n";
                }
        }
        auto& tuning_set = relevant_tuning[values];
        
        for(int x =0;x<numTuningVariables;++x){
          size_t id = tuningVariableIds[x]; 
          
        }
}

extern "C" void kokkosp_end_context(const size_t contextId) {}

