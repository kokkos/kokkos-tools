#ifndef LABRADOR_EXPLORATION_HPP
#define LABRADOR_EXPLORATION_HPP
#include <chrono>
#include <cstddef>
#include <cstring>
#include <impl/Kokkos_Profiling_Interface.hpp>
#include <iostream>
#include <map>
#include <set>
#include <sqlite3.h>
#include <string>
#include <vector>

namespace labrador{
	namespace explorer {
constexpr const size_t max_variables = 64;
struct variableSet {
  int64_t variable_ids[max_variables];
  size_t num_variables;
  size_t num_input_variables;
};
}
}
namespace std {
template <> struct less<labrador::explorer::variableSet> {
  bool operator()(const labrador::explorer::variableSet &l, const labrador::explorer::variableSet &r) {
    if (l.num_variables != r.num_variables) {
      return l.num_variables < r.num_variables;
    }
    for (int x = 0; x < l.num_variables; ++x) {
      if (l.variable_ids[x] != r.variable_ids[x]) {
        return l.variable_ids[x] < r.variable_ids[x];
      }
    }
    return false;
  }
};
} // namespace std

namespace labrador {
namespace explorer {

using valunion = decltype(std::declval<Kokkos::Tools::Experimental::VariableValue>().value);
using valtype = decltype(std::declval<Kokkos::Tools::Experimental::VariableValue>().metadata->type);

int cmp(valtype t, valunion l, valunion r);

struct VariableDatabaseData {
  int64_t canonical_id;
  char* name;
  int64_t candidate_set_size;
  valunion minimum;
  valunion maximum;
  Kokkos::Tools::Experimental::VariableValue *candidate_values;
};

using db_id_type = int64_t;
constexpr const int64_t slice_continuous = 100;
constexpr const int64_t tuning_data_buffer_size = 1000;
constexpr const double epsilon = 1E-24;
using clock_type = std::chrono::high_resolution_clock;
using time_point = decltype(clock_type::now());

struct dataSet {
  float result;
  Kokkos::Tools::Experimental::VariableValue values[max_variables];
  time_point start_time;
};

struct tuningData {
  dataSet data[tuning_data_buffer_size];
  size_t problem_size;
  int64_t problem_id;
  int64_t num_trials;
  int64_t choice_index;
};


sqlite3_stmt *prepare_statement(sqlite3 *db, std::string sql);

void create_tables(sqlite3 *db);

namespace impl {

void bind_arg(sqlite3_stmt *stmt, int index, std::nullptr_t value);
void bind_arg(sqlite3_stmt *stmt, int index, int64_t value);
void bind_arg(sqlite3_stmt *stmt, int index, int value);
void bind_arg(sqlite3_stmt *stmt, int index, double value);
void bind_arg(sqlite3_stmt *stmt, int index, std::string value);

template <typename... Args>
void bind_statement(sqlite3_stmt *stmt, int index, Args... args);

template <typename Arg, typename... Args>
void bind_statement(sqlite3_stmt *stmt, int index, Arg arg, Args... args) {
  bind_arg(stmt, index, arg);
  bind_statement(stmt, index + 1, args...);
}

template <> void bind_statement(sqlite3_stmt *stmt, int index);

//extern template void bind_statement(sqlite3_stmt*, int);
//extern template void bind_statement(sqlite3_stmt*, int,std::nullptr_t, long);
//extern template void bind_statement(sqlite3_stmt*, int);
//extern template void bind_statement(sqlite3_stmt*, int, long, long, std::nullptr_t);
//extern template void bind_statement(sqlite3_stmt*, int, std::string);
//extern template void bind_statement(sqlite3_stmt*, int, std::string, std::string);

} // namespace impl

template <typename... Args>
void bind_statement(sqlite3_stmt *stmt, Args... args) {
  return impl::bind_statement(stmt, 1, args...);
}

//extern template void bind_statement(sqlite3_stmt*, int, std::string, long, long);
//extern template void bind_statement(sqlite3_stmt*, std::string, long, long);

using ValueType = Kokkos::Tools::Experimental::ValueType;
using StatisticalCategory = Kokkos::Tools::Experimental::StatisticalCategory;
using CandidateValueType = Kokkos::Tools::Experimental::CandidateValueType;

int64_t id_for_type(ValueType type) ;

int64_t id_for_category(StatisticalCategory category) ;

void make_candidate_set(int64_t id,
                        const Kokkos::Tools::Experimental::VariableInfo &info) ;

db_id_type
insert_type_id(sqlite3_stmt *stmt, const std::string &name, size_t id,
               const Kokkos::Tools::Experimental::VariableInfo &info) ;

db_id_type get_type_id(sqlite3_stmt *get_stmt, sqlite3_stmt *set_stmt,
                       const std::string &name, size_t id,
                       const Kokkos::Tools::Experimental::VariableInfo &info) ;
 

int64_t slice_helper(double upper, double lower, double step);


int64_t count_range_slices(Kokkos_Tools_ValueRange &in,
                           Kokkos::Tools::Experimental::ValueType type) ;

Kokkos::Tools::Experimental::VariableValue
mvv(size_t id, Kokkos_Tools_VariableValue_ValueUnionSet values,
    Kokkos::Tools::Experimental::VariableInfo &info, int index) ;

Kokkos::Tools::Experimental::VariableValue
mvv(size_t id, Kokkos_Tools_VariableValue_ValueUnion value,
    Kokkos::Tools::Experimental::VariableInfo &info) ;
void form_set_from_range(Kokkos::Tools::Experimental::VariableValue *fill_this,
                         Kokkos_Tools_ValueRange with_this, int64_t num_slices,
                         size_t id,
                         Kokkos::Tools::Experimental::VariableInfo &info) ;
void associate_candidates(const size_t id,
                          Kokkos::Tools::Experimental::VariableInfo &info) ;

void kokkosp_init_library(const int,
                                     const uint64_t interface_version,
                                     const uint32_t, void *) ;

void kokkosp_declare_input_type(const char *name, const size_t id,
                           Kokkos::Tools::Experimental::VariableInfo *info);
void kokkosp_declare_output_type(const char *name, const size_t id,
                            Kokkos::Tools::Experimental::VariableInfo *info);

using Kokkos::Tools::Experimental::VariableValue;

struct contextDescription {
  size_t id;
  size_t size;
};

bool operator<(const contextDescription &l, const contextDescription &r) ;

int64_t make_search_problem(variableSet &variables, tuningData &data) ;
int64_t get_problem_id(variableSet &variables, tuningData &data);
void flush_buffer(variableSet &variables, tuningData &buffer);

void kokkosp_request_values(size_t context_id,
                                       size_t num_context_variables,
                                       VariableValue *context_values,
                                       size_t num_tuning_variables,
                                       VariableValue *tuning_values) ;
void kokkosp_finalize_library();
void kokkosp_end_context(size_t context_id);

} // namespace explorer
} // namespace labrador
#endif
