import sqlite3
import itertools
from enum import Enum
import math
import os
import sys
import pdb

class ValueType(Enum):
  boolean        = 0 # unused, legacy
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

liminality = False

if(len(sys.argv) > 1):
  if sys.argv[1] == "1":
    liminality = True

class CategoricalSpace():
    def __init__(self, initializer = []):
      self.categories = initializer
    def __repr__(self):
      return "CategoricalSpace ( %s ) "% (self.categories,)
class OrdinalSpace():
    def __init__(self, initializer = []):
      self.categories = initializer
    def __repr__(self):
      return "OrdinalSpace ( %s ) "% (self.categories,)

class IntervalSpace():
    def __init__(self, minval = None, maxval= None ):
      self.min = minval
      self.max = maxval
      self.interval = (maxval-minval)/10
    def __repr__(self):
      return "IntervalSpace ( %s , %s ) "% (self.min,self.max)

class RatioSpace():
    def __init__(self, minval = None, maxval= None ):
      self.min = minval
      self.max = maxval
      self.interval = (maxval-minval)/10
    def __repr__(self):
      return "RatioSpace ( %s , %s ) "% (self.min,self.max)


class Sliceable():
  def __init__(self, space, slices = 10):
    self.space = space
    self.slices = slices
  def __repr__(self):
    return "Sliceable %s ( %s ) " %(self.slices, repr(self.space))
fetcher = conn.cursor()

fetcher.execute("SELECT id FROM problem_descriptions")

my_list = fetcher.fetchall()

problem_ids = [x[0] for x in my_list]

problem_descriptions = {}

variable_descriptions = {}

fetcher.execute("SELECT * FROM input_types")

input_types = fetcher.fetchall()

def make_categorical(input_set):
  return CategoricalSpace(sorted(list(set(input_set))))
def make_ordinal(input_set):
  return OrdinalSpace(input_set)
def make_interval(input_set):
  mi = min(input_set)
  ma = max(input_set)
  if(mi is ma):
    return CategoricalSpace([mi])
  return IntervalSpace(mi,ma + 1)
def make_ratio(input_set):
  mi = min(input_set)
  ma = max(input_set)
  if(mi is ma):
    return CategoricalSpace([mi])
  return RatioSpace(mi,ma + 1)

maker_switch = {
  StatisticalCategory.categorical : make_categorical,
  StatisticalCategory.ordinal : make_ordinal,
  StatisticalCategory.interval : make_interval,
  StatisticalCategory.ratio : make_ratio
}

categorical_cutoff = 2

for row in input_types:
  id,name,value_type,statistical_category=row
  variable_descriptions[id] = {"id": id, "name": name, "value_type" : ValueType(value_type), "statistical_category" : StatisticalCategory(statistical_category),"io":"input", "search_space" : None, "discrete" : value_type != 2, "is_categorical" : False }
  #fetcher.execute("SELECT * FROM trial_values WHERE variable_id=?",(id,))
  #variable_data = fetcher.fetchall() 
  #variable_set = [x[2] if x[2] is not None else x[3] for x in variable_data]
  #variable_descriptions[id]["search_space"] = maker_switch[statistical_category](variable_set)
  
fetcher.execute("SELECT max(discrete_result),min(discrete_result),max(real_result),min(real_result),COUNT(DISTINCT(discrete_result)),COUNT(DISTINCT(real_result)),variable_id FROM trial_values GROUP BY variable_id",())

extrema_results = fetcher.fetchall()
categorical_set_query = "SELECT DISTINCT real_result,discrete_result,variable_id FROM trial_values WHERE variable_id IN (-1,"
for row in extrema_results:
    vid = row[-1]
    if vid not in variable_descriptions.keys():
        continue
    rowmax = row[0] if row[0] is not None else row[2]
    rowmin = row[1] if row[1] is not None else row[3]
    rowcount = row[2] if row[2] is not None else row[4]
    nominal_statistical_category = variable_descriptions[vid]["statistical_category"]
    is_categorical = (nominal_statistical_category==StatisticalCategory.ordinal) or (nominal_statistical_category==StatisticalCategory.categorical) or (rowcount <= categorical_cutoff)
    
    if is_categorical:
        variable_descriptions[vid]["is_categorical"] = True
        categorical_set_query += (str(vid)+ ",")
    else:
        maker = maker_switch[nominal_statistical_category]
        variable_descriptions[vid]["search_space"] = maker([rowmin,rowmax])
