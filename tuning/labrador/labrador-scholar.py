import sqlite3
import itertools
from enum import Enum
import math
import os

class ValueType(Enum):
  boolean        = 0 
  integer        = 1
  floating_point = 2
  text           = 3

class StatisticalCategory(Enum):
  categorical = 0
  ordinal     = 1
  interval    = 2
  ratio       = 3

database_name = "tuning_db.db" if os.getenv("LABRADOR_DATABASE") is None else os.getenv("LABRADOR_DATABASE")
conn = sqlite3.connect(database_name)

class EmptySpace():
  def __init__(self):
    pass

class CategoricalSpace():
    def __init__(self, initializer = []):
      self.categories = initializer

class OrdinalSpace():
    def __init__(self, initializer = []):
      self.categories = initializer

class IntervalSpace():
    def __init__(self, minval = None, maxval= None ):
      self.min = minval
      self.max = maxval

class RatioSpace():
    def __init__(self, minval = None, maxval= None ):
      self.min = minval
      self.max = maxval


class Sliceable():
  def __init__(self, space, slices = 10):
    self.space = space
    self.slices = slices
fetcher = conn.cursor()

fetcher.execute("SELECT id FROM problem_descriptions")

my_list = fetcher.fetchall()

problem_ids = [x[0] for x in my_list]

problem_descriptions = {}

variable_descriptions = {}

fetcher.execute("SELECT * FROM input_types")

input_types = fetcher.fetchall()

def make_categorical(input_set):
  return CategoricalSpace(input_set)
def make_ordinal(input_set):
  return OrdinalSpace(input_set)
def make_interval(input_set):
  mi = min(input_set)
  ma = max(input_set)
  if(mi==ma):
    return CategoricalSpace([mi])
  return IntervalSpace(mi,ma)
def make_ratio(input_set):
  mi = min(input_set)
  ma = max(input_set)
  if(mi==ma):
    return CategoricalSpace([mi])
  return RatioSpace(mi,ma)

maker_switch = {
  0 : make_categorical,
  1 : make_ordinal,
  2 : make_interval,
  3 : make_ratio
}

for row in input_types:
  id,name,value_type,statistical_category=row
  variable_descriptions[id] = {"id": id, "name": name, "value_type" : ValueType(value_type), "statistical_category" : StatisticalCategory(statistical_category),"io":"input", "search_space" : None }
  fetcher.execute("SELECT * FROM trial_values WHERE variable_id=?",(id,))
  variable_data = fetcher.fetchall() 
  variable_set = [x[2] if x[2] is not None else x[3] for x in variable_data]
  variable_descriptions[id]["search_space"] = maker_switch[statistical_category](variable_set)
fetcher.execute("SELECT * FROM output_types")

output_types = fetcher.fetchall()

for row in output_types:
  id,name,value_type,statistical_category=row
  variable_descriptions[id] = {"id": id, "name": name, "value_type" : ValueType(value_type), "statistical_category" : StatisticalCategory(statistical_category),"io":"output", "candidates": []}
  fetcher.execute("SELECT * FROM candidate_sets WHERE id=?",(id,))
  candidates = fetcher.fetchall()
  for candidate in candidates:
    _,continuous,discrete = candidate
    variable_descriptions[id]["candidates"].append(discrete if continuous is None else continuous)
  
for id in problem_ids:
  problem_descriptions[id] = {"inputs": [], "outputs": [], "instances": []}
  fetcher.execute("SELECT variable_id FROM problem_inputs WHERE problem_inputs.problem_id=? ORDER BY variable_index", (id,))
  inputs = fetcher.fetchall()
  for x in inputs:
    problem_descriptions[id]["inputs"].append(x[0])
  fetcher.execute("SELECT variable_id FROM problem_outputs WHERE problem_outputs.problem_id=? ORDER BY variable_index", (id,))
  outputs = fetcher.fetchall()
  for x in outputs:
    problem_descriptions[id]["outputs"].append(x[0])
