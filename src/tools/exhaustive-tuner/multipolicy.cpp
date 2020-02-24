#include <Kokkos_Core.hpp>
#include <Kokkos_Macros.hpp>
#include <cmath>
#include <array>
#include <chrono>
#include <thread>
#include <iostream>
constexpr const int num_iterations = 1000;
constexpr const float penalty      = 0.1f;
void sleep_difference(int actual, int guess) {
  std::this_thread::sleep_for(
      std::chrono::milliseconds(int(penalty * std::abs(guess - actual))));
}

size_t getBackendId(){
        static size_t id;
        static bool initialized;
        if(!initialized){
            initialized = true;    
    id = Kokkos::Tuning::getNewVariableId();
    Kokkos::Tuning::VariableInfo info;
    info.type = kokkos_value_integer;
    info.category = kokkos_value_categorical;
    info.valueQuantity = kokkos_value_set;
    Kokkos::Tuning::declareTuningVariable("kokkos.backend_choice",id,info); 
        }
        return id;
}

namespace impl {
  template<typename... Devices>
  struct executor; 
  template<>
  struct executor<> {

    template<typename Functor>
    static void execute(size_t index, const char* name, size_t lower, size_t upper, const Functor&& func){
            return; // TODO DIE HERE
    }
  };
  template<typename Device, typename... Devices>
  struct executor<Device, Devices...> {
    template<typename Functor>
    static void execute(size_t index, const char* name, size_t lower, size_t upper, Functor&& func){
      if(index==0){
              Kokkos::parallel_for(name, Kokkos::RangePolicy<Device>(lower, upper), func);
              return;
      } 
      executor<Devices...>::execute(index-1,name,lower,upper,std::forward<Functor>(func));
    } 
  };
}


template <typename... Devices>
class PolyExecutor  {
 public:
  static PolyExecutor& instance() {
    static PolyExecutor* executor;
    if (!executor) {
      executor = new PolyExecutor();
    }
    return *executor;
  }
  Kokkos::Tuning::VariableValue backend_variable_values[sizeof...(Devices)];
  template<class Functor>
  void execute(const char* name, size_t lower, size_t upper, Functor&& in){
    size_t tuningContextId = Kokkos::Tuning::getNewContextId();
    Kokkos::Tuning::SetOrRange candidates;
    candidates.set.id = backend_variable_values[0].id;
    candidates.set.size = sizeof...(Devices);
    candidates.set.values = backend_variable_values;
    Kokkos::Tuning::VariableValue picked_value = backend_variable_values[0];
    Kokkos::Tuning::requestTuningVariableValues(tuningContextId, 1, &picked_value, &candidates);
    impl::executor<Devices...>::execute(picked_value.value.int_value, name, lower, upper,  std::forward<Functor&&>(in)); 
    Kokkos::Tuning::endContext(tuningContextId);
  }
 private:
  PolyExecutor() {
     //const char** value = { Devices::name()... };
     size_t* ids = new size_t[sizeof...(Devices)];
     for(int x =0 ;x<sizeof...(Devices);++x){
       backend_variable_values[x] = Kokkos::Tuning::make_variable_value(getBackendId(), x);
     }
  }
};

        int main(int argc, char* argv[]) {
  using Executor = PolyExecutor<Kokkos::Serial, Kokkos::OpenMP>;      
  Kokkos::initialize(argc, argv);
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
  std::cout << "PRE DECLARE\n";
  Kokkos::Tuning::declareContextVariable("target_value", context_variable_id,
                                         context_info, value_range);
  std::cout << "POST DECLARE\n";
  {
    Executor exec = Executor::instance();
    for(int attempt =0;attempt<120;++attempt){
    for (int work_intensity = 0; work_intensity < num_iterations;
         work_intensity+=200) {
      size_t contextId = Kokkos::Tuning::getNewContextId();
      Kokkos::Tuning::VariableValue context_value = Kokkos::Tuning::make_variable_value(
              context_variable_id, work_intensity);
      Kokkos::Tuning::declareContextVariableValues(
          contextId, 1, 
          &context_value);
    exec.execute("dogs",0,work_intensity, [&](const int id){});
      //std::cout << "DOGS\n";
      Kokkos::Tuning::endContext(contextId);
    }
    }
  }
  Kokkos::finalize();
}