categorical_set_query += "-1)"
fetcher.execute(categorical_set_query)
distinct_rows = fetcher.fetchall()

for row in distinct_rows:
  vid = row[-1]
  if variable_descriptions[vid]["search_space"] is None:
    variable_descriptions[vid]["search_space"] = CategoricalSpace()
  variable_descriptions[vid]["search_space"].categories.append(row[0] if row[0] is not None else row[1])

fetcher.execute("SELECT * FROM output_types")

output_types = fetcher.fetchall()

for row in output_types:
  id,name,value_type,statistical_category=row
  variable_descriptions[id] = {"id": id, "name": name, "value_type" : ValueType(value_type), "statistical_category" : StatisticalCategory(statistical_category),"io":"output", "candidates": [], "discrete" : value_type != 2, "is_categorical" : False }
  fetcher.execute("SELECT * FROM candidate_sets WHERE id=?",(id,))
  candidates = fetcher.fetchall()
  for candidate in candidates:
    _,continuous,discrete = candidate
    variable_descriptions[id]["candidates"].append(discrete if continuous is None else continuous)

#  boolean        = 0 # unused, legacy
#  integer        = 1
#  floating_point = 2
#  text           = 3

type_map  = { ValueType.boolean : "int",
        ValueType.integer : "int",
        ValueType.floating_point : "real",
        ValueType.text : "int"
        }

