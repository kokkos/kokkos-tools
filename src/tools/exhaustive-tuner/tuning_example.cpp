#include <Kokkos_Core.hpp>
#include <Kokkos_Macros.hpp>
#include <cmath>
#include <array>
#include <chrono>
#include <thread>
#include <iostream>

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

  Kokkos::initialize(argc, argv);
  // *Here we want to declare a context variable, something a tool might tune using
  //
  // **First we get an id for it
  size_t context_variable_id = Kokkos::Tuning::getNewVariableId();

  // **now we *can* (aren't required to) declare candidate values
  // *** a lower bound
  Kokkos::Tuning::VariableValue range_lower =
      Kokkos::Tuning::make_variable_value(context_variable_id, 0);
  // *** an upper bound
  Kokkos::Tuning::VariableValue range_upper =
      Kokkos::Tuning::make_variable_value(context_variable_id, num_iterations);
  // *** and a step size
  Kokkos::Tuning::VariableValue range_step =
      Kokkos::Tuning::make_variable_value(context_variable_id, 1);


  Kokkos::Tuning::VariableInfo context_info;
  // ** now we want to let the tool know about the semantics of that variable
  // *** it's an integer
  context_info.type          = kokkos_value_integer;
  // *** it's 'interval', roughly meaning that subtracting two values makes sense
  context_info.category      = kokkos_value_interval;
  // *** and the candidate values are in a range, not a set 
  context_info.valueQuantity = kokkos_value_range;
  Kokkos::Tuning::SetOrRange value_range;

  // ** this is just the earlier "range" construction
  // ** the last two values are bools representing whether 
  // ** the range is open (endpoints not included) or closed (endpoints included)
  value_range.range = Kokkos::Tuning::ValueRange{
      context_variable_id, &range_lower, &range_upper,
      &range_step,         false,        false};
  // ** here we actually declare it to the tool
  Kokkos::Tuning::declareContextVariable("target_value", context_variable_id,
                                         context_info, value_range);


  // * Now we're declaring the tuning variable
  // ** So we need an id for it
  size_t tuning_value_id = Kokkos::Tuning::getNewVariableId();

  // ** its semantics exactly mirror the tuning variable, it's an
  // ** integer interval value
  Kokkos::Tuning::VariableInfo tuning_variable_info;
  tuning_variable_info.category      = kokkos_value_interval;
  tuning_variable_info.type          = kokkos_value_integer;

  // ** Here I'm setting the candidate values to be from a set for two reasons
  // ** 1) It shows the other side of this interface
  // ** 2) ... the prototype tool doesn't support guessing from ranges yet
  tuning_variable_info.valueQuantity = kokkos_value_set;
  Kokkos::Tuning::declareTuningVariable("tuned_choice", tuning_value_id,
                                        tuning_variable_info);

  Kokkos::Tuning::SetOrRange candidate_values;
  std::vector<Kokkos::Tuning::VariableValue> candidate_value_array = {
      Kokkos::Tuning::make_variable_value(tuning_value_id, 0),
      Kokkos::Tuning::make_variable_value(tuning_value_id, 100),
      Kokkos::Tuning::make_variable_value(tuning_value_id, 200),
      Kokkos::Tuning::make_variable_value(tuning_value_id, 300),
      Kokkos::Tuning::make_variable_value(tuning_value_id, 400),
      Kokkos::Tuning::make_variable_value(tuning_value_id, 500),
      Kokkos::Tuning::make_variable_value(tuning_value_id, 600),
      Kokkos::Tuning::make_variable_value(tuning_value_id, 700),
      Kokkos::Tuning::make_variable_value(tuning_value_id, 800),
      Kokkos::Tuning::make_variable_value(tuning_value_id, 900),
  };
  candidate_values.set = Kokkos::Tuning::ValueSet{tuning_value_id, 10,
                                                  candidate_value_array.data()};

  {
    // * declaring a VariableValue which can hold the results
    // *   of the tuning tool
    Kokkos::Tuning::VariableValue tuned_choice;
    tuned_choice.id = tuning_value_id;
    // ** Note that the default value must not crash the program
    tuned_choice.value.int_value = 0;

    // * Looping multiple times so the tool can converge
    for(int attempt =0;attempt<120;++attempt){
    for (int work_intensity = 0; work_intensity < num_iterations;
         work_intensity+=200) {
      std::cout << "Attempting a run with value "<<work_intensity<<std::endl;
      size_t contextId = Kokkos::Tuning::getNewContextId();

      // *Here we tell the tool the value of the context variable
      Kokkos::Tuning::VariableValue context_value = Kokkos::Tuning::make_variable_value(
              context_variable_id, work_intensity);
      Kokkos::Tuning::declareContextVariableValues(
          contextId, 1, &context_variable_id,
          &context_value);
            
      // *Now we ask the tool to give us the value it thinks will perform best
      Kokkos::Tuning::requestTuningVariableValues(contextId, 1, &tuning_value_id, &tuned_choice, &candidate_values); 

      // *Call the function with those two values
      sleep_difference(work_intensity, tuned_choice.value.int_value);

      // *This call tells the tool the context is over, so it can
      // *take measurements
      Kokkos::Tuning::endContext(contextId);
    }
    }
  }
  Kokkos::finalize();
}
