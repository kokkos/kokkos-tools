#include<fstream>
#include <iostream>
#include <protocols/checkpointing.pb.h>
#include <string>
#include <set>
struct ptr_info {
  std::string who;
  void* what;
  size_t how_much;
  bool where;
  mutable void* canonical; // look, I'm in a hurry, I'm sorry
};
std::string program_prefix = R"PREFIX(
#include <Kokkos_Core.hpp>
#include <fstream>
#include <iostream>
struct ptr_info {
  std::string who;
  void* what;
  size_t how_much;
  bool where;
  mutable void* canonical;
};

std::pair<void*,size_t> get_ptr_data(std::ifstream& input){
    ptr_info info;
    input >> info.who >> info.how_much; 
    char* raw_data = (char*)malloc(info.how_much);
    input.read(raw_data, info.how_much);
    info.what = raw_data;
    return std::make_pair(info.what, info.how_much);
}
std::pair<void*,size_t> pass_data;

int main(int argc, char* argv[]){
  Kokkos::initialize(argc, argv);
  {
    std::ifstream input("checkpoint.kokkos");
)PREFIX";
std::string program_suffix = R"SUFFIX(
  }
  Kokkos::finalize();
}
)SUFFIX";
/**
 *      UnsortedEntriesUpperBound<size_type, ordinal_type, alno_row_view_t_,
                                blno_row_view_t_, clno_row_view_t_>
                                */
std::set<std::string> care_about = {
"C row ptrs",
"non_const_lnow_row",
"C row ptrs",
"C row counts upper bound",
"C entries uncompressed",
"A and B permuted entry indices",
"A entry positions",
"B entry positions"
};
bool name_matches(std::string name){
  return (care_about.find(name) != care_about.end());
}
std::string get_kokkos_view_string(std::string name, bool new_name = true){
  std::string build;
  if(name_matches(name)){
    std::string scalar_type_name = name+"_scalar_type";
    std::string view_type_name  = name+"_view_type";
    std::string view_type = "Kokkos::View<"+scalar_type_name+", Kokkos::CudaSpace>";
    std::string mirror_view_type_name = view_type_name+"::HostMirror";
    std::string mirror_name = name + "_mirror";
    std::string data_name = name+"_data";
    if(new_name){ 
      build += "    using "+scalar_type_name+ "= USER_FILL_THIS_IN;\n";  
      build += "    using "+view_type_name+ "= "+ view_type+"; /** you need to fill in the number of dimensions, sorry */\n";
    }
    std::string declarator = new_name ? std::string("auto ") : std::string("");
    build += std::string("    ")+declarator+data_name+" = get_ptr_data(input);\n";
    build += "    "+declarator+mirror_view_type_name + " "+mirror_name +"( reinterpret_cast<"+scalar_type_name+"*>("+data_name +".first),"+data_name+".second / sizeof(" + scalar_type_name+")); /** you may need to adapt this for Views > 1D */\n";
    build += "    "+declarator+view_type_name+" "+name+"(\""+name+"\","+data_name+".second / sizeof(" + scalar_type_name+"));\n";
    build += "    Kokkos::deep_copy("+name+","+mirror_name+");\n";
  }
  else{
    build+="    get_ptr_data(input);\n";
  }
  return build; 
}
int main(int argc, char* argv[]){
  std::string filename = "checkpoint.kokkos";
  if(argc > 1){
    filename = argv[1];
  }
  std::ifstream input(filename);
  int num_allocations;
  input >> num_allocations;
  std::cout << "NUMALLOC: "<<num_allocations;
  std::string program = program_prefix;

  for(int x =0; x<num_allocations;++x){
    ptr_info info;
    kokkos_checkpointing::View v;
    v.ParseFromIstream(&input);
    info.who = v.name();
    info.how_much = v.size();
    char* raw_data = const_cast<char*>(v.data().c_str());
    //char* raw_data = (char*)malloc(info.how_much);
    int num_entries = info.how_much/sizeof(float);
    //input.read(raw_data, info.how_much);
    std::cout << "NAME: "<< info.who << std::endl;
    std::string reader = get_kokkos_view_string(info.who);    
    program+=reader;
  }
  program += program_suffix;
  std::cout << program;
}