for id in problem_ids:
  problem_descriptions[id] = {"inputs": [], "outputs": [], "instances": [], "name": None}
  fetcher.execute("SELECT variable_id FROM problem_inputs WHERE problem_inputs.problem_id=? ORDER BY variable_index", (id,))
  inputs = fetcher.fetchall()
  name = "problem_"
  for x in inputs:
    problem_descriptions[id]["inputs"].append(x[0])
    name+=str(x[0])+"_" 
  fetcher.execute("SELECT variable_id FROM problem_outputs WHERE problem_outputs.problem_id=? ORDER BY variable_index", (id,))
  outputs = fetcher.fetchall()
  for x in outputs:
    problem_descriptions[id]["outputs"].append(x[0])
    name+=str(x[0])+"_" 
  problem_descriptions[id]["name"]=name
  creator_creator = "CREATE TABLE IF NOT EXISTS "+name+"(trial_id int, "
  inserter = "INSERT INTO "+name+" values (?,"
  selector = "SELECT trial_id, "
  def get_type_info(variable_info,allow_translations):
      if variable_info["is_categorical"] and allow_translations:
        return "int"
      else:
        return type_map[variable_info["value_type"]]
  for x in inputs:
    vid = x[0]
    creator_creator += "input_"+str(vid)+" "+get_type_info(variable_descriptions[vid],True)+", "
    creator_creator += "input_"+str(vid)+"_raw "+get_type_info(variable_descriptions[vid],False)+", "
    inserter+="?,?,"
    selector +="Null,Null,"
  for x in outputs:
    vid = x[0]
    creator_creator += "output_"+str(vid)+" "+type_map[variable_descriptions[vid]["value_type"]]+", "
    inserter+="?,"
    selector +="Null,"
  creator_creator += "result real)"
  inserter+="?)"
  selector +="result FROM trials WHERE problem_id=?"
  fetcher.execute(selector,(id,))
  rows = fetcher.fetchall()
  fetcher.execute(creator_creator)
  conn.commit()
  fetcher.executemany(inserter,rows)
  conn.commit()

  #variable_descriptions[id] = {"id": id, "name": name, "value_type" : ValueType(value_type), "statistical_category" : StatisticalCategory(statistical_category),"io":"input", "search_space" : None, "discrete" : value_type is not ValueType.floating_point }

  def process_field(field_description, force_categorical = False):
    value_field = "discrete_result" if field_description["discrete"] else "real_result"
    statistical_category = field_description["statistical_category"]
    if force_categorical or field_description["is_categorical"] :
      return "trial_values."+value_field
    else:
      space = field_description["search_space"]
      interval = space.interval
      return "CAST (((trial_values."+value_field+" - "+ str(space.min) +" )/ "+str(interval)+") AS INT)"
  innames = []
  groupnames = []
  outnames = []
  for x in inputs:
    vid = x[0]
    fieldname = "input_" + str(vid)
    innames.append(fieldname)
    groupnames.append(fieldname)
    innames.append(fieldname+"_raw")
    process = process_field(variable_descriptions[vid])
    raw_process = process_field(variable_descriptions[vid],True)
    updator = "UPDATE "+name+ " SET "+fieldname+" = " + process +" FROM trial_values WHERE trial_values.variable_id=" +str(vid)+" AND trial_values.trial_id="+name+".trial_id"
    fetcher.execute(updator)
    conn.commit()

    updator = "UPDATE "+name+ " SET "+fieldname+"_raw = " + raw_process +" FROM trial_values WHERE trial_values.variable_id=" +str(vid)+" AND trial_values.trial_id="+name+".trial_id"
    fetcher.execute(updator)
    conn.commit()
  for x in outputs:
    vid = x[0]
    fieldname = "output_" + str(vid)
    outnames.append(fieldname)
    process = process_field(variable_descriptions[vid],True)
    updator = "UPDATE "+name+ " SET "+fieldname+" = " + process +" FROM trial_values WHERE trial_values.variable_id=" +str(vid)+" AND trial_values.trial_id="+name+".trial_id"
    fetcher.execute(updator)
    conn.commit()
  categorical_inputs = []
  other_inputs = []
  categorical_indices = []
  other_indices = []
  def index_for_analysis(name,offset=0):
    pname = name[name.find("_")+1:]
    return offset + int(pname[:pname.find("_")])
  for field_index,x in enumerate(inputs):
    vid = x[0]
    description = variable_descriptions[vid]
    fieldname = "input_"+str(vid)+"_raw"
    category = description["is_categorical"]
    if category:
        categorical_inputs.append(fieldname)
        categorical_indices.append(field_index)
    else:
        other_inputs.append(fieldname)
        other_indices.append(field_index)
  problem_descriptions[id]["analysis_sequence"] = []
  problem_descriptions[id]["analysis_varinfo"] = []
  for item in categorical_indices:
    problem_descriptions[id]["analysis_sequence"].append(item)
  for item in categorical_inputs:
    problem_descriptions[id]["analysis_varinfo"].append(index_for_analysis(item))
  for item in other_indices:
    problem_descriptions[id]["analysis_sequence"].append(item)
  for item in other_inputs:
    problem_descriptions[id]["analysis_varinfo"].append(index_for_analysis(item))
  orderby = "ORDER BY "+",".join([x[:x.find("_raw")] for x in categorical_inputs])+","+",".join([x[:x.find("_raw")] for x in other_inputs])
  # if a problem lacks categorical or non-categorical inputs, the above can lead to commas at start or end
  if orderby[0] == ',':
      orderby = orderby[1:]
  if orderby[-1] == ',':
      orderby = orderby[:-1]
  analyzer = "SELECT "+",".join(outnames)+","+",".join(innames)+",min(avgtime) FROM (select avg(result) as avgtime, "+",".join(innames)+","+",".join(outnames)+" FROM "+name+" GROUP BY "+",".join(groupnames)+","+",".join(outnames)+") as dogs GROUP BY "+",".join(groupnames)+ " " +orderby
  fetcher.execute(analyzer)
  problem_descriptions[id]["solution"] = fetcher.fetchall()
  problem_descriptions[id]["analysis_query"] = analyzer
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
  ValueType.integer : ".int_value",
  ValueType.floating_point : ".double_value",
  ValueType.text : ".string_value"
}

def get_value_extractor(index,tn):
 st = "in[%s].value%s" % (index, type_extractor_map[tn])
 if tn is ValueType.text:
     st = "int64_t(std::hash<std::string>{}(std::string(" + st +")))"
 return st
