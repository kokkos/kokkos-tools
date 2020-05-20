#include <cstddef>
#include <cstring>
#include <impl/Kokkos_Profiling_Interface.hpp>
#include <iostream>
#include <sqlite3.h>
#include <string>
using db_id_type = int64_t;

sqlite3 *tool_db;
sqlite3_stmt *get_input_type;
sqlite3_stmt *get_output_type;
sqlite3_stmt *set_input_type;
sqlite3_stmt *set_output_type;
sqlite3_stmt *insert_candidate_set_entry;
sqlite3_stmt *insert_candidate_group;
int64_t num_types;
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
               "variable_id int)",
               nullptr, nullptr, const_cast<char **>(&data));
  sqlite3_exec(db,
               "CREATE TABLE IF NOT EXISTS problem_outputs(problem_id int, "
               "variable_id int)",
               nullptr, nullptr, const_cast<char **>(&data));
  sqlite3_exec(db,
               "CREATE TABLE IF NOT EXISTS trials(problem_id int, result real)",
               nullptr, nullptr, const_cast<char **>(&data));
  sqlite3_exec(db,
               "CREATE TABLE IF NOT EXISTS candidate_sets(id int, "
               "continuous_value real, discrete_value int)",
               nullptr, nullptr, const_cast<char **>(&data));
  sqlite3_exec(
      db,
      "CREATE TABLE IF NOT EXISTS candidate_groups(id int PRIMARY KEY, "
      "continuous_lower real, discrete_lower int, continuous_upper real, "
      "discrete_upper int, continuous_step real, discrete_step int, open_lower "
      "int NOT NULL, open_upper int NOT NULL, is_discrete int)",
      nullptr, nullptr, const_cast<char **>(&data));
  sqlite3_exec(db,
               "CREATE TABLE IF NOT EXISTS trial_values(problem_id int, "
               "variable_id int, discrete_result int, real_result real)",
               nullptr, nullptr, const_cast<char **>(&data));
  insert_candidate_set_entry =
      prepare_statement(db, "INSERT INTO candidate_sets VALUES(?,?,?)");
  insert_candidate_group = prepare_statement(
      db, "INSERT INTO candidate_groups VALUES (?,?,?,?,?,?,?,?,?,?)");
  get_input_type = prepare_statement(
      db, "SELECT id FROM input_types WHERE input_types.name == ?");
  get_output_type = prepare_statement(
      db, "SELECT id FROM output_types WHERE output_types.name == ?");
  set_output_type =
      prepare_statement(db, "INSERT INTO output_types VALUES (?,?,?,?)");
  set_input_type =
      prepare_statement(db, "INSERT INTO input_types VALUES (?,?,?,?)");
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

template <typename... Args>
void bind_statement(sqlite3_stmt *stmt, int index, Args... args);

template <typename Arg, typename... Args>
void bind_statement(sqlite3_stmt *stmt, int index, Arg arg, Args... args) {
  // std::cout << index<<","<<arg<<std::endl;
  bind_arg(stmt, index, arg);
  bind_statement(stmt, index + 1, args...);
}

template <> void bind_statement(sqlite3_stmt *stmt, int index) {}

} // namespace impl

template <typename... Args>
void bind_statement(sqlite3_stmt *stmt, Args... args) {
  // std::cout <<"Binding new statement\n";
  return impl::bind_statement(stmt, 1, args...);
}

