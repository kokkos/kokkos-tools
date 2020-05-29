import sqlite3
import itertools
from enum import Enum
from sklearn.tree import DecisionTreeClassifier, plot_tree
import math

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

conn = sqlite3.connect("tuning_db.db")
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
  fetcher.execute("SELECT variable_id FROM problem_inputs WHERE problem_inputs.problem_id=?", (id,))
  inputs = fetcher.fetchall()
  for x in inputs:
    problem_descriptions[id]["inputs"].append(x[0])
  fetcher.execute("SELECT variable_id FROM problem_outputs WHERE problem_outputs.problem_id=?", (id,))
  outputs = fetcher.fetchall()
  for x in outputs:
    problem_descriptions[id]["outputs"].append(x[0])

def slice_space(space):
  if(type(space)==Sliceable):
    space_to_slice = space
  else:
    space_to_slice = Sliceable(space,10)
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

for problem_id,problem in problem_descriptions.items():
  merged_space = EmptySpace()
  for variable in problem["inputs"]:
    print(str(variable_descriptions[variable]["search_space"].min/variable_descriptions[variable]["search_space"].max))
    merged_space = combine_spaces(merged_space, variable_descriptions[variable]["search_space"])
  for category in merged_space.categories: 
    query_string = "SELECT trials.trial_id, trials.problem_id, trials.result, "
    num_inputs = len(problem["inputs"])
    num_outputs = len(problem["outputs"])
    for index,variable in enumerate(problem["inputs"]):
      query_string += "dv%s.%s AS variable_value%s%s" % (index, "value%s" % (index,), index, ", ")
    for oindex,variable in enumerate(problem["outputs"]):
      index = num_inputs + oindex
      query_string += "dv%s.%s AS variable_value%s%s" % (index, "value%s" % (index,), index, ", " if oindex is not (num_outputs-1) else " ")
    query_string += " FROM trials "
    for index,variable in enumerate(problem["inputs"]):
      higher = [x for x in slice_space(variable_descriptions[variable]["search_space"]).categories if x > category[index]]
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
    for index,variable in enumerate(problem["inputs"]):
      query_string += "WHERE variable_value NOT NULL AND " % (index)
    query_string += " TRUE "
    
    print(query_string)
#print(merged_space.categories)
#
#fetcher.execute("SELECT * FROM trials")
#trials = fetcher.fetchall()
#
#for trial in trials:
#  trial_id,problem_id,result = trial
#  fetcher.execute("SELECT * FROM trial_values WHERE trial_id = ?",(trial_id,))
#  values = fetcher.fetchall()
#  to_insert = {"labrador.problem.result" : result} 
#  for value in values:
#    _,variable_id,discrete,continuous = value
#    to_insert[variable_id] = discrete if continuous is None else continuous  
#  problem_descriptions[problem_id]["instances"].append(to_insert)  



#print(problem_descriptions)
#print(variable_descriptions)
