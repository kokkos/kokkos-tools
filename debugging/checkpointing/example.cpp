
#include <Kokkos_Core.hpp>
#include <fstream>
#include <iostream>
#include <protocols/checkpointing.pb.h>
struct ptr_info {
  std::string who;
  void* what;
  size_t how_much;
  bool where;
  mutable void* canonical;
};

std::pair<void*,size_t> get_ptr_data(std::ifstream& input){
    size_t message_size;
    input >> message_size;

    char* raw_input = new char[message_size];    
    input.read(raw_input, message_size);
    
    ptr_info info;
    kokkos_checkpointing::View v;
    bool worked = v.ParseFromString(std::string(raw_input,message_size));
    info.who = v.name();
    info.how_much = v.size();
    char* raw_data = reinterpret_cast<char*>(malloc(info.how_much));
    memcpy(raw_data, const_cast<char*>(v.data().c_str()), info.how_much);
    return std::make_pair(raw_data, info.how_much);
}
  template<typename Hv, typename Dv>
  void show(Hv h, Dv d){
    std::cout << d.label() << "["<<h.extent(0) << "] (" << std::hex << h.data()<<") "<< std::dec << h[0]<<std::endl;
    for(int x = 0 ; x < h.extent(0); ++x){
      if(h(x)!=0){
        std::cout << "Nonzero\n";
      }
    }
  std::cout << h(h.extent(0)-2) << std::endl;
  }
int main(int argc, char* argv[]){
  Kokkos::initialize(argc, argv);
  {
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    std::string filename = "checkpoint.kokkos";
    if (argc > 1) {
      filename = argv[1];
    }
    std::ifstream input(filename);
    size_t num_alloc;
    input >> num_alloc;
    using puppies_scalar_type= float;
    using puppies_view_type= Kokkos::View<puppies_scalar_type*, Kokkos::CudaSpace>; /** you need to fill in the number of dimensions, sorry */
    auto puppies_data = get_ptr_data(input);
    puppies_view_type::HostMirror puppies_mirror( reinterpret_cast<puppies_scalar_type*>(puppies_data.first),puppies_data.second / sizeof(puppies_scalar_type)); /** you may need to adapt this for Views > 1D */
    puppies_view_type puppies("puppies",puppies_data.second / sizeof(puppies_scalar_type));
    Kokkos::deep_copy(puppies,puppies_mirror);
    Kokkos::fence();
    show(puppies_mirror,puppies);
    auto puppies_1_data = get_ptr_data(input);
    puppies_view_type::HostMirror puppies_1_mirror( reinterpret_cast<puppies_scalar_type*>(puppies_1_data.first),puppies_1_data.second / sizeof(puppies_scalar_type)); /** you may need to adapt this for Views > 1D */
    puppies_view_type puppies_1("puppies_1",puppies_1_data.second / sizeof(puppies_scalar_type));
    Kokkos::deep_copy(puppies_1,puppies_1_mirror);
    Kokkos::fence();
    show(puppies_1_mirror,puppies_1);

  }
  Kokkos::finalize();
}
