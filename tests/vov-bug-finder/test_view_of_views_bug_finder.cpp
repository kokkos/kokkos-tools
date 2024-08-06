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

#include "Kokkos_Core.hpp"
#include "gtest/gtest.h"

void test_no_throw_placement_new_in_parallel_for() {
  ASSERT_NO_THROW(({
    using V = Kokkos::View<int *>;
    Kokkos::View<V **, Kokkos::HostSpace> vov(
        Kokkos::view_alloc("vov", Kokkos::WithoutInitializing), 2, 3);
    V a("a", 4);
    V b("b", 5);
    Kokkos::parallel_for(
        "Fine", Kokkos::RangePolicy<Kokkos::DefaultHostExecutionSpace>(0, 1),
        KOKKOS_LAMBDA(int) {
          new (&vov(0, 0)) V(a);
          new (&vov(0, 1)) V(a);
          new (&vov(1, 0)) V(b);
        });
  }));
}

void test_death_allocation_in_parallel_for() {
  ASSERT_DEATH(
      ({
        using V = Kokkos::View<int *>;
        Kokkos::View<V **, Kokkos::HostSpace> vov(
            Kokkos::view_alloc("vov", Kokkos::WithoutInitializing), 2, 3);
        V a("a", 4);
        new (&vov(0, 0)) V(a);
        new (&vov(0, 1)) V(a);
        Kokkos::parallel_for(
            "AllocatesInParallel]For",
            Kokkos::RangePolicy<Kokkos::DefaultHostExecutionSpace>(0, 1),
            KOKKOS_LAMBDA(int) {
              V b("b", 5);
              new (&vov(1, 0)) V(b);
            });
      }),
      "allocating \"b\" within parallel region \"AllocatesInParallel]For\"");
}

void test_no_throw_team_scratch_pad_parallel_for() {
  ASSERT_NO_THROW(({
    Kokkos::parallel_for(
        "L0",
        Kokkos::TeamPolicy<>(1, Kokkos::AUTO)
            .set_scratch_size(0, Kokkos::PerTeam(1000)),
        KOKKOS_LAMBDA(Kokkos::TeamPolicy<>::member_type const &){});

    Kokkos::parallel_for(
        "L1",
        Kokkos::TeamPolicy<>(1, Kokkos::AUTO)
            .set_scratch_size(1, Kokkos::PerTeam(1000)),
        KOKKOS_LAMBDA(Kokkos::TeamPolicy<>::member_type const &){});
  }));
}

// TODO initialize in main and split unit tests
TEST(ViewOfViews, find_bugs) {
  Kokkos::initialize();
  {
    ASSERT_NO_THROW(({
      using V = Kokkos::View<int *>;
      Kokkos::View<V **, Kokkos::HostSpace> vov("vov", 2, 3);
      V a("a", 4);
      V b("b", 5);
      vov(0, 0) = a;
      vov(0, 1) = a;
      vov(1, 0) = b;

      vov(0, 0) = V();
      vov(0, 1) = V();
      vov(1, 0) = V();
    }));

    ASSERT_NO_THROW(({
      using V = Kokkos::View<int *>;
      Kokkos::View<V **, Kokkos::HostSpace> vov(
          Kokkos::view_alloc("vov", Kokkos::WithoutInitializing), 2, 3);
      V a("a", 4);
      V b("b", 5);
      new (&vov(0, 0)) V(a);
      new (&vov(0, 1)) V(a);
      new (&vov(1, 0)) V(b);

      vov(0, 0).~V();
      vov(0, 1).~V();
      // vov(1, 0).~V();
      // ^ leaking "b" but not caught by the tool
    }));

    ASSERT_DEATH(({
                   using V = Kokkos::View<int *>;
                   Kokkos::View<V **, Kokkos::HostSpace> vov("vo]v", 2, 3);
                   // ^ included a closing square bracket in the label to try
                   // to trip the substring extraction
                   V a("a", 4);
                   V b("b", 5);
                   vov(0, 0) = a;
                   vov(0, 1) = a;
                   vov(1, 0) = b;
                 }),
                 "view of views \"vo]v\" not properly cleared");

    test_no_throw_placement_new_in_parallel_for();

    test_death_allocation_in_parallel_for();

    test_no_throw_team_scratch_pad_parallel_for();
  }
  Kokkos::finalize();
}
