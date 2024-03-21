//@HEADER
// ************************************************************************
//
//                        Kokkos v. 4.0
//       Copyright (2022) National Technology & Engineering
//               Solutions of Sandia, LLC (NTESS).
//
// Under the terms of Contract DE-NA0003525 with NTESS,
// the U.S. Government retains certain rights in this software.
//
// Part of Kokkos, under the Apache License v2.0 with LLVM Exceptions.
// See https://kokkos.org/LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//@HEADER

#ifndef KOKKOSTOOLS_PROFILING_SPACETIMESTACK_HPP
#define KOKKOSTOOLS_PROFILING_SPACETIMESTACK_HPP

#include <chrono>

namespace KokkosTools::SpaceTimeStack {

enum Space { SPACE_HOST, SPACE_CUDA, SPACE_HIP, SPACE_SYCL, SPACE_OMPT };

enum { NSPACES = 5 };

enum StackKind {
  STACK_FOR,
  STACK_REDUCE,
  STACK_SCAN,
  STACK_REGION,
  STACK_COPY
};

class StackNode {
 public:
  using clock_type = std::chrono::steady_clock;
  using time_point = typename clock_type::time_point;

  //! Similar to @c std::chrono::seconds, but represented using @c double.
  using seconds = std::chrono::duration<double, std::ratio<1, 1>>;

 public:
  StackNode* parent;
  std::string name;
  StackKind kind;
  std::set<StackNode> children;
  double total_runtime;
  double total_kokkos_runtime;
  double max_runtime;
  double avg_runtime;
  std::int64_t number_of_calls;
  std::int64_t total_number_of_kernel_calls;  // Counts all kernel calls (but
                                              // not region calls) this node and
                                              // below this node in the tree
  time_point start_time;

 public:
  StackNode(StackNode* parent_in, std::string&& name_in, StackKind kind_in);

  StackNode* get_child(std::string&& child_name, StackKind child_kind);

  bool operator<(StackNode const& other) const;

  std::string get_full_name() const;

  void begin();

  void end(time_point const& end_time);

  void adopt();

  StackNode invert() const;

  void print_recursive_json(std::ostream& os, StackNode const* parent,
                            double tree_time) const;

  void print_json(std::ostream& os) const;

  void print_recursive(std::ostream& os, std::string my_indent,
                       std::string const& child_indent, double tree_time) const;

  void print(std::ostream& os) const;

  void reduce_over_mpi(bool mpi_usable);
};

struct Allocation {
  std::string name;
  const void* ptr;
  std::uint64_t size;
  StackNode* frame;
  Allocation(std::string&& name_in, const void* ptr_in, std::uint64_t size_in,
             StackNode* frame_in)
      : name(std::move(name_in)), ptr(ptr_in), size(size_in), frame(frame_in) {}
  bool operator<(Allocation const& other) const {
    if (size != other.size) return size > other.size;
    return ptr < other.ptr;
  }
};

struct Allocations {
  std::uint64_t total_size;
  std::set<Allocation> alloc_set;

  Allocations() : total_size(0) {}

  void allocate(std::string&& name, const void* ptr, std::uint64_t size,
                StackNode* frame);

  void deallocate(std::string&& name, const void* ptr, std::uint64_t size,
                  StackNode* frame);

  void print(std::ostream& os, bool mpi_usable);
};

class State {
 private:
  StackNode stack_root;
  StackNode* stack_frame;
  Allocations current_allocations[NSPACES];
  Allocations hwm_allocations[NSPACES];

 public:
  State();

  ~State();

  const StackNode& getCurrentStackFrame() const { return *stack_frame; }

  void begin_frame(const char* name, StackKind kind);

  void end_frame(const StackNode::time_point& end_time);

  std::uint64_t begin_kernel(const char* name, StackKind kind);

  void end_kernel(std::uint64_t kernid);

  void push_region(const char* name);

  void pop_region();

  void allocate(Space space, const char* name, const void* ptr,
                std::uint64_t size);

  void deallocate(Space space, const char* name, const void* ptr,
                  std::uint64_t size);

  void begin_deep_copy(Space dst_space, const char* dst_name, const void*,
                       Space src_space, const char* src_name, const void*,
                       std::uint64_t);

  void end_deep_copy();

  static State& get() { return *global_state; }

  static void initialize();

  static void finalize();

 private:
  static std::unique_ptr<State> global_state;
};

}  // namespace KokkosTools::SpaceTimeStack

#endif  // KOKKOSTOOLS_PROFILING_SPACETIMESTACK_HPP
