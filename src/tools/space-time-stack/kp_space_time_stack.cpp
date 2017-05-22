#include <cstdint>
#include <iostream>
#include <cstdlib>
#include <memory>
#include <string>
#include <set>
#include <cassert>
#include <chrono>
#include <queue>

namespace {

struct KokkosPDeviceInfo {
  std::uint32_t deviceID;
};

struct SpaceHandle {
  char name[64];                                                                        
};

#if 0
enum Space {
  SPACE_HOST,
  SPACE_CUDA
};

enum Space get_space(SpaceHandle const& handle) {
  if (handle.name[0] == 'H') return SPACE_HOST;
  if (handle.name[0] == 'C') return SPACE_CUDA;
  abort();
  return SPACE_HOST;
}
#endif

struct Now {
  typedef std::chrono::time_point<std::chrono::high_resolution_clock> Impl;
  Impl impl;
};

Now now() {
  Now t;
  t.impl = std::chrono::high_resolution_clock::now();
  return t;
}

double operator-(Now b, Now a) {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(b.impl - a.impl)
             .count() *
         1e-9;
}

struct StackNode {
  StackNode* parent;
  std::string name;
  std::set<StackNode> children;
  double total_runtime;
  std::int64_t number_of_calls;
  Now start_time;
  StackNode(StackNode* parent_in, std::string&& name):
    parent(parent_in),
    name(std::forward<std::string>(name)),
    total_runtime(0.0),
    number_of_calls(0) {
  }
  StackNode* get_child(std::string&& child_name) {
    StackNode candidate(this, std::move(child_name));
    auto it = children.find(candidate);
    if (it == children.end()) {
      auto res = children.emplace(std::move(candidate));
      it = res.first;
      assert(res.second);
    }
    return const_cast<StackNode*>(&(*(it)));
  }
  bool operator<(StackNode const& other) const {
    return this->name < other.name;
  }
  std::string get_full_name() const {
    std::string full_name;
    for (auto p = this; p; p = p->parent) {
      full_name = p->name + '/' + full_name;
    }
    return full_name;
  }
  void begin() {
    this->number_of_calls++;
    this->start_time = now();
  }
  void end(Now const& end_time) {
    auto runtime = (end_time - this->start_time);
    this->total_runtime += runtime;
  }
  StackNode invert() const {
    StackNode inv_root(nullptr, "");
    std::queue<StackNode const*> q;
    q.push(this);
    while (!q.empty()) {
      auto node = q.front(); q.pop();
      auto self_time = node->total_runtime;
      auto calls = node->number_of_calls;
      for (auto& child : node->children) {
        self_time -= child.total_runtime;
        q.push(&child);
      }
      auto inv_node = &inv_root;
      inv_node->total_runtime += self_time;
      inv_node->number_of_calls += calls;
      for (; node; node = node->parent) {
        std::string name = node->name;
        inv_node = inv_node->get_child(std::move(name));
        inv_node->total_runtime += self_time;
        inv_node->number_of_calls += calls;
      }
    }
    return inv_root;
  }
  void print_recursive(
      std::ostream& os, std::string my_indent, std::string const& child_indent, double tree_time) const {
    auto percent = int((this->total_runtime / tree_time) * 100.0);
    if (percent < 1) return; // 1% of total runtime is noise threshold
    if (!this->name.empty()) {
      os << my_indent;
      os << percent << "% / " << this->number_of_calls << " " << this->name;
      os << '\n';
    }
    if (this->children.empty()) return;
    auto by_time = [](StackNode const* a, StackNode const* b) {
      if (a->total_runtime != b->total_runtime) {
        return a->total_runtime > b->total_runtime;
      }
      return a->name < b->name;
    };
    std::set<StackNode const*, decltype(by_time)> children_by_time(by_time);
    for (auto& child : children) {
      children_by_time.insert(&child);
    }
    auto last = children_by_time.end();
    --last;
    for (auto it = children_by_time.begin(); it != children_by_time.end(); ++it) {
      std::string grandchild_indent;
      if (it == last) {
        grandchild_indent = child_indent + "    ";
      } else {
        grandchild_indent = child_indent + "|   ";
      }
      auto child = *it;
      child->print_recursive(
          os, child_indent + "|-> ", grandchild_indent, tree_time);
    }
  }
  void print(std::ostream& os) const {
    this->print_recursive(os, "", "", this->total_runtime);
  }
};

struct State {
  StackNode stack_root;
  StackNode* stack_frame;
  State():stack_root(nullptr, ""),stack_frame(&stack_root) {
    stack_frame->begin();
  }
  ~State() {
    auto end_time = now();
    if (stack_frame != &stack_root) {
      std::cerr << "Program ended before \"" << stack_frame->get_full_name()
                << "\" ended\n";
      abort();
    }
    stack_frame->end(end_time);
    std::cout << "TOP-DOWN TIME TREE:\n";
    std::cout << "================== \n";
    stack_root.print(std::cout);
    auto inv_stack_root = stack_root.invert();
    std::cout << "BOTTOM-UP TIME TREE:\n";
    std::cout << "=================== \n";
    inv_stack_root.print(std::cout);
  }
  std::uint64_t begin_kernel(std::string&& name) {
    stack_frame = stack_frame->get_child(std::move(name));
    stack_frame->begin();
    return reinterpret_cast<std::uint64_t>(stack_frame);
  }
  void end_kernel(std::uint64_t kernid) {
    auto end_time = now();
    auto expect_node = reinterpret_cast<StackNode*>(kernid);
    if (expect_node != stack_frame) {
      std::cerr << "Expected \"" << stack_frame->get_full_name()
                << "\" to end, got different kernel ID\n";
      abort();
    }
    stack_frame->end(end_time);
    stack_frame = stack_frame->parent;
  }
};

State* global_state = nullptr;

}  // end anonymous namespace

extern "C" void kokkosp_init_library(
    int loadseq, uint64_t version, uint32_t ndevinfos, KokkosPDeviceInfo* devinfos) {
  (void)loadseq;
  (void)ndevinfos;
  (void)devinfos;
  if (version != 20150628) {
    std::cerr << "kokkosp_init_library: version " << version << " != 20150628\n";
    abort();
  }
  global_state = new State();
}

extern "C" void kokkosp_begin_parallel_for(
    const char* name, std::uint32_t devid, std::uint64_t* kernid) {
  (void) devid;
  *kernid = global_state->begin_kernel(std::string(name) + " [for]");
}

extern "C" void kokkosp_begin_parallel_reduce(
    const char* name, std::uint32_t devid, std::uint64_t* kernid) {
  (void) devid;
  *kernid = global_state->begin_kernel(std::string(name) + " [reduce]");
}

extern "C" void kokkosp_begin_parallel_scan(
    const char* name, std::uint32_t devid, std::uint64_t* kernid) {
  (void) devid;
  *kernid = global_state->begin_kernel(std::string(name) + " [scan]");
}

extern "C" void kokkosp_end_parallel_for(std::uint64_t kernid) {
  global_state->end_kernel(kernid);
}

extern "C" void kokkosp_end_parallel_reduce(std::uint64_t kernid) {
  global_state->end_kernel(kernid);
}

extern "C" void kokkosp_end_parallel_scan(std::uint64_t kernid) {
  global_state->end_kernel(kernid);
}

extern "C" void kokkosp_finalize_library() {
  delete global_state;
  global_state = nullptr;
}
