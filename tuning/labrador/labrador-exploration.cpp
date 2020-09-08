#include <chrono>
#include <cstddef>
#include <cstring>
#include <impl/Kokkos_Profiling_Interface.hpp>
#include "labrador-exploration.hpp"
#include <iostream>
#include <map>
#include <set>
#include <sqlite3.h>
#include <string>
#include <vector>
namespace labrador {
namespace explorer {
using valunion = decltype(std::declval<Kokkos::Tools::Experimental::VariableValue>().value);
using valtype = decltype(std::declval<Kokkos::Tools::Experimental::VariableValue>().metadata->type);

int cmp(valtype t, valunion l, valunion r){

  if(l.int_value < r.int_value){
          return -1;
  }
  if(l.int_value > r.int_value){
    return 1;
  }
  return 0;
}


using db_id_type = int64_t;
static int64_t tuning_choice;

using clock_type = std::chrono::high_resolution_clock;
using time_point = decltype(clock_type::now());

sqlite3 *tool_db;
sqlite3_stmt *get_input_type;
sqlite3_stmt *get_output_type;
sqlite3_stmt *set_input_type;
sqlite3_stmt *set_output_type;
sqlite3_stmt *insert_candidate_set_entry;
sqlite3_stmt *get_search_problem;
sqlite3_stmt *insert_search_problem;
sqlite3_stmt *insert_problem_input;
sqlite3_stmt *insert_problem_output;
sqlite3_stmt *insert_trial_data;
sqlite3_stmt *insert_trial_values;
int64_t num_types;
int64_t num_problems;
int64_t num_trials;
const char *tail = (char *)malloc(1024);
sqlite3_stmt *prepare_statement(sqlite3 *db, std::string sql) {
  sqlite3_stmt *statement;
  size_t sql_size = sql.size();
  int status = sqlite3_prepare_v3(db, sql.data(), sql_size,
                                  SQLITE_PREPARE_PERSISTENT, &statement, &tail);
  if (status != SQLITE_OK) {
    std::cout << "[prepare_statement] compile error " << sql << " [" << status
              << "]\n";
  }
  return statement;
}

void create_tables(sqlite3 *db) {
  std::string errors;
  errors.reserve(128);
  const char *data = errors.c_str();
  sqlite3_exec(
      db,
      "CREATE TABLE IF NOT EXISTS input_types(id int PRIMARY KEY, name text "
      "NOT NULL, type int NOT NULL, statistical_category int NOT NULL)",
      nullptr, nullptr, const_cast<char **>(&data));
  sqlite3_exec(
      db,
      "CREATE TABLE IF NOT EXISTS output_types(id int PRIMARY KEY, name text "
      "NOT NULL, type int NOT NULL, statistical_category int NOT NULL)",
      nullptr, nullptr, const_cast<char **>(&data));
  sqlite3_exec(db,
               "CREATE TABLE IF NOT EXISTS optimization_goals(id int PRIMARY "
               "KEY, optimized_type int, goal_type int)",
               nullptr, nullptr, const_cast<char **>(&data));
  sqlite3_exec(db,
               "CREATE TABLE IF NOT EXISTS problem_descriptions(id int, "
               "problem_inputs int, problem_outputs int)",
               nullptr, nullptr, const_cast<char **>(&data));
  sqlite3_exec(db,
               "CREATE TABLE IF NOT EXISTS problem_inputs(problem_id int, "
               "variable_id int, variable_index int)",
               nullptr, nullptr, const_cast<char **>(&data));
  sqlite3_exec(db,
               "CREATE TABLE IF NOT EXISTS problem_outputs(problem_id int, "
               "variable_id int, variable_index int)",
               nullptr, nullptr, const_cast<char **>(&data));
  sqlite3_exec(db,
               "CREATE TABLE IF NOT EXISTS trials(trial_id int, problem_id "
               "int,  result real)",
               nullptr, nullptr, const_cast<char **>(&data));
  sqlite3_exec(db,
               "CREATE TABLE IF NOT EXISTS candidate_sets(id int, "
               "continuous_value real, discrete_value int)",
               nullptr, nullptr, const_cast<char **>(&data));
  sqlite3_exec(db,
               "CREATE TABLE IF NOT EXISTS trial_values(trial_id int, "
               "variable_id int, discrete_result int, "
               "real_result real)",
               nullptr, nullptr, const_cast<char **>(&data));
  insert_trial_data =
      prepare_statement(db, "INSERT INTO trials VALUES (?,?,?)");
  insert_trial_values =
      prepare_statement(db, "INSERT INTO trial_values VALUES (?,?,?,?)");
  insert_candidate_set_entry =
      prepare_statement(db, "INSERT INTO candidate_sets VALUES(?,?,?)");
  get_input_type = prepare_statement(
      db, "SELECT id FROM input_types WHERE input_types.name == ?");
  get_output_type = prepare_statement(
      db, "SELECT id FROM output_types WHERE output_types.name == ?");
  set_output_type =
      prepare_statement(db, "INSERT INTO output_types VALUES (?,?,?,?)");
  set_input_type =
      prepare_statement(db, "INSERT INTO input_types VALUES (?,?,?,?)");
  get_search_problem = prepare_statement(
      db,
      "SELECT * FROM ((SELECT distinct problem_id, group_concat(variable_id) "
      "OVER(PARTITION BY problem_id ORDER BY variable_id ROWS BETWEEN "
      "UNBOUNDED PRECEDING AND UNBOUNDED FOLLOWING) AS input_variables FROM "
      "problem_inputs)) a LEFT JOIN ((SELECT distinct problem_id, "
      "group_concat(variable_id) OVER(PARTITION BY problem_id ORDER BY "
      "variable_id ROWS BETWEEN UNBOUNDED PRECEDING AND UNBOUNDED FOLLOWING) "
      "AS output_variables FROM problem_outputs)) b ON a.problem_id == "
      "b.problem_id WHERE a.input_variables==? AND b.output_variables==?");
  insert_search_problem =
      prepare_statement(db, "INSERT INTO problem_descriptions VALUES (?,?,?)");
  insert_problem_input =
      prepare_statement(db, "INSERT INTO problem_inputs VALUES (?,?,?)");
  insert_problem_output =
      prepare_statement(db, "INSERT INTO problem_outputs VALUES (?,?,?)");
  sqlite3_exec(
      db, "SELECT COUNT(*) FROM input_types",
      [](void *, int count, char **values, char **) {
        num_types = std::stoi(values[0]);
        return 0;
      },
      nullptr, const_cast<char **>(&data));
  sqlite3_exec(
      db, "SELECT COUNT(*) FROM output_types",
      [](void *, int count, char **values, char **) {
        num_types += std::stoi(values[0]);
        return 0;
      },
      nullptr, const_cast<char **>(&data));
  sqlite3_exec(
      db, "SELECT COUNT(*) FROM problem_descriptions",
      [](void *, int count, char **values, char **) {
        num_problems = std::stoi(values[0]);
        return 0;
      },
      nullptr, const_cast<char **>(&data));
  sqlite3_exec(
      db, "SELECT COUNT(*) FROM trials",
      [](void *, int count, char **values, char **) {
        num_trials = std::stoi(values[0]);
        return 0;
      },
      nullptr, const_cast<char **>(&data));
}

namespace impl {

void bind_arg(sqlite3_stmt *stmt, int index, std::nullptr_t value) {
  sqlite3_bind_null(stmt, index);
}
void bind_arg(sqlite3_stmt *stmt, int index, int64_t value) {
  sqlite3_bind_int64(stmt, index, value);
}
void bind_arg(sqlite3_stmt *stmt, int index, int value) {
  sqlite3_bind_int(stmt, index, value);
}
void bind_arg(sqlite3_stmt *stmt, int index, double value) {
  sqlite3_bind_double(stmt, index, value);
}
void bind_arg(sqlite3_stmt *stmt, int index, std::string value) {
  char *x = new char[value.size()];
  strncpy(x, value.c_str(), value.size());
  sqlite3_bind_text(stmt, index, x, value.size(),
                    [](void *data) { free(data); });
}
} // namespace impl
void kokkosp_init_library(const int,
                                     const uint64_t interface_version,
                                     const uint32_t, void *) {
  const char* index = getenv("OMPI_COMM_WORLD_RANK");
  std::string name = "tuning_db";
  if(index){
    name += std::string(index); 
  }
  name += ".db";
  sqlite3_open_v2(name.c_str(), &tool_db,
                  SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
  create_tables(tool_db);
  char *tuning_choice_string = getenv("LABRADOR_TUNING_CHOICE");
  if (tuning_choice_string == nullptr) {
    tuning_choice = 0;
  } else {
    tuning_choice = std::stoi(std::string(tuning_choice_string));
  }
  srand(time(nullptr));
}

using ValueType = Kokkos::Tools::Experimental::ValueType;
using StatisticalCategory = Kokkos::Tools::Experimental::StatisticalCategory;
using CandidateValueType = Kokkos::Tools::Experimental::CandidateValueType;

int64_t id_for_type(ValueType type) {
  switch (type) {
  case ValueType::kokkos_value_int64:
    return 1;
  case ValueType::kokkos_value_double:
    return 2;
  case ValueType::kokkos_value_string:
    return 3;
  }
  return -1;
}

int64_t id_for_category(StatisticalCategory category) {
  switch (category) {
  case StatisticalCategory::kokkos_value_categorical:
    return 0;
  case StatisticalCategory::kokkos_value_ordinal:
    return 1;
  case StatisticalCategory::kokkos_value_interval:
    return 2;
  case StatisticalCategory::kokkos_value_ratio:
    return 3;
  }
  return -1;
}

void make_candidate_set(int64_t id,
                        const Kokkos::Tools::Experimental::VariableInfo *info) {
  auto database_data =
      reinterpret_cast<VariableDatabaseData *>(info->toolProvidedInfo);
  auto set = database_data->candidate_values;
  switch (info->type) {
  case ValueType::kokkos_value_int64:
    for (int x = 0; x < database_data->candidate_set_size; ++x) {
      bind_statement(insert_candidate_set_entry, id, std::nullptr_t{},
                     set[x].value.int_value);
      sqlite3_step(insert_candidate_set_entry);
      sqlite3_reset(insert_candidate_set_entry);
    }
    break;
  case ValueType::kokkos_value_double:
    for (int x = 0; x < database_data->candidate_set_size; ++x) {
      bind_statement(insert_candidate_set_entry, id, set[x].value.double_value,
                     std::nullptr_t{});
      sqlite3_step(insert_candidate_set_entry);
      sqlite3_reset(insert_candidate_set_entry);
    }
    break;
  case ValueType::kokkos_value_string:
    for (int x = 0; x < database_data->candidate_set_size; ++x) {
      bind_statement(insert_candidate_set_entry, id, std::nullptr_t{},
                     int64_t(std::hash<std::string>{}(
                         std::string(set[x].value.string_value))));
      sqlite3_step(insert_candidate_set_entry);
      sqlite3_reset(insert_candidate_set_entry);
    }
    break;
  }
}

db_id_type
insert_type_id(sqlite3_stmt *stmt, const std::string &name, size_t id,
               const Kokkos::Tools::Experimental::VariableInfo *info) {
  int type_id = ++num_types;
  bind_statement(stmt, type_id, name, id_for_type(info->type),
                 id_for_category(info->category));
  int status = sqlite3_step(stmt);
  if (status != SQLITE_DONE) {
    std::cout << "[insert_input_type] insert not successful [" << status << "]"
              << std::endl;
  } else {
    sqlite3_reset(stmt);
  }
  switch (info->valueQuantity) {
  case CandidateValueType::kokkos_value_range:
  case CandidateValueType::kokkos_value_set:
    make_candidate_set(type_id, info);
    break;
  case CandidateValueType::kokkos_value_unbounded:
    // TODO: check that this is an output variable?
    break;
  }
  return type_id;
}

db_id_type get_type_id(sqlite3_stmt *get_stmt, sqlite3_stmt *set_stmt,
                       const std::string &name, size_t id,
                       const Kokkos::Tools::Experimental::VariableInfo *info) {
  bind_statement(get_stmt, name);
  // check status
  int status = sqlite3_step(get_stmt);
  if (status == SQLITE_ROW) {
    int64_t canonical_id = sqlite3_column_int64(get_stmt, 0);
    sqlite3_reset(get_stmt);
    return canonical_id;
    // std::cout << "[get_input_type_id] "<<id<<" query with rows
    // "<<canonical_id<<std::endl;

  } else if (status == SQLITE_DONE) {

    // std::cout << "[get_input_type_id] "<<id<<" query without rows
    // for"<<name<<std::endl;
    sqlite3_reset(get_stmt);
    return insert_type_id(set_stmt, name, id, info);
  } else {
    std::cout << "[get_input_type_id] " << id << " errors?" << std::endl;
  }
  sqlite3_reset(get_stmt);
  return -1;
}
 

int64_t slice_helper(double upper, double lower, double step){
  return ((upper - lower) / step);

}

int64_t count_range_slices(Kokkos_Tools_ValueRange &in,
                           Kokkos::Tools::Experimental::ValueType type) {

  switch (type) {
  case ValueType::kokkos_value_int64:
    return ((in.openUpper ? in.lower.int_value : (in.lower.int_value - 1)) -
            (in.openLower ? in.lower.int_value : (in.lower.int_value + 1))) /
           in.step.int_value;
  case ValueType::kokkos_value_double:
    return (in.step.double_value ==
            0.0) // intentional comparison to double value 0.0, this shouldn't
                 // be a calculated value, but a value that has been set to the
                 // literal 0.0
               ? slice_continuous
	       : slice_helper( in.openUpper ? in.upper.double_value : (in.upper.double_value - epsilon),
                  in.openLower ? in.lower.double_value : (in.lower.double_value + epsilon),
		  in.step.double_value
			       );
  case ValueType::kokkos_value_string:
    // TODO: error mechanism?
    return -1;
  }
  return -1;
}

Kokkos::Tools::Experimental::VariableValue
mvv(size_t id, Kokkos_Tools_VariableValue_ValueUnionSet values,
    Kokkos::Tools::Experimental::VariableInfo *info, int index) {
  Kokkos_Tools_VariableValue_ValueUnion holder;
  switch (info->type) {
  case ValueType::kokkos_value_int64:
    holder.int_value = values.int_value[index];
    break;
  case ValueType::kokkos_value_double:
    holder.double_value = values.double_value[index];
    break;
  case ValueType::kokkos_value_string:
    strncpy(holder.string_value,values.string_value[index],64);
    break;
  }
  Kokkos::Tools::Experimental::VariableValue value;
  value.type_id = id;
  value.value = holder;
  Kokkos::Tools::Experimental::VariableInfo *new_info =
      new Kokkos::Tools::Experimental::VariableInfo(*info);
  value.metadata = new_info;
  return value;
}

Kokkos::Tools::Experimental::VariableValue
mvv(size_t id, Kokkos_Tools_VariableValue_ValueUnion value,
    Kokkos::Tools::Experimental::VariableInfo *info) {
  Kokkos_Tools_VariableValue_ValueUnion holder;
  switch (info->type) {
  case ValueType::kokkos_value_int64:
    holder.int_value = value.int_value;
    break;
  case ValueType::kokkos_value_double:
    holder.double_value = value.double_value;
    break;
  case ValueType::kokkos_value_string:
    strncpy(holder.string_value,value.string_value,64);
    break;
  }
  Kokkos::Tools::Experimental::VariableValue ret_value;
  ret_value.type_id = id;
  ret_value.value = holder;
  ret_value.metadata = info;
  return ret_value;
}
void form_set_from_range(Kokkos::Tools::Experimental::VariableValue *fill_this,
                         Kokkos_Tools_ValueRange with_this, int64_t num_slices,
                         size_t id,
                         Kokkos::Tools::Experimental::VariableInfo *info) {
  if (info->type == ValueType::kokkos_value_int64) {
    for (int x = 0; x < num_slices; ++x) {
      auto lower =
          (with_this.openLower
               ? with_this.lower.int_value
               : (with_this.lower.int_value + with_this.step.int_value));
      Kokkos_Tools_VariableValue_ValueUnion value;
      value.int_value = lower + (x * with_this.step.int_value);
      fill_this[x] = mvv(id, value, info);
    }
  } else if (info->type == ValueType::kokkos_value_double) {
    for (int x = 0; x < num_slices; ++x) {
      auto lower =
          (with_this.openLower ? with_this.lower.double_value
                               : (with_this.lower.double_value + epsilon));
      Kokkos_Tools_VariableValue_ValueUnion value;
      value.double_value = lower + (x * with_this.step.double_value);
      fill_this[x] = mvv(id, value, info);
    }
  }
}
void associate_candidates(const size_t id,
                          Kokkos::Tools::Experimental::VariableInfo *info) {
  int64_t candidate_set_size = 0;
  auto *databaseInfo =
      reinterpret_cast<VariableDatabaseData *>(info->toolProvidedInfo);
  switch (info->valueQuantity) {
  case CandidateValueType::kokkos_value_set:
    candidate_set_size = info->candidates.set.size;
    break;
  case CandidateValueType::kokkos_value_range:
    candidate_set_size = count_range_slices(info->candidates.range, info->type);
    //std::cout << "[ac] range as slices: "<<candidate_set_size<<"\n";
    break;
  case CandidateValueType::kokkos_value_unbounded:
    candidate_set_size = 0;
    break;
  }
  if (candidate_set_size > 0) {
    Kokkos::Tools::Experimental::VariableValue *candidate_values =
        new Kokkos::Tools::Experimental::VariableValue[candidate_set_size];
    switch (info->valueQuantity) {
    case CandidateValueType::kokkos_value_set:
      for (int x = 0; x < candidate_set_size; ++x) {
        candidate_values[x] = mvv(id, info->candidates.set.values, info, x);
      }
      break;
    case CandidateValueType::kokkos_value_range:
      form_set_from_range(candidate_values, info->candidates.range,
                          candidate_set_size, id, info);
      break;
    case CandidateValueType::kokkos_value_unbounded:
      candidate_set_size = 0;
      break;
    }
    databaseInfo->candidate_values = candidate_values;
  }
  databaseInfo->candidate_set_size = candidate_set_size;
}

void
kokkosp_declare_input_type(const char *name, const size_t id,
                           Kokkos::Tools::Experimental::VariableInfo *info) {
  info->toolProvidedInfo = new VariableDatabaseData{};
  associate_candidates(id, info);
  int64_t canonical_type =
      get_type_id(get_input_type, set_input_type, std::string(name), id, info);
  reinterpret_cast<VariableDatabaseData *>(info->toolProvidedInfo)
      ->canonical_id = canonical_type;
 reinterpret_cast<VariableDatabaseData *>(info->toolProvidedInfo)
      ->name = reinterpret_cast<char*>(malloc(sizeof(char) * 64)); 
  strncpy(reinterpret_cast<VariableDatabaseData *>(info->toolProvidedInfo)
      ->name , name, 64);

}

void
kokkosp_declare_output_type(const char *name, const size_t id,
                            Kokkos::Tools::Experimental::VariableInfo *info) {
  info->toolProvidedInfo = new VariableDatabaseData{};
  associate_candidates(id, info);
  int64_t canonical_type = get_type_id(get_output_type, set_output_type,
                                       std::string(name), id, info);
  reinterpret_cast<VariableDatabaseData *>(info->toolProvidedInfo)
      ->canonical_id = canonical_type;
 reinterpret_cast<VariableDatabaseData *>(info->toolProvidedInfo)
      ->name = reinterpret_cast<char*>(malloc(sizeof(char) * 64)); 
  strncpy(reinterpret_cast<VariableDatabaseData *>(info->toolProvidedInfo)
      ->name , name, 64);
}

using Kokkos::Tools::Experimental::VariableValue;


bool operator<(const contextDescription &l, const contextDescription &r) {
  return l.id < r.id;
}

std::set<contextDescription> context_data;

int64_t make_search_problem(variableSet &variables, tuningData &data) {
  int niv = variables.num_input_variables;
  int nov = variables.num_variables - niv;
  int64_t problem_id = ++num_problems;
  data.problem_id = problem_id;
  bind_statement(insert_search_problem, problem_id, niv, nov);
  sqlite3_step(insert_search_problem);
  sqlite3_reset(insert_search_problem);
  for (int x = 0; x < niv; ++x) {
    bind_statement(insert_problem_input, problem_id, variables.variable_ids[x], x);
    sqlite3_step(insert_problem_input);
    sqlite3_reset(insert_problem_input);
    // add a problem input
  }
  for (int x = niv; x < variables.num_variables; ++x) {
    bind_statement(insert_problem_output, problem_id,
                   variables.variable_ids[x], (x-niv));
    sqlite3_step(insert_problem_output);
    sqlite3_reset(insert_problem_output);
    // add a problem output
  }
  // bind_statement(insert_search_problem, num_problems,
  // FLAG MONDAY START HERE
  return problem_id;
}
int64_t get_problem_id(variableSet &variables, tuningData &data) {
  int niv = variables.num_input_variables;
  int nov = variables.num_variables - niv;

  std::string input_string;
  std::string output_string;

  for (int x = niv - 1; x >= 1; --x) {
    input_string += std::to_string(variables.variable_ids[x]) + ",";
  }
  input_string += std::to_string(variables.variable_ids[0]);

  for (int x = variables.num_variables - 1; x >= niv + 1; --x) {
    output_string += std::to_string(variables.variable_ids[x]) + ",";
  }
  output_string += std::to_string(variables.variable_ids[niv]);
  bind_statement(get_search_problem, input_string, output_string);
  int status = sqlite3_step(get_search_problem);
  if (status == SQLITE_ROW) {
    int64_t problem_id = sqlite3_column_int64(get_search_problem, 0);
    data.problem_id = problem_id;
    sqlite3_reset(get_search_problem);
    return problem_id;

  } else if (status == SQLITE_DONE) {
    sqlite3_reset(get_search_problem);
    auto id = make_search_problem(variables, data);
    data.problem_id = id;
    return id;
  } else {
    // std::cout << "[get_problem_id] error ["<<status<<"]\n";
  }
  sqlite3_reset(get_search_problem);
  return -1;
}

// sqlite3_exec(db,
//              "CREATE TABLE IF NOT EXISTS trial_values(trial_id int, "
//              "variable_id int, discrete_result int, real_result real)",
//              nullptr, nullptr, const_cast<char **>(&data));
void flush_buffer(variableSet &variables, tuningData &buffer) {
  for (int trial = 0; trial < buffer.num_trials; ++trial) {
    
    int64_t trial_num = ++num_trials;
    int64_t problem_id = buffer.problem_id;
    float result = buffer.data[trial].result;
    bind_statement(insert_trial_data, trial_num, problem_id, result);
    sqlite3_step(insert_trial_data);
    sqlite3_reset(insert_trial_data);
    for (int variable = 0; variable < variables.num_variables; ++variable) {
      VariableDatabaseData* dbdat = reinterpret_cast<VariableDatabaseData*>(buffer.data[trial].values[variable].metadata->toolProvidedInfo);
      auto real_id = dbdat->canonical_id;
            int status = SQLITE_OK;
      switch (buffer.data[trial].values[variable].metadata->type) {
      case ValueType::kokkos_value_double:
        bind_statement(insert_trial_values, trial_num,
                       real_id,
                       std::nullptr_t{},
                       buffer.data[trial].values[variable].value.double_value);
       status = sqlite3_step(insert_trial_values);
        sqlite3_reset(insert_trial_values);
        break;
      case ValueType::kokkos_value_int64:
        if(reinterpret_cast<VariableDatabaseData *>(buffer.data[trial].values[variable].metadata->toolProvidedInfo)->canonical_id == 3){
          //std::cout << "Insert on "<<buffer.data[trial].values[variable].value.int_value << std::endl;
        }
        bind_statement(insert_trial_values, trial_num,
                       real_id,

                       buffer.data[trial].values[variable].value.int_value,
                       std::nullptr_t{});
        status = sqlite3_step(insert_trial_values);
  if(status==SQLITE_DONE){
  } else {
    std::cout << "[inserting int] [" << trial_num <<"," << buffer.data[trial].values[variable].type_id  <<","<<buffer.data[trial].values[variable].value.int_value<< "] error "<<status<<"?" << std::endl;
  }
        sqlite3_reset(insert_trial_values);
        break;
      case ValueType::kokkos_value_string:
        bind_statement(
            insert_trial_values, trial_num,
            real_id,
            int64_t(std::hash<std::string>{}(
                buffer.data[trial].values[variable].value.string_value)),
            std::nullptr_t{});
        sqlite3_step(insert_trial_values);
        sqlite3_reset(insert_trial_values);
        break;
      }
    }
  }
}

std::map<variableSet, tuningData> data_repo;
std::map<size_t, dataSet *> live_contexts;
void kokkosp_request_values(size_t context_id,
                                       size_t num_context_variables,
                                       VariableValue *context_values,
                                       size_t num_tuning_variables,
                                       VariableValue *tuning_values) {

  variableSet set;
  set.num_variables = num_context_variables + num_tuning_variables;
  set.num_input_variables = num_context_variables;
  int index = 0;
  for (int x = 0; x < num_context_variables; ++x) {
    VariableDatabaseData *database_info =
        reinterpret_cast<VariableDatabaseData *>(
            context_values[x].metadata->toolProvidedInfo);
    auto maxcmp = cmp(context_values[x].metadata->type,context_values[x].value,database_info->maximum); 
    if(maxcmp>0){
      database_info->maximum = context_values[x].value;
    }
    set.variable_ids[index++] = database_info->canonical_id;
  }
  for (int x = 0; x < num_tuning_variables; ++x) {
    VariableDatabaseData *database_info =
        reinterpret_cast<VariableDatabaseData *>(
            tuning_values[x].metadata->toolProvidedInfo);
    set.variable_ids[index++] = database_info->canonical_id;
  }
  auto &tuning_data = data_repo[set];
  if (tuning_data.problem_size == 0) {
    int64_t problem_size = 1;
    for (int x = 0; x < num_tuning_variables; ++x) {
      auto *database_info = reinterpret_cast<VariableDatabaseData *>(
          tuning_values[x].metadata->toolProvidedInfo);
      problem_size *= database_info->candidate_set_size;
    }
    tuning_data.problem_size = problem_size;
    int64_t id = get_problem_id(set, tuning_data);
    data_repo[set].problem_id = id;
  }

  if (tuning_data.num_trials == tuning_data_buffer_size) {
    flush_buffer(set, tuning_data);
    tuning_data.num_trials = 0;
  }

  int64_t trial_num = tuning_data.num_trials++;

  tuning_data.data[trial_num] = dataSet{0.0f};
  tuning_data.data[trial_num].start_time = clock_type::now();
  for (int x = 0; x < num_context_variables; ++x) {
    tuning_data.data[trial_num].values[x] = context_values[x];
  }
  int64_t tuning_choice = tuning_data.choice_index++ % tuning_data.problem_size;
  tuning_data.choice_index %= tuning_data.problem_size;
  for (int x = 0; x < num_tuning_variables; ++x) {
    auto *database_info = reinterpret_cast<VariableDatabaseData *>(
        tuning_values[x].metadata->toolProvidedInfo);
    int64_t local_choice = tuning_choice % database_info->candidate_set_size;
    auto canonical_metadata = tuning_values[x].metadata;
    tuning_values[x] = database_info->candidate_values[local_choice];
    tuning_values[x].metadata = canonical_metadata;
    tuning_data.data[trial_num].values[num_context_variables + x] =
        tuning_values[x];
    tuning_choice /= database_info->candidate_set_size;
  }
  live_contexts[context_id] = &tuning_data.data[trial_num];
}
void kokkosp_finalize_library() {
  for (auto &pr : data_repo) {
    auto vars = pr.first;
    auto data = pr.second;
    flush_buffer(vars, data);
  }
}
void kokkosp_end_context(size_t context_id) {
  auto *data_set = live_contexts[context_id];
  if(data_set){
    auto end_time = clock_type::now();
    auto time_diff = end_time - data_set->start_time;
    data_set->result = time_diff.count();
    live_contexts.erase(context_id);
  }
}

namespace impl {
template <> void bind_statement(sqlite3_stmt *stmt, int index) {}

} // end namespace impl
} // namespace explorer
} // namespace labrador