def slice_space(space):
  if(type(space)==Sliceable):
    return space
  else:
    space_to_slice = Sliceable(space,10)
  if(type(space_to_slice.space == CategoricalSpace)):
    return space_to_slice
  if(type(space_to_slice.space == OrdinalSpace)):
    values = []
    sliced_distance = (space_to_slice.space.max - space_to_slice.space.min) / space_to_slice.slices
    for i in range(space_to_slice.slices):
      values.append(space_to_slice.space.min + (sliced_distance * i))
    return CategoricalSpace(values)
  elif(type(space_to_slice.space == RatioSpace)):
    values = []
    sliced_distance = (math.log(space_to_slice.space.max) - math.log(space_to_slice.space.min)) / space_to_slice.slices
    for i in range(space_to_slice.slices):
      values.append(space_to_slice.space.min * math.exp(i))
    return CategoricalSpace(values)

def num_slices(space):
  if(type(space) is CategoricalSpace):
    return len(space.categories)
  if(type(space) is Sliceable):
    return space.num_slices
  return len(slice_space(space).categories)
import pdb
def combine_spaces(space_one,space_two):
  if (type(space_one)==EmptySpace):    
    return space_two
  if (type(space_two)==EmptySpace):    
    return space_one
  if (type(space_one) is not CategoricalSpace):
    cs1 = slice_space(space_one)  
  else:
    cs1 = space_one
  if (type(space_two) is not CategoricalSpace):
    cs2 = slice_space(space_two)  
  else:
    cs2 = space_two
  #return CategoricalSpace([x for x in itertools.product(cs1.categories, cs2.categories)])
  out_categories = []
  for l in cs1.categories:
    if ((type(l) is not tuple)):
      fl = (l,)
    else:
      fl = l
    for r in cs2.categories:
      if ((type(r) is not tuple)):
        fr = (r,)
      else:
        fr = r
      out_categories.append(fl+fr)
  return CategoricalSpace(out_categories)

answers = {}
best_trial = {}
for problem_id,problem in problem_descriptions.items():
  best_trial[problem_id] = {}
  merged_space = EmptySpace()
  for variable in problem["inputs"]:
    merged_space = combine_spaces(merged_space, variable_descriptions[variable]["search_space"])
  problem["sliced_space"] = merged_space
  for category in merged_space.categories: 
    query_string = "SELECT * FROM ("
    query_string += "SELECT COUNT(),MIN(trials.trial_id), trials.problem_id, AVG(trials.result) AS avg_result, "
    num_inputs = len(problem["inputs"])
    num_outputs = len(problem["outputs"])
    for index,variable in enumerate(problem["inputs"]):
      query_string += "dv%s.%s AS variable_value%s%s" % (index, "value%s" % (index,), index, ", ")
    group_by = ""
    for oindex,variable in enumerate(problem["outputs"]):
      index = num_inputs + oindex
      group_by += "variable_value%s%s" % (index, "," if oindex is not (len(problem["outputs"])-1) else "")
      query_string += "dv%s.%s AS variable_value%s%s" % (index, "value%s" % (index,), index, ", " if oindex is not (num_outputs-1) else " ")
    query_string += " FROM trials "
    for index,variable in enumerate(problem["inputs"]):
      higher = [x for x in slice_space(variable_descriptions[variable]["search_space"]).space.categories if x > category[index]]
      query_string+="LEFT JOIN (SELECT trial_id AS tid%s, %s AS value%s  FROM trial_values WHERE variable_id=%s AND " % (index, "discrete_result", index, variable)
      if higher:
        next_higher = min(higher)
        query_string += " %s >= %s and %s < %s " % ("discrete_result", category[index], "discrete_result", next_higher)
      else:
        query_string += " %s >= %s " % ("discrete_result", category[index])
      query_string += ") dv%s ON dv%s.%s = trials.trial_id " % (index, index, "tid%s" % (index))
    for oindex,variable in enumerate(problem["outputs"]):
      index = num_inputs + oindex
      query_string+="LEFT JOIN (SELECT trial_id AS tid%s, %s AS value%s  FROM trial_values WHERE variable_id=%s " % (index, "discrete_result", index, variable)
      query_string += ") dv%s ON dv%s.%s = trials.trial_id " % (index, index, "tid%s" % (index))
    query_string += " GROUP BY %s ) concretized WHERE " % (group_by,)
    for index,variable in enumerate(problem["inputs"]):
      query_string += "concretized.variable_value%s NOT NULL AND " % (index)
    query_string += " (1=1) ORDER BY avg_result"
    fetcher.execute(query_string)
    best_string = fetcher.fetchall()
    if(len(best_string) > 0):
      best_trial[problem_id][category] = best_string[0][1]
      #print (query_string)
      #print(best_string[0][1])
    else:
      best_trial[problem_id][category] = None
