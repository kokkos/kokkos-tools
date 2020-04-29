
#include <cassert>
#include <cinttypes>
#include <cstdio>
#include <algorithm>
#include <cstring>
#include <utility>
#include <functional>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <vector>
#include <chrono>
#include <tuple>
#include <impl/Kokkos_Profiling_Interface.hpp>
#include <impl/Kokkos_Tuning_Interface.hpp>

#define TUNING_TOOL_SAVE_STATISTICS  // TODO DZP make build system option

template <typename... Args>
using MapType = std::map<Args...>;
template <typename... Args>
using SetType = std::set<Args...>;

static MapType<size_t, Kokkos::Tools::ValueType> var_info;

// from
// https://github.com/LLNL/Caliper/blob/5be39c5de6cba3806d96fe26ae1923a95fca3f54/src/common/util/format_util.cpp

const char whitespace[80 + 1] =
    "                                        "
    "                                        ";

std::ostream& pad_right(std::ostream& os, const std::string& str,
                        std::size_t width) {
  os << str;

  if (str.size() > width)
    os << ' ';
  else {
    std::size_t s = 1 + width - str.size();

    for (; s > 80; s -= 80) os << whitespace;

    os << whitespace + (80 - s);
  }

  return os;
}

std::ostream& pad_left(std::ostream& os, const std::string& str,
                       std::size_t width) {
  if (str.size() < width) {
    std::size_t s = width - str.size();

    for (; s > 80; s -= 80) os << whitespace;

    os << whitespace + (80 - s);
  }

  os << str << ' ';

  return os;
}