code = """
#include <impl/Kokkos_Profiling_Interface.hpp>
#include "labrador-exploration.hpp"
#include <iostream>
#include <cstring>

using namespace labrador::explorer;

using ValueUnion = decltype(std::declval<Kokkos::Tools::Experimental::VariableValue>().value);


template<typename Arg1, typename Arg2>
void warn_unseen_min(Kokkos::Tools::Experimental::VariableValue var, Arg1 held, Arg2 min){
  //std::cout << "[liblabrador_retriever] is seeing an unfamiliar value for "<< reinterpret_cast<VariableDatabaseData*>(var.metadata->toolProvidedInfo)->name << ", provided value is "<< held <<", but minimum was "<< min <<"\\n";
}

template<typename Arg1, typename Arg2>
void warn_unseen_max(Kokkos::Tools::Experimental::VariableValue var, Arg1 held, Arg2 max){
  //std::cout << "[liblabrador_retriever] is seeing an unfamiliar value for "<< reinterpret_cast<VariableDatabaseData*>(var.metadata->toolProvidedInfo)->name << ", provided value is "<< held <<", but maximum was "<< max <<"\\n";
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
  strncpy(ret.string_value,in,63);
  return ret;
}

extern "C" void kokkosp_end_context(size_t context_id){
  labrador::explorer::kokkosp_end_context(context_id);
}

extern "C" void
kokkosp_declare_input_type(const char *name, const size_t id,
                           Kokkos::Tools::Experimental::VariableInfo *info) {
"""

if not liminality:
  for index,variable in variable_descriptions.items():
    if variable["io"] == "input":
      code += "  if(strncmp(name,\"%s\",256)==0) {\n" % variable["name"]
      code += "    info->toolProvidedInfo = new VariableDatabaseData { %s, \"%s\" };\n" % (variable["id"], variable["name"])
      code += "  }\n"
else:
  code+="""
  labrador::explorer::kokkosp_declare_input_type(name, id,info);
  """
code+="""
}
extern "C" void
kokkosp_declare_output_type(const char *name, const size_t id,
                            Kokkos::Tools::Experimental::VariableInfo *info) {
			    """
if not liminality:
  for index,variable in variable_descriptions.items():
    if variable["io"] == "output":
      code += "  if(strncmp(name,\"%s\",256)==0) {\n" % variable["name"]
      code += "    info->toolProvidedInfo = new VariableDatabaseData { %s, \"%s\" };\n" % (variable["id"], variable["name"])
      code += "  }\n"
else:
  code+="""
  labrador::explorer::kokkosp_declare_output_type(name, id,info);
  """
code+= "}\n"

def disambiguate_variable_value(raw_value, variable_info):
  typeinfo = variable_info["value_type"]
  if typeinfo == ValueType.floating_point:
    return "double("+str(raw_value)+")"
  if (typeinfo == ValueType.integer) or (typeinfo == ValueType.text):
    return "int64_t("+str(raw_value)+")"
  pass