import string      
import random
def make_variable_value(vid, value):
  return "{ %s, make_value_union(%s) }" % (vid, value,)
  boolean        = 0 
  integer        = 1
  floating_point = 2
  text           = 3
type_constructor_map = {
  ValueType.boolean : "bool",
  ValueType.integer : "int64_t",
  ValueType.floating_point : "double",
  ValueType.text : "std::string"
}
type_extractor_map = {
  ValueType.boolean : ".bool_value",
  ValueType.integer : ".int_value",
  ValueType.floating_point : ".double_value",
  ValueType.text : ".string_value"
}
code = """
#include <impl/Kokkos_Profiling_Interface.hpp>
#include <iostream>
#include <cstring>
using ValueUnion = decltype(std::declval<Kokkos::Tools::Experimental::VariableValue>().value);

void warn_unseen(){
  std::cout << "[librador_retriever] is seeing an unfamiliar value\\n";
}

ValueUnion make_value_union(bool in){
  ValueUnion ret;
  ret.bool_value = in;
  return ret;
}
ValueUnion make_value_union(int64_t in){
  ValueUnion ret;
  ret.int_value = in;
  return ret;
}
ValueUnion make_value_union(double in){
  ValueUnion ret;
  ret.double_value = in;
  return ret;
}
ValueUnion make_value_union(const char* in){
  ValueUnion ret;
  ret.string_value = in;
  return ret;
}

struct CanonicalIdHolder {
  size_t id;
};

extern "C" void
kokkosp_declare_input_type(const char *name, const size_t id,
                           Kokkos::Tools::Experimental::VariableInfo &info) {
"""

for index,variable in variable_descriptions.items():
  if variable["io"] is "input":
    code += "  if(strncmp(name,\"%s\",256)==0) {\n" % variable["name"]
    code += "    info.toolProvidedInfo = new CanonicalIdHolder { %s };\n" % (variable["id"])
    code += "  }\n"

