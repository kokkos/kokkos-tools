//@HEADER
// ************************************************************************
//
//                        Kokkos v. 3.0
//       Copyright (2020) National Technology & Engineering
//               Solutions of Sandia, LLC (NTESS).
//
// Under the terms of Contract DE-NA0003525 with NTESS,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY NTESS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL NTESS OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact David Poliakoff (dzpolia@sandia.gov)
//
// ************************************************************************
//@HEADER
#include <cstdint>
#include <cinttypes>
#include <iostream>
#include <ios>
#include <iomanip>
#include <fstream>
#include <cstdlib>
#include <memory>
#include <string>
#include <set>
#include <cassert>
#include <queue>
#include <regex>
#include <sstream>
#include <sys/resource.h>
#include <algorithm>
#include <cstring>

#include "kp_core.hpp"

#if USE_MPI
#include <mpi.h>
#endif

#include <chrono>

namespace KokkosTools {
namespace SpaceTimeStack {

enum Space {
  SPACE_HOST,
  SPACE_CUDA,
  SPACE_HIP,
  SPACE_SYCL,
  SPACE_OMPT
};

enum { NSPACES = 5 };

Space get_space(SpaceHandle const& handle) {
  // check that name starts with "Cuda"
  if (strncmp(handle.name, "Cuda", 4) == 0)
    return SPACE_CUDA;
  // check that name starts with "SYCL"
  if (strncmp(handle.name, "SYCL", 4) == 0)
    return SPACE_SYCL;
  // check that name starts with "OpenMPTarget"
  if (strncmp(handle.name, "OpenMPTarget", 12) == 0)
    return SPACE_OMPT;
  // check that name starts with "HIP"
  if (strncmp(handle.name, "HIP", 3) == 0)
    return SPACE_HIP;
  if (strcmp(handle.name, "Host") == 0)
    return SPACE_HOST;

  abort();
  return SPACE_HOST;
}

const char* get_space_name(int space) {
  switch (space) {
    case SPACE_HOST: return "HOST";
    case SPACE_CUDA: return "CUDA";
    case SPACE_SYCL: return "SYCL";
    case SPACE_OMPT: return "OpenMPTarget";
    case SPACE_HIP: return "HIP";
  }
  abort();
  return nullptr;
}

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

enum StackKind {
  STACK_FOR,
  STACK_REDUCE,
  STACK_SCAN,
  STACK_REGION,
  STACK_COPY
};

void print_process_hwm() {
  struct rusage sys_resources;
  getrusage(RUSAGE_SELF, &sys_resources);
  long hwm = sys_resources.ru_maxrss;
  long hwm_max = hwm;

#if USE_MPI
  int rank, world_size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  // Max
  MPI_Reduce(&hwm, &hwm_max, 1, MPI_LONG, MPI_MAX, 0, MPI_COMM_WORLD);

  // Min
  long hwm_min;
  MPI_Reduce(&hwm, &hwm_min, 1, MPI_LONG, MPI_MIN, 0, MPI_COMM_WORLD);

  // Average
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);
  long hwm_ave;
  MPI_Reduce(&hwm, &hwm_ave, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
  hwm_ave /= world_size;

  if (rank == 0)
#endif
  {
    printf("Host process high water mark memory consumption: %ld kB\n",
      hwm_max);
#if USE_MPI
    printf("  Max: %ld, Min: %ld, Ave: %ld kB\n",
      hwm_max,hwm_min,hwm_ave);
#endif
    printf("\n");
  }
}

struct StackNode {
  StackNode* parent;
  std::string name;
  StackKind kind;
  std::set<StackNode> children;
  double total_runtime;
  double total_kokkos_runtime;
  double max_runtime;
  double avg_runtime;
  std::int64_t number_of_calls;
  std::int64_t total_number_of_kernel_calls;// Counts all kernel calls (but not region calls) this node and below this node in the tree
  Now start_time;
  StackNode(StackNode* parent_in, std::string&& name_in, StackKind kind_in):
    parent(parent_in),
    name(std::move(name_in)),
    kind(kind_in),
    total_runtime(0.),
    total_kokkos_runtime(0.),
    number_of_calls(0),
    total_number_of_kernel_calls(0) {
  }
  StackNode* get_child(std::string&& child_name, StackKind child_kind) {
    StackNode candidate(this, std::move(child_name), child_kind);
    auto it = children.find(candidate);
    if (it == children.end()) {
      auto res = children.emplace(std::move(candidate));
      it = res.first;
      assert(res.second);
    }
    return const_cast<StackNode*>(&(*(it)));
  }
  bool operator<(StackNode const& other) const {
    if (kind != other.kind) {
      return int(kind) < int(other.kind);
    }
    return name < other.name;
  }
  std::string get_full_name() const {
    std::string full_name = this->name;
    for (auto p = this->parent; p; p = p->parent) {
      if (p->name.empty() && !p->parent) continue;
      full_name = p->name + '/' + full_name;
    }
    return full_name;
  }
  void begin() {
    number_of_calls++;

    // Regions are not kernels, so we don't tally those
    if(kind==STACK_FOR || kind==STACK_REDUCE || kind==STACK_SCAN || kind==STACK_COPY)
      total_number_of_kernel_calls++;
    start_time = now();
  }
  void end(Now const& end_time) {
    auto runtime = (end_time - start_time);
    total_runtime += runtime;
  }
  void adopt() {
    if (this->kind != STACK_REGION) {
      this->total_kokkos_runtime += this->total_runtime;
    }
    for (auto& child : this->children) {
      const_cast<StackNode&>(child).adopt();
      this->total_kokkos_runtime += child.total_kokkos_runtime;
      this->total_number_of_kernel_calls +=  child.total_number_of_kernel_calls;
    }
    assert(this->total_kokkos_runtime >= 0.);
  }
  StackNode invert() const {
    StackNode inv_root(nullptr, "", STACK_REGION);
    std::queue<StackNode const*> q;
    q.push(this);
    while (!q.empty()) {
      auto node = q.front(); q.pop();
      auto self_time = node->total_runtime;
      auto self_kokkos_time = node->total_kokkos_runtime;
      auto calls = node->number_of_calls;
      for (auto& child : node->children) {
        self_time -= child.total_runtime;
        self_kokkos_time -= child.total_kokkos_runtime;
        q.push(&child);
      }
      self_time = std::max(self_time, 0.); // floating-point may give negative epsilon instead of zero
      self_kokkos_time = std::max(self_kokkos_time, 0.); // floating-point may give negative epsilon instead of zero
      auto inv_node = &inv_root;
      inv_node->total_runtime += self_time;
      inv_node->number_of_calls += calls;
      inv_node->total_kokkos_runtime += self_kokkos_time;
      for (; node; node = node->parent) {
        std::string name = node->name;
        inv_node = inv_node->get_child(std::move(name), node->kind);
        inv_node->total_runtime += self_time;
        inv_node->number_of_calls += calls;
        inv_node->total_kokkos_runtime += self_kokkos_time;
      }
    }
    return inv_root;
  }
  void print_recursive_json(
      std::ostream& os, StackNode const* parent, double tree_time) const {
    static bool add_comma = false;
    auto percent = (total_runtime / tree_time) * 100.0;
    if (percent < 0.1) return;
    if (!name.empty()) {
      if (add_comma) os << ",\n";
      add_comma = true;
      os << "{\n";
      auto imbalance = (max_runtime / avg_runtime - 1.0) * 100.0;
      os << "\"average-time\" : ";
      os << std::scientific << std::setprecision(2);
      os << avg_runtime << ",\n";
      os << std::fixed << std::setprecision(1);
      auto percent_kokkos = (total_kokkos_runtime / total_runtime) * 100.0;

      os << "\"percent\" : " << percent << ",\n";
      os << "\"percent-kokkos\" : " << percent_kokkos << ",\n";
      os << "\"imbalance\" : " << imbalance << ",\n";

      // Sum over kids if we're a region
      if (kind==STACK_REGION) {
        double child_runtime = 0.0;
        for (auto& child : children) {
          child_runtime += child.total_runtime;
        }
        auto remainder = (1.0 - child_runtime / total_runtime) * 100.0;
        double kps = total_number_of_kernel_calls / avg_runtime;
        os << "\"remainder\" : " << remainder << ",\n";
        os <<  std::scientific << std::setprecision(2);
        os << "\"kernels-per-second\" : " << kps << ",\n";
      }
      else
      {
        os << "\"remainder\" : \"N/A\",\n";
        os << "\"kernels-per-second\" : \"N/A\",\n";
      }
      os << "\"number-of-calls\" : " << number_of_calls << ",\n";
      auto name_escape_double_quote_twices = std::regex_replace(name, std::regex("\""), "\\\"");
      os << "\"name\" : \"" << name_escape_double_quote_twices << "\",\n";
      os << "\"parent-id\" : \"" << parent << "\",\n";
      os << "\"id\" : \"" << this << "\",\n";

      os << "\"kernel-type\" : ";
      switch (kind) {
        case STACK_FOR: os << "\"for\""; break;
        case STACK_REDUCE: os << "\"reduce\""; break;
        case STACK_SCAN: os << "\"scan\""; break;
        case STACK_REGION: os << "\"region\""; break;
        case STACK_COPY: os << "\"copy\""; break;
      };

      os << "\n}";
    }
    if (children.empty()) return;
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
      auto child = *it;
      child->print_recursive_json(
          os, this, tree_time);
    }
  }
  void print_json(std::ostream& os) const {
    std::ios saved_state(nullptr);
    saved_state.copyfmt(os);
    os << "{\n";
    os << "\"space-time-stack-data\" : [\n";
    print_recursive_json(os, nullptr, total_runtime);
    os << '\n';
    os << "]\n}\n";
    os.copyfmt(saved_state);
  }
  void print_recursive(
      std::ostream& os, std::string my_indent, std::string const& child_indent, double tree_time) const {
    auto percent = (total_runtime / tree_time) * 100.0;
    if (percent < 0.1) return;
    if (!name.empty()) {
      os << my_indent;
      auto imbalance = (max_runtime / avg_runtime - 1.0) * 100.0;
      os << std::scientific << std::setprecision(2);
      os << avg_runtime << " sec ";
      os << std::fixed << std::setprecision(1);
      auto percent_kokkos = (total_kokkos_runtime / total_runtime) * 100.0;

      // Sum over kids if we're a region
      if (kind==STACK_REGION) {
        double child_runtime = 0.0;
        for (auto& child : children) {
          child_runtime += child.total_runtime;
        }
        auto remainder = (1.0 - child_runtime / total_runtime) * 100.0;
        double kps = total_number_of_kernel_calls / avg_runtime;
        os << percent << "% " << percent_kokkos << "% " << imbalance << "% " << remainder << "% " << std::scientific << std::setprecision(2) << kps << " " << number_of_calls << " " << name;
      }
      else
        os << percent << "% " << percent_kokkos << "% " << imbalance << "% " << "------ " << number_of_calls << " " << name;

      switch (kind) {
        case STACK_FOR: os << " [for]"; break;
        case STACK_REDUCE: os << " [reduce]"; break;
        case STACK_SCAN: os << " [scan]"; break;
        case STACK_REGION: os << " [region]"; break;
        case STACK_COPY: os << " [copy]"; break;
      };


      os << '\n';
    }
    if (children.empty()) return;
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
    std::ios saved_state(nullptr);
    saved_state.copyfmt(os);
    print_recursive(os, "", "", total_runtime);
    os << '\n';
    os.copyfmt(saved_state);
  }
  void reduce_over_mpi() {
#if USE_MPI
    int rank, comm_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    std::queue<StackNode*> q;
    std::set<std::pair<std::string, StackKind>> children_to_process;
    q.push(this);
    while (!q.empty()) {
      auto node = q.front(); q.pop();
      node->max_runtime = node->total_runtime;
      node->avg_runtime = node->total_runtime;
      MPI_Allreduce(MPI_IN_PLACE, &(node->total_runtime),
          1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
      MPI_Allreduce(MPI_IN_PLACE, &(node->max_runtime),
          1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
      MPI_Allreduce(MPI_IN_PLACE, &(node->avg_runtime),
          1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
      node->avg_runtime /= comm_size;
      MPI_Allreduce(MPI_IN_PLACE, &(node->total_kokkos_runtime),
          1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
      /* Not all children necessarily exist on every rank. To handle this we will:
         1) Build a set of the child node names on each rank.
         2) Start with rank 0, broadcast all of it's child names and add them to the
            queue, removing them from the set of names to be processed.
            If a child doesn't exist on a rank we add an empty node for it.
         3) Do a check for the lowest rank that has any remaining unprocessed children
            and repeat step 2 broadcasting from that rank until we process all children
            from all ranks.
       */
      children_to_process.clear();
      for (auto& child : node->children) {
        children_to_process.emplace(child.name, child.kind);
      }

      int bcast_rank = 0;
      do
      {
        int nchildren_to_process = int(children_to_process.size());
        MPI_Bcast(&nchildren_to_process, 1, MPI_INT, bcast_rank, MPI_COMM_WORLD);
        if (rank == bcast_rank) {
          for (auto& child_info : children_to_process) {
            std::string child_name = child_info.first;
            int kind = child_info.second;
            int name_len = child_name.length();
            MPI_Bcast(&name_len, 1, MPI_INT, bcast_rank, MPI_COMM_WORLD);
            MPI_Bcast(&child_name[0], name_len, MPI_CHAR, bcast_rank, MPI_COMM_WORLD);
            MPI_Bcast(&kind, 1, MPI_INT, bcast_rank, MPI_COMM_WORLD);
            auto * child = node->get_child(std::move(child_name), StackKind(kind));
            q.push(child);
          }
          children_to_process.clear();
        } else {
          for (int i = 0; i < nchildren_to_process; ++i) {
            int name_len;
            MPI_Bcast(&name_len, 1, MPI_INT, bcast_rank, MPI_COMM_WORLD);
            std::string name(size_t(name_len), '?');
            MPI_Bcast(&name[0], name_len, MPI_CHAR, bcast_rank, MPI_COMM_WORLD);
            int kind;
            MPI_Bcast(&kind, 1, MPI_INT, bcast_rank, MPI_COMM_WORLD);
            auto child = node->get_child(std::move(name), StackKind(kind));
            q.push(child);
            children_to_process.erase({child->name, child->kind});
          }
        }
        int local_next_bcast_rank = children_to_process.empty() ? comm_size : rank;
        MPI_Allreduce(&local_next_bcast_rank, &bcast_rank, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
      } while(bcast_rank < comm_size);
    }
#else
    std::queue<StackNode*> q;
    q.push(this);
    while (!q.empty()) {
      auto node = q.front(); q.pop();
      node->max_runtime = node->total_runtime;
      node->avg_runtime = node->total_runtime;
      for (auto& child : node->children) {
        q.push(const_cast<StackNode*>(&child));
      }
    }
#endif
  }
};

struct Allocation {
  std::string name;
  const void* ptr;
  std::uint64_t size;
  StackNode* frame;
  Allocation(std::string&& name_in, const void* ptr_in, std::uint64_t size_in,
      StackNode* frame_in):
    name(std::move(name_in)),ptr(ptr_in),size(size_in),frame(frame_in) {
  }
  bool operator<(Allocation const& other) const {
    if (size != other.size) return size > other.size;
    return ptr < other.ptr;
  }
};

struct Allocations {
  std::uint64_t total_size;
  std::set<Allocation> alloc_set;
  Allocations():total_size(0) {}
  void allocate(std::string&& name, const void* ptr, std::uint64_t size,
      StackNode* frame) {
    auto res = alloc_set.emplace(
        Allocation(std::move(name), ptr, size, frame));
    assert(res.second);
    total_size += size;
  }
  void deallocate(std::string&& name, const void* ptr, std::uint64_t size,
      StackNode* frame) {
    auto key = Allocation(std::move(name), ptr, size, frame);
    auto it = alloc_set.find(key);
    if (it == alloc_set.end()) {
      std::stringstream ss;
      ss << "WARNING! allocation(\"" << key.name << "\", " << key.ptr
         << ", " << key.size << "), deallocated at \"" << frame->get_full_name() << "\", "
         << " was not in the currently allocated set!\n";
      auto s = ss.str();
      std::cerr << s;
    } else {
      total_size -= it->size;
      alloc_set.erase(it);
    }
  }
  void print(std::ostream& os) {
    std::string s;
#if USE_MPI
    auto max_total_size = total_size;
    MPI_Allreduce(MPI_IN_PLACE, &max_total_size, 1,
        MPI_UINT64_T, MPI_MAX, MPI_COMM_WORLD);
    /* this bit of logic is here to break ties in case two
     * or more MPI ranks allocated the same (maximum) amount of
     * memory. the one with the lowest MPI rank will print
     * its snapshot */
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    auto min_max_rank = (max_total_size == total_size) ? rank : size;
    MPI_Allreduce(MPI_IN_PLACE, &min_max_rank, 1,
        MPI_INT, MPI_MIN, MPI_COMM_WORLD);
    assert(min_max_rank < size);
    if (rank == min_max_rank)
#endif
    {
      std::stringstream ss;
      ss << std::fixed << std::setprecision(1);
      ss << "MAX MEMORY ALLOCATED: " << double(total_size)/1024.0 << " kB" << '\n'; // convert bytes to kB
#if USE_MPI
      ss << "MPI RANK WITH MAX MEMORY: " << rank << '\n';
#endif
      ss << "ALLOCATIONS AT TIME OF HIGH WATER MARK:\n";
      std::ios saved_state(nullptr);
      for (auto& allocation : alloc_set) {
        auto percent = double(allocation.size) / double(total_size) * 100.0;
        if (percent < 0.1) continue;
        std::string full_name = allocation.frame->get_full_name();
        if (full_name.empty()) full_name = allocation.name;
        else full_name = full_name + "/" + allocation.name;
        ss << "  " << percent << "% " << full_name << '\n';
      }
      ss << '\n';
      s = ss.str();
    }
#if USE_MPI
    // a little MPI dance to send the string from min_max_rank to rank 0
    MPI_Request request;
    int string_size;
    if (rank == 0) {
      MPI_Irecv(&string_size, 1, MPI_INT, min_max_rank, 42, MPI_COMM_WORLD, &request);
    }
    if (rank == min_max_rank) {
      string_size = int(s.size());
      MPI_Send(&string_size, 1, MPI_INT, 0, 42, MPI_COMM_WORLD);
    }
    if (rank == 0) {
      MPI_Wait(&request, MPI_STATUS_IGNORE);
      s.resize(size_t(string_size));
      MPI_Irecv(const_cast<char*>(s.data()), string_size, MPI_CHAR, min_max_rank, 42, MPI_COMM_WORLD, &request);
    }
    if (rank == min_max_rank) {
      MPI_Send(const_cast<char*>(s.data()), string_size, MPI_CHAR, 0, 42, MPI_COMM_WORLD);
    }
    if (rank == 0) {
      MPI_Wait(&request, MPI_STATUS_IGNORE);
      os << s;
    }
#else
    os << s;
#endif
  }
};

struct State {
  StackNode stack_root;
  StackNode* stack_frame;
  Allocations current_allocations[NSPACES];
  Allocations hwm_allocations[NSPACES];
  State():stack_root(nullptr, "", STACK_REGION),stack_frame(&stack_root) {
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
    stack_root.adopt();
    stack_root.reduce_over_mpi();
    if (getenv("KOKKOS_PROFILE_EXPORT_JSON")) {
#if USE_MPI
      int rank;
      MPI_Comm_rank(MPI_COMM_WORLD, &rank);
      if (rank == 0) {
#endif
        std::ofstream fout("noname.json");
        stack_root.print_json(fout);
#if USE_MPI
      }
#endif
        return;
    }

    auto inv_stack_root = stack_root.invert();
    inv_stack_root.reduce_over_mpi();
#if USE_MPI
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0)
#endif
    {
      std::cout << "\nBEGIN KOKKOS PROFILING REPORT:\n";
      std::cout << "TOTAL TIME: " << stack_root.max_runtime << " seconds\n";
      std::cout << "TOP-DOWN TIME TREE:\n";
      std::cout << "<average time> <percent of total time> <percent time in Kokkos> <percent MPI imbalance> <remainder> <kernels per second> <number of calls> <name> [type]\n";
      std::cout << "=================== \n";
      stack_root.print(std::cout);
      std::cout << "BOTTOM-UP TIME TREE:\n";
      std::cout << "<average time> <percent of total time> <percent time in Kokkos> <percent MPI imbalance> <number of calls> <name> [type]\n";
      std::cout << "=================== \n";
      inv_stack_root.print(std::cout);
    }
    for (int space = 0; space < NSPACES; ++space) {
#if USE_MPI
      if (rank == 0)
#endif
      {
        std::cout << "KOKKOS " << get_space_name(space) << " SPACE:\n";
        std::cout << "=================== \n";
        std::cout.flush();
      }
      hwm_allocations[space].print(std::cout);
    }
    print_process_hwm();
#if USE_MPI
    if (rank == 0)
#endif
    {
      std::cout << "END KOKKOS PROFILING REPORT.\n";
      std::cout.flush();
    }
  }
  void begin_frame(const char* name, StackKind kind) {
    std::string name_str(name);
    stack_frame = stack_frame->get_child(std::move(name_str), kind);
    stack_frame->begin();
  }
  void end_frame(Now end_time) {
    stack_frame->end(end_time);
    stack_frame = stack_frame->parent;
  }
  std::uint64_t begin_kernel(const char* name, StackKind kind) {
    begin_frame(name, kind);
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
    end_frame(end_time);
  }
  void push_region(const char* name) {
    begin_frame(name, STACK_REGION);
  }
  void pop_region() {
    end_frame(now());
  }
  void allocate(Space space, const char* name, const void* ptr, std::uint64_t size) {
    current_allocations[space].allocate(
        std::string(name), ptr, size, stack_frame);
    if (current_allocations[space].total_size > hwm_allocations[space].total_size) {
      hwm_allocations[space] = current_allocations[space];
    }
  }
  void deallocate(Space space, const char* name, const void* ptr, std::uint64_t size) {
    current_allocations[space].deallocate(
        std::string(name), ptr, size, stack_frame);
  }
  void begin_deep_copy(
      Space, const char* dst_name, const void*,
      Space, const char* src_name, const void*,
      std::uint64_t) {
    std::string frame_name;
    frame_name += "\"";
    frame_name += dst_name;
    frame_name += "\"=\"";
    frame_name += src_name;
    frame_name += "\"";
    begin_frame(frame_name.c_str(), STACK_COPY);
  }
  void end_deep_copy() {
    end_frame(now());
  }
};

State* global_state = nullptr;

void kokkosp_init_library(
    int /* loadseq */,
    uint64_t /* interfaceVer */,
    uint32_t /* ndevinfos */,
    Kokkos_Profiling_KokkosPDeviceInfo* /* devinfos */) {
  global_state = new State();
}

void kokkosp_finalize_library() {
  delete global_state;
  global_state = nullptr;
}

void kokkosp_begin_parallel_for(
    const char* name, std::uint32_t devid, std::uint64_t* kernid) {
  (void) devid;
  *kernid = global_state->begin_kernel(name, STACK_FOR);
}

void kokkosp_begin_parallel_reduce(
    const char* name, std::uint32_t devid, std::uint64_t* kernid) {
  (void) devid;
  *kernid = global_state->begin_kernel(name, STACK_REDUCE);
}

void kokkosp_begin_parallel_scan(
    const char* name, std::uint32_t devid, std::uint64_t* kernid) {
  (void) devid;
  *kernid = global_state->begin_kernel(name, STACK_SCAN);
}

void kokkosp_end_parallel_for(std::uint64_t kernid) {
  global_state->end_kernel(kernid);
}

void kokkosp_end_parallel_reduce(std::uint64_t kernid) {
  global_state->end_kernel(kernid);
}

void kokkosp_end_parallel_scan(std::uint64_t kernid) {
  global_state->end_kernel(kernid);
}

void kokkosp_push_profile_region(const char* name) {
  global_state->push_region(name);
}

void kokkosp_pop_profile_region() {
  global_state->pop_region();
}

void kokkosp_allocate_data(
    SpaceHandle handle, const char* name, const void* ptr, uint64_t size) {
  auto space = get_space(handle);
  global_state->allocate(space, name, ptr, size);
}

void kokkosp_deallocate_data(
    SpaceHandle handle, const char* name, const void* ptr, uint64_t size) {
  auto space = get_space(handle);
  global_state->deallocate(space, name, ptr, size);
}

void kokkosp_begin_deep_copy(
    SpaceHandle dst_handle, const char* dst_name, const void* dst_ptr,
    SpaceHandle src_handle, const char* src_name, const void* src_ptr,
    uint64_t size) {
  auto dst_space = get_space(dst_handle);
  auto src_space = get_space(src_handle);
  global_state->begin_deep_copy(dst_space, dst_name, dst_ptr,
      src_space, src_name, src_ptr, size);
}

void kokkosp_end_deep_copy() {
  global_state->end_deep_copy();
}

Kokkos::Tools::Experimental::EventSet get_event_set() {
    Kokkos::Tools::Experimental::EventSet my_event_set;
    memset(&my_event_set, 0, sizeof(my_event_set)); // zero any pointers not set here
    my_event_set.init = kokkosp_init_library;
    my_event_set.finalize = kokkosp_finalize_library;
    my_event_set.push_region = kokkosp_push_profile_region;
    my_event_set.pop_region = kokkosp_pop_profile_region;
    my_event_set.allocate_data = kokkosp_allocate_data;
    my_event_set.deallocate_data = kokkosp_deallocate_data;
    my_event_set.begin_deep_copy = kokkosp_begin_deep_copy;
    my_event_set.end_deep_copy = kokkosp_end_deep_copy;
    my_event_set.begin_parallel_for = kokkosp_begin_parallel_for;
    my_event_set.begin_parallel_reduce = kokkosp_begin_parallel_reduce;
    my_event_set.begin_parallel_scan = kokkosp_begin_parallel_scan;
    my_event_set.end_parallel_for = kokkosp_end_parallel_for;
    my_event_set.end_parallel_reduce = kokkosp_end_parallel_reduce;
    my_event_set.end_parallel_scan = kokkosp_end_parallel_scan;
    return my_event_set;
}

}} // KokkosTools::SpaceTimeStack

extern "C" {

namespace impl = KokkosTools::SpaceTimeStack;

EXPOSE_INIT(impl::kokkosp_init_library)
EXPOSE_FINALIZE(impl::kokkosp_finalize_library)
EXPOSE_PUSH_REGION(impl::kokkosp_push_profile_region)
EXPOSE_POP_REGION(impl::kokkosp_pop_profile_region)
EXPOSE_ALLOCATE(impl::kokkosp_allocate_data)
EXPOSE_DEALLOCATE(impl::kokkosp_deallocate_data)
EXPOSE_BEGIN_PARALLEL_FOR(impl::kokkosp_begin_parallel_for)
EXPOSE_END_PARALLEL_FOR(impl::kokkosp_end_parallel_for)
EXPOSE_BEGIN_PARALLEL_SCAN(impl::kokkosp_begin_parallel_scan)
EXPOSE_END_PARALLEL_SCAN(impl::kokkosp_end_parallel_scan)
EXPOSE_BEGIN_PARALLEL_REDUCE(impl::kokkosp_begin_parallel_reduce)
EXPOSE_END_PARALLEL_REDUCE(impl::kokkosp_end_parallel_reduce)

} // extern "C"