for problem_id,problem in problem_descriptions.items():
  num_inputs = len(problem["inputs"])
  num_outputs = len(problem["outputs"])
  map_initializer = {}
  problem_groupings = {}
  tuner_name = ""
  for x in range(25):
    tuner_name += random.choice(string.ascii_lowercase)
  problem["tuner_name"] = tuner_name
  choice_array_name = "%s_values" % (tuner_name)
  code += "Kokkos::Tools::Experimental::VariableValue %s [][%s] = { \n" % (choice_array_name,num_outputs,)
  sizes = []
  spaces = []
  #search_space = problem["sliced_space"]
  answer  = problem["solution"]
  if len(answer) == 0:
      continue
  analysis_sequence = problem["analysis_sequence"]
  analysis_varinfo = problem["analysis_varinfo"]
  analysis_sequence.reverse()
  analysis_varinfo.reverse()
  num_numerical = 0
  for problem_input in analysis_varinfo:
    description = variable_descriptions[problem_input]
    if not description["is_categorical"]:
        num_numerical += 1
    else:
        break
  num_categorical = len(analysis_sequence) - num_numerical
  print(problem["analysis_query"])
  print("AS: "+str(analysis_sequence))
  print("AV: "+str(analysis_varinfo))
  print(answer[0])
  #print("Pre loop")
  measured_indices = set()
  last_category = None
  for answer_line in answer:
    built_category = () 
    for input_index,problem_input in enumerate(analysis_sequence[num_numerical:]):
      measured_index = num_outputs + 2 * (problem_input)
      raw_index = measured_index + 1
      built_category = (answer_line[raw_index],) + built_category
    if (built_category != last_category):
      last_category = built_category
      problem_groupings[built_category] = []
    row = [answer_line[:num_outputs]]
    for input_index,problem_input in enumerate(analysis_sequence[:num_numerical]):
      measured_index = num_outputs + 2 * (problem_input)
      raw_index = measured_index + 1
      row.append(answer_line[measured_index])
    row.reverse()
    problem_groupings[built_category].append(row)
  for categorical_key, numerical_results in problem_groupings.items():
      inorder = analysis_sequence[:num_numerical]
      inorder.reverse()
      varinfo = analysis_varinfo[:num_numerical]
      varinfo.reverse()
      space_size = 1
      mapped_to = "{ "
      output_id = []
      for x in problem["outputs"]:
          output_id.append(x)
      def get_space_size(space):
          if (type(space) is CategoricalSpace or type(space) is OrdinalSpace):
              return len(space.categories)
          return 10 # TODO: smrter
      def make_empty_entry(size):
          retval = " { "
          retval += "make_variable_value(-1,int64_t(-1))"
          for x in range(size-1):
              retval += ", make_variable_value(-1,int64_t(-1))"
          retval += "}"
          return retval
      for item in varinfo:
          space_size *= get_space_size(variable_descriptions[item]["search_space"])
      space_index = 0
      for whole_space_index in range(space_size):
          if(space_index >= len(numerical_results)):
              break
          space_index_value = 0
          multiplier = space_size / get_space_size(variable_descriptions[varinfo[num_numerical-1]]["search_space"])
          for set_index,variable_index in enumerate(varinfo[:num_numerical]):
            oindex = set_index
            space_index_value += multiplier * numerical_results[space_index][oindex]
            multiplier /= get_space_size(variable_descriptions[variable_index]["search_space"])
          #print(space_index_value)
          if whole_space_index != space_index_value:
            mapped_to += make_empty_entry(num_outputs)
          else:
            subentry = "{ "
            subentry += "make_variable_value(" + \
            str(output_id[0])+", "+ \
            disambiguate_variable_value(str(numerical_results[space_index][-1][0]), \
                    variable_descriptions[output_id[0]])+")"
            for x in range(num_outputs-1):
              subentry += ", make_variable_value(" +str(output_id[x+1])+", "+disambiguate_variable_value(str(numerical_results[space_index][-1][x+1]),variable_descriptions[output_id[x+1]])+")"
            subentry += "} "
            space_index += 1
            mapped_to += subentry
          mapped_to +=", "
      mapped_to += make_empty_entry(num_outputs)
      mapped_to += "}"
      map_initializer[categorical_key] = mapped_to
  for key,val in map_initializer.items():
      #analysis_sequence
      #analysis_varinfo
      print(analysis_sequence)
      print(analysis_varinfo)
      cut_analysis_sequence = analysis_sequence[num_numerical:]
      cut_analysis_varinfo = analysis_varinfo[num_categorical:]
      cut_analysis_sequence.reverse()
      cut_analysis_varinfo.reverse()
      
      key_builder = list(zip(cut_analysis_varinfo,key)) 
      # TODONEXT: make as in labrador-retriever.cpp
      first_part = key_builder[0]
      map_key = " { make_variable_value(" + str(first_part[0])+", "+disambiguate_variable_value(str(first_part[1]),variable_descriptions[first_part[0]]) +")"
      if(len(key_builder) > 1):
        for varinfo,value in key_builder[1:]:
            map_key += ", " +" make_variable_value(" + str(varinfo)+", "+disambiguate_variable_value(str(value),variable_descriptions[varinfo]) +")" 
      map_key += " } "
      pdb.set_trace()

     
  #code += "Kokkos::Tools::Experimental::VariableValue* value_for_%s(Kokkos::Tools::Experimental::VariableValue* in) {\n"% (tuner_name,)

  for input_index, input_id in enumerate(problem["inputs"]):
    
    choice_variable = "  choice_%s" % (input_index,)
    holder_variable = "  holder_%s" % (input_index,)
    code += "  int %s;\n" % (choice_variable,)
    search_space = variable_descriptions[input_id]["search_space"]
    sliced = slice_space(search_space)
    type_id = variable_descriptions[input_id]["value_type"]
    if type(search_space) is CategoricalSpace or type(search_space) is OrdinalSpace:
      code += "  switch(%s) {\n" % (get_value_extractor(input_index,type_id),)
      for category_index,category in enumerate(search_space.categories):
        code += "    case %s :\n" % (category,)
        code += "      %s = %s;\n" % (choice_variable,category_index)
        code += "      break;\n"
      code+="""
        default: return nullptr;
      """
      code += "  }"
    else:
      code += "  auto %s = %s;\n" % (holder_variable, get_value_extractor(input_index,type_id),) 
      if type(search_space) is IntervalSpace:
         minval = sliced.categories[0]
         maxval = sliced.categories[-1]
         step = (sliced.categories[1] - sliced.categories[0])
         code += "%s = int((%s - %s) / %s);\n" % (choice_variable, holder_variable, minval, step)
         code += "  if((%s < 0)) {\n" % (choice_variable, )
         code += "    warn_unseen_min(in[%s],%s,%s);\n" % (input_index, holder_variable, minval)
         code += "    return nullptr;\n"
         code += "  }\n"
         code += "  if((%s >= %s)) {\n" % (choice_variable, len(sliced.categories))
         code += "    warn_unseen_max(in[%s],%s,%s);\n" % (input_index, holder_variable, maxval)
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
    tmp = sliced
    if type(sliced) is Sliceable:
      tmp = sliced.space
    code += "  stride *= %s;\n" % (len(tmp.categories))
  code += "  return %s[returned_choice];\n" % (choice_array_name,)
  code += "}\n"
