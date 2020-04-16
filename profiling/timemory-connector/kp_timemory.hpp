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

#pragma once

#include <random>
#include <timemory/timemory.hpp>

//--------------------------------------------------------------------------------------//

namespace tim
{
namespace component
{
//--------------------------------------------------------------------------------------//
/// \class rand_intercept
/// \brief this component can be used to replace the C implementation of rand()
///
struct rand_intercept : public base<rand_intercept, void>
{
    using random_engine_t = std::default_random_engine;
    using distribution_t  = std::uniform_int_distribution<int>;
    using limits_t        = std::numeric_limits<int>;

    static void message()
    {
        // deliver one-time message
        static int num = (puts("intercepting rand..."), 0);
        consume_parameters(num);
    }

    static random_engine_t& get_generator()
    {
        static random_engine_t generator(tim::get_env("RANDOM_SEED", time(NULL)));
        return generator;
    }

    static int get_rand()
    {
        static auto&          generator = get_generator();
        static distribution_t dist(0, limits_t::max());
        return dist(generator);
    }

    rand_intercept() { message(); }

    // replaces 'void srand(unsigned)' with STL implementation
    void operator()(unsigned rseed) { get_generator() = random_engine_t(rseed); }

    // replaces 'int rand(void)' with STL implementation
    int operator()() { return get_rand(); }
};

//--------------------------------------------------------------------------------------//

}  // namespace component
}  // namespace tim

//--------------------------------------------------------------------------------------//

using namespace tim::component;

using empty_t       = tim::component_tuple<>;
using counter_t     = tim::component_tuple<trip_count>;
using rand_gotcha_t = tim::component::gotcha<2, empty_t, rand_intercept>;
using misc_gotcha_t = tim::component::gotcha<8, counter_t, void>;

//--------------------------------------------------------------------------------------//

using gotcha_tools_t = tim::component_tuple<rand_gotcha_t, misc_gotcha_t>;

//--------------------------------------------------------------------------------------//
