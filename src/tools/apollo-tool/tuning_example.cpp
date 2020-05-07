#include <Kokkos_Core.hpp>
#include <Kokkos_Macros.hpp>
#include <cmath>
#include <array>
#include <mpi.h>
#include <chrono>
#include <thread>
#include <iostream>
#include <vector>
constexpr const int num_iterations = 1000;
constexpr const float penalty = 0.1f;
void sleep_difference(int actual, int guess){
  std::this_thread::sleep_for(std::chrono::milliseconds(int(penalty * std::abs(guess-actual))));
}

// This is a simple tuning example, in which we have a function that takes two values and
// sleeps for their difference. The tool is given one value, and asked to guess the other
//
// The correct answer is to guess the same value the tool was given in the context

int main(int argc, char* argv[]) {
  MPI_Init(&argc, &argv);
  Kokkos::initialize(argc, argv);
  // *Here we want to declare a context variable, something a tool might tune using
  //
  // **First we get an id for it
  size_t context_variable_id = Kokkos::Tools::getNewVariableId();

  // **now we *can* (aren't required to) declare candidate values
  // *** a lower bound
  Kokkos::Tools::VariableValue range_lower =
      Kokkos::Tools::make_variable_value(context_variable_id, int64_t(0));
  // *** an upper bound
  Kokkos::Tools::VariableValue range_upper =
      Kokkos::Tools::make_variable_value(context_variable_id, int64_t(num_iterations));
  // *** and a step size
  Kokkos::Tools::VariableValue range_step =
      Kokkos::Tools::make_variable_value(context_variable_id, int64_t(1));


  Kokkos::Tools::VariableInfo context_info;
  // ** now we want to let the tool know about the semantics of that variable
  // *** it's an integer
  context_info.type          = kokkos_value_integer;
  // *** it's 'interval', roughly meaning that subtracting two values makes sense
  context_info.category      = kokkos_value_interval;
  // *** and the candidate values are in a range, not a set 
  context_info.valueQuantity = kokkos_value_range;
  Kokkos::Tools::SetOrRange value_range;

  // ** this is just the earlier "range" construction
  // ** the last two values are bools representing whether 
  // ** the range is open (endpoints not included) or closed (endpoints included)
  value_range.range = Kokkos::Tools::ValueRange{
      context_variable_id, &range_lower, &range_upper,
      &range_step,         false,        false};
  // ** here we actually declare it to the tool
  Kokkos::Tools::declareContextVariable("target_value", context_variable_id,
                                         context_info, value_range);


  // * Now we're declaring the tuning variable
  // ** So we need an id for it
  size_t tuning_value_id = Kokkos::Tools::getNewVariableId();

  // ** its semantics exactly mirror the tuning variable, it's an
  // ** integer interval value
  Kokkos::Tools::VariableInfo tuning_variable_info;
  tuning_variable_info.category      = kokkos_value_interval;
  tuning_variable_info.type          = kokkos_value_integer;

  // ** Here I'm setting the candidate values to be from a set for two reasons
  // ** 1) It shows the other side of this interface
  // ** 2) ... the prototype tool doesn't support guessing from ranges yet
  tuning_variable_info.valueQuantity = kokkos_value_set;
  Kokkos::Tools::declareTuningVariable("tuned_choice", tuning_value_id,
                                        tuning_variable_info);

  Kokkos::Tools::SetOrRange candidate_values;
  std::vector<Kokkos::Tools::VariableValue> candidate_value_array {
      Kokkos::Tools::make_variable_value(tuning_value_id, int64_t(0)),
      Kokkos::Tools::make_variable_value(tuning_value_id, int64_t(100)),
      Kokkos::Tools::make_variable_value(tuning_value_id, int64_t(200)),
      Kokkos::Tools::make_variable_value(tuning_value_id, int64_t(300)),
      Kokkos::Tools::make_variable_value(tuning_value_id, int64_t(400)),
      Kokkos::Tools::make_variable_value(tuning_value_id, int64_t(500)),
      Kokkos::Tools::make_variable_value(tuning_value_id, int64_t(600)),
      Kokkos::Tools::make_variable_value(tuning_value_id, int64_t(700)),
      Kokkos::Tools::make_variable_value(tuning_value_id, int64_t(800)),
      Kokkos::Tools::make_variable_value(tuning_value_id, int64_t(900)),
  };
  candidate_values.set = Kokkos::Tools::ValueSet{tuning_value_id, 10,
                                                  candidate_value_array.data()};

  {
    // * declaring a VariableValue which can hold the results
    // *   of the tuning tool
    Kokkos::Tools::VariableValue tuned_choice;
    tuned_choice.id = tuning_value_id;
    // ** Note that the default value must not crash the program
    tuned_choice.value.int_value = 0;

    // * Looping multiple times so the tool can converge
    for(int attempt =0;attempt<3;++attempt){
    for (int work_intensity = 0; work_intensity < num_iterations;
         work_intensity+=200) {
      std::cout << "Attempting a run with value "<<work_intensity<<std::endl;
      size_t contextId = Kokkos::Tools::getNewContextId();

      // *Here we tell the tool the value of the context variable
      Kokkos::Tools::VariableValue context_value = Kokkos::Tools::make_variable_value(
              context_variable_id, int64_t(work_intensity));
      Kokkos::Tools::declareContextVariableValues(
          contextId, 1, 
          &context_value);
            
      // *Now we ask the tool to give us the value it thinks will perform best
      Kokkos::Tools::requestTuningVariableValues(contextId, 1, &tuned_choice, &candidate_values); 

      // *Call the function with those two values
      sleep_difference(work_intensity, tuned_choice.value.int_value);

      // *This call tells the tool the context is over, so it can
      // *take measurements
      Kokkos::Tools::endContext(contextId);
    }
    }
  }
  Kokkos::finalize();
  MPI_Finalize();
}