extern "C" void kokkosp_init_library(const int,
                                     const uint64_t interface_version,
                                     const uint32_t, void *) {
  sqlite3_open_v2("tuning_db.db", &tool_db,
                  SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
  create_tables(tool_db);
}

using ValueType = Kokkos::Tools::Experimental::ValueType;
using StatisticalCategory = Kokkos::Tools::Experimental::StatisticalCategory;
using CandidateValueType = Kokkos::Tools::Experimental::CandidateValueType;

int64_t id_for_type(ValueType type) {
  switch (type) {
  case ValueType::kokkos_value_boolean:
    return 0;
  case ValueType::kokkos_value_integer:
    return 1;
  case ValueType::kokkos_value_floating_point:
    return 2;
  case ValueType::kokkos_value_text:
    return 3;
  }
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
}
// "CREATE TABLE IF NOT EXISTS candidate_groups(id int PRIMARY KEY, "
// "continuous_lower real, discrete_lower int, continuous_upper real, "
// "discrete_upper int, continuous_step real, discrete_step int, open_lower "
// "int NOT NULL, open_upper int NOT NULL, is_discrete int)",
// nullptr, nullptr, const_cast<char **>(&data));

void make_candidate_range(
    int64_t id, const Kokkos::Tools::Experimental::VariableInfo &info) {
  auto range = info.candidates.range;
  switch (info.type) {
  case ValueType::kokkos_value_integer:
    bind_statement(insert_candidate_group, id, std::nullptr_t{},
                   range.lower.int_value, std::nullptr_t{},
                   range.upper.int_value, std::nullptr_t{},
                   range.step.int_value, range.openLower ? int64_t(1) : 0,
                   range.openUpper ? int64_t(1) : int64_t(0), int64_t(1));
    sqlite3_step(insert_candidate_group);
    sqlite3_reset(insert_candidate_group);
    break;
  case ValueType::kokkos_value_floating_point:
    bind_statement(insert_candidate_group, id, range.lower.double_value,
                   std::nullptr_t{}, range.upper.double_value, std::nullptr_t{},
                   range.step.double_value, std::nullptr_t{},
                   range.openLower ? int64_t(1) : 0,
                   range.openUpper ? int64_t(1) : int64_t(0),
                   (range.step.double_value == 0) ? int64_t(1) : int64_t(0));
    sqlite3_step(insert_candidate_group);
    sqlite3_reset(insert_candidate_group);
    break;
  case ValueType::kokkos_value_boolean:
  case ValueType::kokkos_value_text:
    std::cerr << "The authors of this tool didn't consider the cases of ranges "
                 "of booleans or strings. I guess we could see a range of "
                 "strings being used by biologists, but we don't want to "
                 "encourage that kind of behavior\n";
  }
}

void make_candidate_set(int64_t id,
                        const Kokkos::Tools::Experimental::VariableInfo &info) {

  auto set = info.candidates.set;
  switch (info.type) {
  case ValueType::kokkos_value_integer:
    for (int x = 0; x < set.size; ++x) {
      bind_statement(insert_candidate_set_entry, id, std::nullptr_t{},
                     set.values.int_value[x]);
      sqlite3_step(insert_candidate_set_entry);
      sqlite3_reset(insert_candidate_set_entry);
    }
    break;
  case ValueType::kokkos_value_floating_point:
    for (int x = 0; x < set.size; ++x) {
      bind_statement(insert_candidate_set_entry, id, set.values.double_value[x],
                     std::nullptr_t{});
      sqlite3_step(insert_candidate_set_entry);
      sqlite3_reset(insert_candidate_set_entry);
    }
    break;
  case ValueType::kokkos_value_text:
    for (int x = 0; x < set.size; ++x) {
      bind_statement(insert_candidate_set_entry, id, std::nullptr_t{},
                     int64_t(std::hash<std::string>{}(
                         std::string(set.values.string_value[x]))));
      sqlite3_step(insert_candidate_set_entry);
      sqlite3_reset(insert_candidate_set_entry);
    }
    break;
  case ValueType::kokkos_value_boolean:
    for (int x = 0; x < set.size; ++x) {
      bind_statement(insert_candidate_set_entry, id, std::nullptr_t{},
                     set.values.bool_value[x] ? int64_t(1) : int64_t(0));
      sqlite3_step(insert_candidate_set_entry);
      sqlite3_reset(insert_candidate_set_entry);
    }
    break;
  }
}

db_id_type
insert_type_id(sqlite3_stmt *stmt, const std::string &name, size_t id,
               const Kokkos::Tools::Experimental::VariableInfo &info) {
  int type_id = ++num_types;
  bind_statement(stmt, type_id, name, id_for_type(info.type),
                 id_for_category(info.category));
  int status = sqlite3_step(stmt);
  if (status != SQLITE_DONE) {
    std::cout << "[insert_input_type] insert not successful [" << status << "]"
              << std::endl;
  } else {
    sqlite3_reset(stmt);
  }
  switch (info.valueQuantity) {
  case CandidateValueType::kokkos_value_range:
    make_candidate_range(type_id, info);
    break;
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
                       const Kokkos::Tools::Experimental::VariableInfo &info) {
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

struct VariableDatabaseData {
  int64_t canonical_id;
};

extern "C" void
kokkosp_declare_input_type(const char *name, const size_t id,
                           Kokkos::Tools::Experimental::VariableInfo &info) {
  int64_t canonical_type =
      get_type_id(get_input_type, set_input_type, std::string(name), id, info);
  info.toolProvidedInfo = new VariableDatabaseData{canonical_type};
}

extern "C" void
kokkosp_declare_output_type(const char *name, const size_t id,
                            Kokkos::Tools::Experimental::VariableInfo &info) {
  int64_t canonical_type = get_type_id(get_output_type, set_output_type,
                                       std::string(name), id, info);
  info.toolProvidedInfo = new VariableDatabaseData{canonical_type};
}