code += "Kokkos::Tools::Experimental::VariableValue* get_output(size_t in_count, size_t out_count, Kokkos::Tools::Experimental::VariableValue* in, Kokkos::Tools::Experimental::VariableValue* out) {\n"
for problem_id,problem in problem_descriptions.items():
  tuner_name = problem["tuner_name"] 
  code += "  if ((%s == %s) && (%s == %s)) {\n" % ("in_count",len(problem["inputs"]),"out_count",len(problem["outputs"]),)
  code += "    if("    
  for index,inp in enumerate(problem["inputs"]):
    code += "(reinterpret_cast<VariableDatabaseData*>(in[%s].metadata->toolProvidedInfo)->canonical_id ==%s) &&" % (index, inp,)
  for index,inp in enumerate(problem["outputs"]):
    code += "(reinterpret_cast<VariableDatabaseData*>(out[%s].metadata->toolProvidedInfo)->canonical_id ==%s) &&" % (index, inp,)
  code += " true ) {\n"
  code += "      auto ret = value_for_%s(in);\n" % (tuner_name,)
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
code += "  auto result = get_output(num_context_variables, num_tuning_variables, context_values, tuning_values);\n" 
code += "  if(result != nullptr) {\n" 
code += "    for(int x = 0; x< num_tuning_variables; ++x){\n" 
code += "      tuning_values[x] = result[x];\n"
code += "    }\n" 
code += "    tuning_values = result;\n" 
code += "  }\n"
if liminality:
  code += """
    else {
      labrador::explorer::kokkosp_request_values(context_id, num_context_variables, context_values, num_tuning_variables, tuning_values);
    }
  """
code+= "}"

if liminality:
  code += """
extern "C" void kokkosp_begin_parallel_for(const char* name,
                                           const uint32_t devID,
                                           uint64_t* kID) {
}

extern "C" void kokkosp_end_parallel_for(const uint64_t kID) {
  //printf("kokkosp_end_parallel_for:%lu::", kID);
}

extern "C" void kokkosp_begin_parallel_scan(const char* name,
                                            const uint32_t devID,
                                            uint64_t* kID) {
  //printf("kokkosp_begin_parallel_scan:%s:%u:%lu::", name, devID, *kID);
}

extern "C" void kokkosp_end_parallel_scan(const uint64_t kID) {
  //printf("kokkosp_end_parallel_scan:%lu::", kID);
}

extern "C" void kokkosp_begin_parallel_reduce(const char* name,
                                              const uint32_t devID,
                                              uint64_t* kID) {
  //printf("kokkosp_begin_parallel_reduce:%s:%u:%lu::", name, devID, *kID);
}

extern "C" void kokkosp_end_parallel_reduce(const uint64_t kID) {
  //printf("kokkosp_end_parallel_reduce:%lu::", kID);
}

extern "C" void kokkosp_init_library(const int a1,
                                     const uint64_t a2,
                                     const uint32_t a3, void * a4) {
  labrador::explorer::kokkosp_init_library(a1,a2,a3,a4);
}

extern "C" void kokkosp_finalize_library(){
  labrador::explorer::kokkosp_finalize_library();
}
  """

print(code)