code+="""
}
extern "C" void
kokkosp_declare_output_type(const char *name, const size_t id,
                            Kokkos::Tools::Experimental::VariableInfo &info) {
}
"""
for problem_id,problem in problem_descriptions.items():
  num_inputs = len(problem["inputs"])
  num_outputs = len(problem["outputs"])
  tuner_name = ""
  for x in range(25):
    tuner_name += random.choice(string.ascii_lowercase)
  problem["tuner_name"] = tuner_name
  choice_array_name = "%s_values" % (tuner_name)
  code += "Kokkos::Tools::Experimental::VariableValue %s [][%s] = { \n" % (choice_array_name,num_outputs,)
  sizes = []
  spaces = []
  for inp in problem["inputs"]:
    space = slice_space(variable_descriptions[inp]["search_space"])
    spaces.append(space)
    sizes.append(len(space.space.categories))
  num_categories = len(problem["sliced_space"].categories)
  #print(problem["sliced_space"].categories)
  for category_index, category in enumerate(problem["sliced_space"].categories):
    code += " { "
    best_option = best_trial[problem_id][category]
    if best_option is not None:
      for variable_index,output in enumerate(problem["outputs"]):
        fetcher.execute("SELECT discrete_result FROM trial_values WHERE trial_id=? AND variable_id=?" , (best_option, output))
        value = fetcher.fetchone()[0]
        code += make_variable_value(output, "%s(%s) " %(type_constructor_map[variable_descriptions[output]["value_type"]], value))
    else:
      code += make_variable_value(0, "int64_t(0)")
    code += " }%s " % (' ' if category_index is (num_categories-1) else ',')
  code += " }; \n "
  code += "Kokkos::Tools::Experimental::VariableValue* value_for_%s(Kokkos::Tools::Experimental::VariableValue* in) {\n"% (tuner_name,)

  for input_index, input_id in enumerate(problem["inputs"]):
    
    choice_variable = "  choice_%s" % (input_index,)
    holder_variable = "  holder_%s" % (input_index,)
    code += "  int %s;\n" % (choice_variable,)
    search_space = variable_descriptions[input_id]["search_space"]
    sliced = slice_space(search_space)
    type_id = variable_descriptions[input_id]["value_type"]
    if type(search_space) is CategoricalSpace or type(search_space) is OrdinalSpace:
      code += "  switch(in[%s].value%s) { " % (input_index, type_extractor_map[type_id],)
      for category_index,category in enumerate(search_space.categories):
        code += "    case %s :" % (category,)
        code += "      %s = %s;" % (choice_variable,category_index)
        code += "      break;"
        pass
      code += "  }"
    else:
      code += "  auto %s = in[%s].value%s;\n" % (holder_variable, input_index, type_extractor_map[type_id],) 
      if type(search_space) is IntervalSpace:
         minval = sliced.categories[0]
         maxval = sliced.categories[-1]
         step = (sliced.categories[1] - sliced.categories[0])
         code += "%s = int((%s - %s) / %s);\n" % (choice_variable, holder_variable, minval, step)
         code += "  if((%s < 0) || (%s >= %s)) {\n" % (choice_variable, choice_variable, len(sliced.categories))
         code += "    warn_unseen();\n"
         code += "    return nullptr;\n"
         code += "  }\n"
         
      else:
         # if I'm a ratio space
         pass
      pass
  iter_list =[x for x in enumerate(problem["inputs"])]
  iter_list.reverse()
  code += "  auto stride = 1;"
  code += "  auto returned_choice = 0;"
  for input_index, input_id in iter_list:
    search_space = variable_descriptions[input_id]["search_space"]
    sliced = slice_space(search_space)
    choice_variable = "  choice_%s" % (input_index,)
    code += "  returned_choice += stride * %s;\n" % (choice_variable,)
    code += "  stride *= %s;\n" % (len(sliced.space.categories))
  code += "  return %s[returned_choice];\n" % (choice_array_name,)
  code += "}\n"
code += "Kokkos::Tools::Experimental::VariableValue* get_output(size_t count, Kokkos::Tools::Experimental::VariableValue* in) {\n"
for problem_id,problem in problem_descriptions.items():
  tuner_name = problem["tuner_name"] 
  code += "  if (%s == %s) {\n" % ("count",len(problem["inputs"]),)
  code += "    if("    
  for index,inp in enumerate(problem["inputs"]):
    code += "(reinterpret_cast<CanonicalIdHolder*>(in[%s].metadata->toolProvidedInfo)->id ==%s) &&" % (index, inp,)
  code += " true ) {\n"
  code += "      auto ret = value_for_%s(in);\n" % (tuner_name,)
  code+=  "      if(ret[0].id == 0) {\n"
  code+=  "        return nullptr;\n"
  code+=  "      }\n";
  code+=  "      return ret;\n";
         
  code += "    }\n"
  code += "  }\n"
  code += "  return nullptr;"
  code += "}\n"

code += """
extern "C" void kokkosp_request_values(size_t context_id,
                                       size_t num_context_variables,
                                       Kokkos::Tools::Experimental::VariableValue *context_values,
                                       size_t num_tuning_variables,
                                       Kokkos::Tools::Experimental::VariableValue *tuning_values) {
"""
code += "  auto result = get_output(num_context_variables, context_values);\n" 
code += "  if(result != nullptr) {\n" 
code += "    for(int x = 0; x< num_tuning_variables; ++x){\n" 
code += "      tuning_values[x] = result[x];\n"
code += "    }\n" 
code += "    tuning_values = result;\n" 
code += "  }\n" 
code+= "}"
print(code)
