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

int main(int argc, char* argv[]) {
  Kokkos::initialize(argc, argv);

  size_t context_variable_id = Kokkos::Tuning::getNewVariableId();
  Kokkos::Tuning::VariableValue range_lower =
      Kokkos::Tuning::make_variable_value(context_variable_id, 0);
  Kokkos::Tuning::VariableValue range_upper =
      Kokkos::Tuning::make_variable_value(context_variable_id, num_iterations);
  Kokkos::Tuning::VariableValue range_step =
      Kokkos::Tuning::make_variable_value(context_variable_id, 1);
  Kokkos::Tuning::VariableInfo context_info;
  context_info.category      = kokkos_value_interval;
  context_info.type          = kokkos_value_integer;
  context_info.valueQuantity = kokkos_value_range;
  Kokkos::Tuning::SetOrRange value_range;
  value_range.range = Kokkos::Tuning::ValueRange{
      context_variable_id, &range_lower, &range_upper,
      &range_step,         false,        false};
  Kokkos::Tuning::declareContextVariable("target_value", context_variable_id,
                                         context_info, value_range);

  size_t tuning_value_id = Kokkos::Tuning::getNewVariableId();
  Kokkos::Tuning::VariableInfo tuning_variable_info;
  tuning_variable_info.category      = kokkos_value_interval;
  tuning_variable_info.type          = kokkos_value_integer;
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
    Kokkos::Tuning::VariableValue tuned_choice;
    tuned_choice.id = tuning_value_id;
    tuned_choice.value.int_value = 0;
    for(int attempt =0;attempt<120;++attempt){
    for (int work_intensity = 0; work_intensity < num_iterations;
         work_intensity+=200) {
      std::cout << "Attempting a run with value "<<work_intensity<<std::endl;
      size_t contextId = Kokkos::Tuning::getNewContextId();
      Kokkos::Tuning::declareContextVariableValues(
          contextId, 1, &context_variable_id,
          new Kokkos::Tuning::VariableValue(Kokkos::Tuning::make_variable_value(
              context_variable_id, work_intensity)));
      Kokkos::Tuning::requestTuningVariableValues(contextId, 1, &tuning_value_id, &tuned_choice, &candidate_values); 
      sleep_difference(work_intensity, tuned_choice.value.int_value);
      Kokkos::Tuning::endContext(contextId);
    }
    }
  }
  Kokkos::finalize();
}
