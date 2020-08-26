#include <fstream>
#include <iostream>
#include <protocols/checkpointing.pb.h>
#include <string>
#include <set>
struct ptr_info {
  std::string who;
  void* what;
  size_t how_much;
  bool where;
  mutable void* canonical;  // look, I'm in a hurry, I'm sorry
};
std::string program_prefix = R"PREFIX(
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
std::map<std::string, int> num_encounters;
std::map<std::string, std::string> care_about = {
    {"C row ptrs", "x_rowmap"},
    {"non_const_lnow_row", "b_rowmap"},
    {"C row counts upper bound", "c_rowmap_upperbound"},
    {"C entries uncompressed", "c_entries_uncompressed"},
    {"A and B permuted entry indices", "ab_perm"},
    {"A entry positions", "a_pos"},
    {"B entry positions", "b_pos"},
    {"puppies", "puppies"}};

std::map<std::string, std::pair<std::string,std::string>> type_info = {
    //{"C row ptrs", "x_rowmap"},
    //{"non_const_lnow_row", "b_rowmap"},
    //{"C row counts upper bound", "c_rowmap_upperbound"},
    //{"C entries uncompressed", "c_entries_uncompressed"},
    //{"A and B permuted entry indices", {"long long","*"}},
    //{"A entry positions", "a_pos"},
    //{"B entry positions", "b_pos"},
   { "puppies", {"float", "*"}}       
};

std::set<std::string> encountered_names;
bool name_matches(std::string name) {
  return (care_about.find(name) != care_about.end());
}
std::string get_kokkos_view_string(std::string oname, bool new_name = true) {
  std::string build;
  if (name_matches(oname)) {
    if (num_encounters.find(oname) == num_encounters.end()) {
      num_encounters[oname] = 0;
    }
    auto& value                  = num_encounters[oname];
    auto name                    = care_about[oname];
    std::string value_name = name;
    if(value!=0){
      value_name += "_" + std::to_string(value);
    }
    ++value;
    std::string raw_scalar_type = "USER_FILL_THIS_IN";
    std::string raw_pointer_addend = "*";
    if(type_info.find(oname)!=type_info.end()){
      auto pr = type_info[oname];
      raw_scalar_type = pr.first;
      raw_pointer_addend = pr.second;
    }
    std::string scalar_type_name = name + "_scalar_type";
    std::string view_type_name   = name + "_view_type";
    std::string view_type =
        "Kokkos::View<" + scalar_type_name + raw_pointer_addend + ", Kokkos::CudaSpace>";
    std::string mirror_view_type_name = view_type_name + "::HostMirror";
    std::string mirror_name           = value_name + "_mirror";
    std::string data_name             = value_name + "_data";
    // build+= ((new_name) ? ("YES\n") : ("NO\n"));
    if (new_name) {
      build += "    using " + scalar_type_name + "= "+raw_scalar_type+";\n";
      build += "    using " + view_type_name + "= " + view_type +
               "; /** you need to fill in the number of dimensions, sorry */\n";
    }
    std::string declarator = "auto ";
    build += std::string("    ") + declarator + data_name +
             " = get_ptr_data(input);\n";
    build += "    " + mirror_view_type_name + " " + mirror_name +
             "( reinterpret_cast<" + scalar_type_name + "*>(" + data_name +
             ".first)," + data_name + ".second / sizeof(" + scalar_type_name +
             ")); /** you may need to adapt this for Views > 1D */\n";
    build += "    " + view_type_name + " " + value_name + "(\"" + value_name + "\"," +
             data_name + ".second / sizeof(" + scalar_type_name + "));\n";
    build += "    Kokkos::deep_copy(" + value_name + "," + mirror_name + ");\n";
    build += "    Kokkos::fence();\n";
    build += "    show("+mirror_name+","+value_name+");\n";
  } else {
    build += "    get_ptr_data(input);\n";
  }
  return build;
}
int main(int argc, char* argv[]) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  std::string filename = "checkpoint.kokkos";
  if (argc > 1) {
    filename = argv[1];
  }
  std::ifstream input(filename);
  int num_allocations;
  input >> num_allocations;
  std::string program = program_prefix;
  for (int x = 0; x < num_allocations; ++x) {
    size_t message_size;
    input >> message_size;

    char* raw_input = new char[message_size];
    input.read(raw_input, message_size);

    ptr_info info;
    kokkos_checkpointing::View v;
    bool worked      = v.ParseFromString(raw_input);
    info.who         = v.name();
    info.how_much    = v.size();
    char* raw_data   = const_cast<char*>(v.data().c_str());
    int num_entries  = info.how_much / sizeof(float);
    bool found       = false;
    auto encountered = encountered_names.find(info.who);
    if (encountered == encountered_names.end()) {
      found = true;
      encountered_names.insert(info.who);
    }
    std::string reader = get_kokkos_view_string(info.who, found);
    program += reader;
  }
  program += program_suffix;
  std::cout << program;
}