namespace std {
template <>
struct less<Kokkos::Tools::VariableValue> {
  bool operator()(const Kokkos::Tools::VariableValue& l,
                  const Kokkos::Tools::VariableValue& r) {
    switch (var_info[l.id]) {
      case kokkos_value_boolean: return l.value.bool_value < r.value.bool_value;
      case kokkos_value_integer:
        return l.value.int_value <
               r.value.int_value;  // TODO DZP: should this be (difference >
                                   // threshold)?
      case kokkos_value_floating_point:
        return l.value.double_value < r.value.double_value;
      case kokkos_value_text:
        return strncmp(l.value.string_value, r.value.string_value, 1024) < 0;
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
  size_t id                      = in.id;
  Kokkos::Tools::ValueType type = var_info[id];
  if (type == kokkos_value_boolean) {
    return (in.value.bool_value) ? "true" : "false";
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
 * use the tuning parameters I have encountered least
 *
 */

struct FeatureSet {
  size_t count;
  const size_t* ids;
  bool operator<(const FeatureSet& other) const {
    if (count < other.count) {
      return true;
    } else if (count >= other.count) {
      return false;
    } else {
      for (int x = 0; x < count; ++x) {
        if (ids[x] < other.ids[x]) {
          return true;
        }
        if (ids[x] >= other.ids[x]) {
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
  Kokkos::Tools::VariableValue* values;
  int times_encountered;
  size_t time_value;
  bool operator<(const TuningParameterValues& r) const {
    return times_encountered > r.times_encountered;
  }
};

bool Kokkos_Value_Less(const size_t l_id,
                       const Kokkos::Tools::VariableValue& l,
                       const size_t r_id,
                       const Kokkos::Tools::VariableValue& r) {
  //assert(var_info[l_id] == var_info[r_id]);
  switch (var_info[l_id]) {
    case kokkos_value_boolean: return l.value.bool_value < r.value.bool_value;
    case kokkos_value_integer: return l.value.int_value < r.value.int_value;
    case kokkos_value_floating_point:
      return l.value.double_value < r.value.double_value;
    case kokkos_value_text:
      return strncmp(l.value.string_value, r.value.string_value, 1024);
  }
}

struct FeatureValues {
  const size_t count;
  const size_t* ids;
  Kokkos::Tools::VariableValue* values;
  static std::less<Kokkos::Tools::VariableValue> comp;
  bool operator<(const FeatureValues& other) const {
    if (count < other.count) {
      return true;
    } else if (count > other.count) {
      return false;
    } else {
      for (int x = 0; x < count; ++x) {
        // TODO DZP: port over to user operator<
        if (comp(values[x], other.values[x])) {
          return true;
        } else if (comp(other.values[x], values[x])) {
          return false;
        }
      }
    }
    return false;
  }
};
std::less<Kokkos::Tools::VariableValue> FeatureValues::comp;

struct TuningResults {
  std::priority_queue<TuningParameterValues> candidates;
  bool done;
  TuningParameterValues ideal;
  ~TuningResults() {}  // TODO DZP: make this work, segfault on exit is terrible
};

MapType<TuningParameterSet,
        MapType<FeatureSet, MapType<FeatureValues, TuningResults>>>
    performance_data;

MapType<size_t, SetType<Kokkos::Tools::VariableValue>> candidate_values;
MapType<size_t, bool> candidate_is_set;  // TODO DZP: rewrite all these to be
                                         // one size_t -> VariableInfo map
MapType<size_t, char*> variable_names;
Kokkos::Tools::SetOrRange copy_candidate_values(
    const Kokkos::Tools::SetOrRange& in, bool isSet) {
  Kokkos::Tools::SetOrRange ret;
  if (isSet) {
    ret.set.size   = in.set.size;
    ret.set.values = new Kokkos::Tools::VariableValue[ret.set.size];
    std::copy(in.set.values, in.set.values + ret.set.size, ret.set.values);
  } else {
    ret.range = in.range;
  }
  return ret;
}

extern "C" void kokkosp_declare_tuning_variable(
    const char* name, const size_t id, Kokkos::Tools::VariableInfo info) {
  printf("New variable named %s with id %zu\n", name, id);
  var_info[id]         = info.type;
  candidate_is_set[id] = info.valueQuantity == kokkos_value_set;
  variable_names[id]   = (char*)malloc(sizeof(char) * 1024);
  strncpy(variable_names[id], name, 1024);
  // candidate_values[id] = copy_candidate_values(candidate_value,
  // (info.valueQuantity == kokkos_value_set));
}
extern "C" void kokkosp_declare_context_variable(
    const char* name, const size_t id, Kokkos::Tools::VariableInfo info,
    Kokkos::Tools::SetOrRange candidate_value) {
  printf("New variable named %s with id %zu\n", name, id);
  var_info[id]         = info.type;
  candidate_is_set[id] = info.valueQuantity == kokkos_value_set;
  variable_names[id]   = (char*)malloc(sizeof(char) * 1024);
  strncpy(variable_names[id], name, 1024);
}

template <typename... Args>
using VectorType = std::vector<Args...>;

using WorkingSet = VectorType<VectorType<Kokkos::Tools::VariableValue>>;

WorkingSet make_tuning_set_impl(WorkingSet& in, WorkingSet& add,
                                int debug = 0) {
  if (add.empty()) {
    // std::cout << "[mtsi] add_nothing {in.size=" << in.size() << "}\n";
    return in;
  }
  if (in.empty()) {
    std::vector<std::vector<Kokkos::Tools::VariableValue>> start_set;
    std::copy(add.begin(), add.end(), std::back_inserter(start_set));
    add.erase(add.begin());
    // std::cout << "[mtsi] in_nothing {start_set.size=" << start_set.size()
    //          << "}\n";
    // int foo = *start_set;
    WorkingSet working;
    for (auto item : start_set[0]) {
      working.push_back(std::vector<Kokkos::Tools::VariableValue>{item});
    }
    const auto& x = make_tuning_set_impl(working, add, debug + 1);
    // std::cout << "[mtsi] in_nothing {return_size=" << x.size() << "}\n";

    return x;
  }
  // std::cout << "[mtsi] merging\n";
  WorkingSet working;
  std::vector<Kokkos::Tools::VariableValue>& features =
      *add.erase(add.begin());
  for (auto& vec : in) {
    for (auto& feature : features) {
      std::vector<Kokkos::Tools::VariableValue> build_me{feature};
      std::copy(vec.begin(), vec.end(), build_me.begin());
      working.push_back(std::move(build_me));
    }
  }
  return make_tuning_set_impl(working, add, debug + 1);
}

constexpr int num_samples = 1;

// TODO DZP: this function is much truncated, is it still necessary?
std::vector<Kokkos::Tools::VariableValue> make_feature_vector(
    const SetType<Kokkos::Tools::VariableValue>& in) {
  std::vector<Kokkos::Tools::VariableValue> ret;
  std::copy(in.begin(), in.end(), std::back_inserter(ret));
  return ret;
}

TuningParameterValues values_from_vector(
    std::vector<Kokkos::Tools::VariableValue> in);

TuningParameterValues makeTuningParameters(
    const std::vector<Kokkos::Tools::VariableValue>& in) {
  TuningParameterValues ret;
  ret.count             = in.size();
  ret.time_value        = 0;
  ret.times_encountered = 0;
  ret.values            = new Kokkos::Tools::VariableValue[ret.count];

  std::copy(in.begin(), in.end(), ret.values);
  return ret;
}

std::priority_queue<TuningParameterValues> make_tuning_set(const size_t count,
                                                           const size_t* ids) {
  std::priority_queue<TuningParameterValues> ret;
  WorkingSet in;
  for (auto x = 0; x < count; ++x) {
    auto candidate_values_for_feature = candidate_values[ids[x]];
    int debug                         = 0;
    auto feature_vector = make_feature_vector(candidate_values_for_feature);
    // std::cout << "[mts] candidate_values for {" << ids[x]
    //          << "}, number of values {" <<
    //          candidate_values_for_feature.size()
    //          << "}, features in each vector {}\n";
    in.push_back(feature_vector);
  }
  WorkingSet temp;  // TODO DZP: refactor
  // std::cout << "[mts] def not about to segault\n";
  for (auto candidate : make_tuning_set_impl(temp, in)) {
    // std::cout << "[mts] inserting candidate of size " << candidate.size()
    //           << "\n";

    ret.push(makeTuningParameters(candidate));
  }
  // std::cout << "[mts] see, didn't segfault\n";

  return ret;
}
Kokkos::Tools::VariableValue copy_variable_value(
    const Kokkos::Tools::VariableValue& in) {
  Kokkos::Tools::VariableValue ret;
  switch (var_info[in.id]) {
    case kokkos_value_boolean:
    case kokkos_value_integer:
    case kokkos_value_floating_point: ret = in; break;
    case kokkos_value_text:
      ret.id                 = in.id;
      std::string* temp      = new std::string(in.value.string_value);
      ret.value.string_value = temp->c_str();
      break;
  }
  return ret;
}
MapType<size_t, std::vector<std::pair<std::reference_wrapper<TuningResults>,
                                      TuningParameterValues>>>
    live_values;
#ifdef TUNING_TOOL_SAVE_STATISTICS
struct TuningData {
  size_t count;
  Kokkos::Tools::VariableValue* values;
  size_t time;
};
static MapType<FeatureValues, std::vector<TuningData>> full_performance_results;
#endif
TuningParameterValues getIdeal(FeatureValues& context, TuningResults& results) {
  auto& in = results.candidates;
  TuningParameterValues ideal;
  ideal = in.top();
  in.pop();
  while (!in.empty()) {
    auto candidate = in.top();
    if (candidate.time_value < ideal.time_value) {
      ideal = candidate;
    }
#ifdef TUNING_TOOL_SAVE_STATISTICS
    full_performance_results[context].push_back(
        TuningData{candidate.count, candidate.values, candidate.time_value});
#endif
    in.pop();
  }
  return ideal;
}
MapType<size_t, decltype(std::chrono::system_clock::now())> running_timers;
extern "C" void kokkosp_request_tuning_variable_values(
    const size_t contextId, const size_t numContextVariables,
    
    const Kokkos::Tools::VariableValue* contextVariableValues,
    const size_t numTuningVariables, 
    Kokkos::Tools::VariableValue* tuningVariableValues,
    Kokkos::Tools::SetOrRange* request_candidate_values) {
  // TODO DZP: only make copies when you're inserting into a map, this is slow
  // and leaky
  auto tuningVariableIds_copy = new size_t[numTuningVariables];
  //std::copy(tuningVariableIds, tuningVariableIds + numTuningVariables,
  //          tuningVariableIds_copy);
  for(int x =0; x < numTuningVariables;++x){
    tuningVariableIds_copy[x] = tuningVariableValues[x].id;
  }
  auto contextVariableIds_copy = new size_t[numContextVariables];
  for(int x =0; x < numContextVariables;++x){
    contextVariableIds_copy[x] = contextVariableValues[x].id;
  }
  //std::copy(contextVariableIds, contextVariableIds + numContextVariables,
  //          contextVariableIds_copy);
  TuningParameterSet request_parameters = {numTuningVariables,
                                           tuningVariableIds_copy};
  FeatureSet features = {numContextVariables, contextVariableIds_copy};
  auto relevantVariableValues_copy =
      new Kokkos::Tools::VariableValue[numContextVariables];
  for (int x = 0; x < numContextVariables; ++x) {
    relevantVariableValues_copy[x] =
        copy_variable_value(contextVariableValues[x]);
  }
  FeatureValues values = {numContextVariables, contextVariableIds_copy,
                          relevantVariableValues_copy};
  for (int x = 0; x < numTuningVariables; ++x) {
    const size_t variableId = tuningVariableIds_copy[x];
    if (candidate_is_set[variableId]) {
      bool newResults = false;
      for (auto iter = request_candidate_values[x].set.values;
           iter < request_candidate_values[x].set.values +
                      request_candidate_values[x].set.size;
           ++iter) {
        // std::cout << "Inserting into map with id " << iter->id << std::endl;
        if(candidate_values.find(variableId)==candidate_values.end()){
          candidate_values[variableId] = SetType<Kokkos::Tools::VariableValue>();
        }
        auto insert_result = candidate_values[variableId].insert(
            Kokkos::Tools::VariableValue{iter->id, iter->value});
        newResults |= insert_result.second;
        // std::cout << "[rtvv] insert on {" << variableId << "}\n";
      }  // TODO DZP: if newResults, add in the new values to
         // the training set
      if (newResults) {
        // TODO handle actual extension, not just single
        // values being right
      }
    } else {
      // TODO DZP: handle ranges
    }
  }
  auto& relevant_tuning = performance_data[request_parameters][features];
  if (relevant_tuning.find(values) == relevant_tuning.end()) {
    relevant_tuning[values] = {
        make_tuning_set(numTuningVariables, tuningVariableIds_copy), false};
  }
  auto& tuning_set = relevant_tuning[values];
  if (tuning_set.done) {
    auto& ideal = tuning_set.ideal;
    for (int k = 0; k < tuning_set.ideal.count; ++k) {
      for (int x = 0; x < numTuningVariables; ++x) {
        if (tuningVariableIds_copy[x] == tuning_set.ideal.values[k].id) {
          tuningVariableValues[x] = tuning_set.ideal.values[k];
        }
      }
    }
    return;
  } else {
    auto& candidate = tuning_set.candidates.top();
    if (candidate.times_encountered > num_samples) {
      tuning_set.done  = true;
      tuning_set.ideal = getIdeal(values, tuning_set);
      std::cout << "Ideal value: ";
      for (auto x = 0; x < tuning_set.ideal.count; ++x) {
        std::cout << getName(tuning_set.ideal.values[x]) << std::endl;
      }
    }
    for (int k = 0; k < candidate.count; ++k) {
      for (int x = 0; x < numTuningVariables; ++x) {
        if (tuningVariableIds_copy[x] == candidate.values[k].id) {
          tuningVariableValues[x] = candidate.values[k];
          if (running_timers.find(contextId) == running_timers.end()) {
            running_timers[contextId] = std::chrono::system_clock::now();
            live_values[contextId].push_back(
                std::make_pair(std::ref(tuning_set), candidate));
          }
        }
      }
    }
    tuning_set.candidates.pop();
  }
}

extern "C" void kokkosp_end_context(const size_t contextId) {
  if (running_timers.find(contextId) != running_timers.end()) {
    auto now       = std::chrono::system_clock::now();
    size_t elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                         now - running_timers[contextId])
                         .count();
    for (auto& tuning_values : live_values[contextId]) {
      auto& tuningResults = tuning_values.first;
      if (tuningResults.get().done) {
        return;
      }
      auto values = tuning_values.second;
      values.time_value += elapsed;  // TODO DZP: some overflow handling
      values.times_encountered += 1;
      // std::cout << "I've seen this value(" << getName(values.values[0]) <<
      //")"
      //         << values.times_encountered << " times \n";
      tuningResults.get().candidates.push(values);
    }
    live_values[contextId].clear();
  }
  running_timers.erase(contextId);
}

template <typename Iterable, typename Functor>
void for_all_variables(const Iterable& in, const Functor& func) {
  for (int x = 0; x < in.count; ++x) {
    func(in.ids[x]);
  }
}

template <typename Iterable, typename Functor>
void for_all_values(const Iterable& in, const Functor& func) {
  for (int x = 0; x < in.count; ++x) {
    func(in.values[x]);
  }
}

extern "C" void kokkosp_finalize_library() {
  printf("KokkosP: Kokkos library finalization called.\n");
  for (auto& kv : performance_data) {
    auto& tuning_parameters = kv.first;
    std::cout << "Tuning Parameter Set\n";
    for_all_variables(tuning_parameters, [&](const size_t id) {
      std::cout << variable_names[id] << ", ";
    });
    std::cout << std::endl;
    for (auto& kv2 : kv.second) {
      auto& features = kv2.first;
      std::cout << "  Feature set\n";
      std::cout << "  ";
      for_all_variables(features, [&](const size_t id) {
        // std::cout << std::cout.width(24) << std::cout.fill(' ') <<std::left<<
        // variable_names[id] << std::cout.width(0)<<", ";
        pad_right(std::cout, std::string(variable_names[id]) + ",", 24);
      });
      std::cout << "\n";
      for (auto& kv3 : kv2.second) {
        auto& feature_values = kv3.first;
        auto& tuning_results = kv3.second;
        std::cout << "  ";
        for_all_values(
            feature_values, [&](const Kokkos::Tools::VariableValue& in) {
              pad_right(std::cout, std::string(getName(in)) + ",", 24);
            });
        std::cout << "\n";
        if (!tuning_results.done) {
          std::cout << "    Not enough samples to converge (common for "
                       "internal foralls)\n";
          continue;
        }
        std::cout << "    Best results\n";  // TODO DZP: implement ideal in
                                            // TuningResults
        std::cout << "    ";
        for_all_values(
            tuning_results.ideal, [&](const Kokkos::Tools::VariableValue& in) {
              pad_right(std::cout, std::string(getName(in)) + ", ", 24);
            });
        std::cout << " {time in us: "
                  << tuning_results.ideal.time_value / num_samples << "}";
        std::cout << std::endl;
#ifdef TUNING_TOOL_SAVE_STATISTICS
std::sort(full_performance_results[feature_values].begin(), full_performance_results[feature_values].end(), [&](const TuningData& l, const TuningData& r ) { return l.time < r.time;});
        for (const auto& alternate : full_performance_results[feature_values] ) {
          std::cout << "      Other results\n";  // TODO DZP: implement ideal in
                                               // TuningResults
          std::cout << "      ";
          for_all_values(
              alternate, [&](const Kokkos::Tools::VariableValue& in) {
                pad_right(std::cout, std::string(getName(in)) + ", ", 24);
              });
        std::cout << " {time in us: "
                  << alternate.time / num_samples << "}";
        std::cout << std::endl;
        }
#endif
      }
    }
  }
}

