/*******************************<GINKGO LICENSE>******************************
Copyright 2017-2018

Karlsruhe Institute of Technology
Universitat Jaume I
University of Tennessee

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
******************************<GINKGO LICENSE>*******************************/

#include <core/solver/cg.hpp>


#include <gtest/gtest.h>


#include <random>


#include <core/base/exception.hpp>
#include <core/base/executor.hpp>
#include <core/matrix/dense.hpp>
#include <core/solver/cg_kernels.hpp>
#include <core/test/utils.hpp>

namespace {


class Cg : public ::testing::Test {
protected:
    using Mtx = gko::matrix::Dense<>;
    void SetUp()
    {
        ASSERT_GT(gko::GpuExecutor::get_num_devices(), 0);
        ref = gko::ReferenceExecutor::create();
        gpu = gko::GpuExecutor::create(0, ref);
    }

    void TearDown()
    {
        if (gpu != nullptr) {
            ASSERT_NO_THROW(gpu->synchronize());
        }
    }

    std::unique_ptr<Mtx> gen_mtx(int num_rows, int num_cols)
    {
        return gko::test::generate_random_matrix<Mtx>(
            ref, num_rows, num_cols,
            std::uniform_int_distribution<>(num_cols, num_cols),
            std::normal_distribution<>(0.0, 1.0), rand_engine);
    }

    void initialize_data()
    {
        int m = 24;
        int n = 7;
        auto b = gen_mtx(m, n);
        auto r = gen_mtx(m, n);
        auto z = gen_mtx(m, n);
        auto p = gen_mtx(m, n);
        auto q = gen_mtx(m, n);
        auto prev_rho = Mtx::create(ref, n, 1);
        auto rho = Mtx::create(ref, n, 1);

        auto d_b = Mtx::create(gpu);
        d_b->copy_from(b.get());
        auto d_r = Mtx::create(gpu);
        d_r->copy_from(r.get());
        auto d_z = Mtx::create(gpu);
        d_z->copy_from(z.get());
        auto d_p = Mtx::create(gpu);
        d_p->copy_from(p.get());
        auto d_q = Mtx::create(gpu);
        d_q->copy_from(q.get());
        auto d_prev_rho = Mtx::create(gpu, n, 1);
        d_prev_rho->copy_from(prev_rho.get());
        auto d_rho = Mtx::create(gpu, n, 1);
        d_rho->copy_from(rho.get());
    }

    void copy_back_data()
    {
        auto b_result = Mtx::create(ref);
        auto r_result = Mtx::create(ref);
        auto z_result = Mtx::create(ref);
        auto p_result = Mtx::create(ref);
        auto q_result = Mtx::create(ref);
        auto prev_rho_result = Mtx::create(ref);
        auto rho_result = Mtx::create(ref);
        b_result->copy_from(d_b.get());
        r_result->copy_from(d_r.get());
        z_result->copy_from(d_z.get());
        p_result->copy_from(d_p.get());
        q_result->copy_from(d_q.get());
        prev_rho_result->copy_from(d_prev_rho.get());
    }

    std::shared_ptr<gko::ReferenceExecutor> ref;
    std::shared_ptr<const gko::GpuExecutor> gpu;

    std::ranlux48 rand_engine;

    std::unique_ptr<Mtx> b;
    std::unique_ptr<Mtx> r;
    std::unique_ptr<Mtx> prev_rho;
    std::unique_ptr<Mtx> rho;
    std::unique_ptr<Mtx> p;
    std::unique_ptr<Mtx> q;
    std::unique_ptr<Mtx> z;
    std::unique_ptr<Mtx> d_b;
    std::unique_ptr<Mtx> d_r;
    std::unique_ptr<Mtx> d_prev_rho;
    std::unique_ptr<Mtx> d_rho;
    std::unique_ptr<Mtx> d_p;
    std::unique_ptr<Mtx> d_q;
    std::unique_ptr<Mtx> d_z;
    std::unique_ptr<Mtx> b_result;
    std::unique_ptr<Mtx> r_result;
    std::unique_ptr<Mtx> prev_rho_result;
    std::unique_ptr<Mtx> rho_result;
    std::unique_ptr<Mtx> p_result;
    std::unique_ptr<Mtx> q_result;
    std::unique_ptr<Mtx> z_result;
};


TEST_F(Cg, GpuCgInitializeIsEquivalentToRef)
{
    /*std::ranlux48 rand_engine(30);


    auto gen_mtx = [&](int m, int n) {
        return gko::test::generate_random_matrix<Mtx>(
            ref, m, n, std::uniform_int_distribution<>(1, 1),
            std::normal_distribution<>(0.0, 1.0), rand_engine);
    };

    int m = 24;
    int n = 7;

    auto b = gen_mtx(m, n);
    auto r = gen_mtx(m, n);
    auto z = gen_mtx(m, n);
    auto p = gen_mtx(m, n);
    auto q = gen_mtx(m, n);
    auto prev_rho = Mtx::create(ref, n, 1);
    auto rho = Mtx::create(ref, n, 1);

    auto d_b = Mtx::create(gpu);
    auto d_r = Mtx::create(gpu);
    auto d_z = Mtx::create(gpu);
    auto d_p = Mtx::create(gpu);
    auto d_q = Mtx::create(gpu);
    auto d_prev_rho = Mtx::create(gpu, n, 1);
    auto d_rho = Mtx::create(gpu, n, 1);
    d_b->copy_from(b.get());
    d_r->copy_from(r.get());
    d_z->copy_from(z.get());
    d_p->copy_from(p.get());
    d_q->copy_from(q.get());
    d_prev_rho->copy_from(prev_rho.get());
    d_rho->copy_from(rho.get());
*/
    initialize_data();
    gko::kernels::reference::cg::initialize(b.get(), r.get(), z.get(), p.get(),
                                            q.get(), prev_rho.get(), rho.get());
    gko::kernels::gpu::cg::initialize(d_b.get(), d_r.get(), d_z.get(),
                                      d_p.get(), d_q.get(), d_prev_rho.get(),
                                      d_rho.get());

    /*auto b_result = Mtx::create(ref);
    auto r_result = Mtx::create(ref);
    auto z_result = Mtx::create(ref);
    auto p_result = Mtx::create(ref);
    auto q_result = Mtx::create(ref);
    auto prev_rho_result = Mtx::create(ref);
    auto rho_result = Mtx::create(ref);
    b_result->copy_from(d_b.get());
    r_result->copy_from(d_r.get());
    z_result->copy_from(d_z.get());
    p_result->copy_from(d_p.get());
    q_result->copy_from(d_q.get());
    prev_rho_result->copy_from(d_prev_rho.get());
    rho_result->copy_from(d_rho.get());
*/
    copy_back_data();
    ASSERT_MTX_NEAR(b_result, b, 1e-14);
    ASSERT_MTX_NEAR(r_result, r, 1e-14);
    ASSERT_MTX_NEAR(z_result, z, 1e-14);
    ASSERT_MTX_NEAR(p_result, p, 1e-14);
    ASSERT_MTX_NEAR(q_result, q, 1e-14);
    ASSERT_MTX_NEAR(prev_rho_result, prev_rho, 1e-14);
    ASSERT_MTX_NEAR(rho_result, rho, 1e-14);
}


TEST_F(Cg, GpuCgStep1IsEquivalentToRef)
{
    std::ranlux48 rand_engine(30);


    auto gen_mtx = [&](int m, int n) {
        return gko::test::generate_random_matrix<Mtx>(
            ref, m, n, std::uniform_int_distribution<>(1, 1),
            std::normal_distribution<>(0.0, 1.0), rand_engine);
    };

    int m = 24;
    int n = 7;

    auto p = gen_mtx(m, n);
    auto z = gen_mtx(m, n);
    auto rho = gen_mtx(n, 1);
    auto prev_rho = gen_mtx(n, 1);

    auto d_p = Mtx::create(gpu);
    auto d_z = Mtx::create(gpu);
    auto d_prev_rho = Mtx::create(gpu, n, 1);
    auto d_rho = Mtx::create(gpu, n, 1);
    d_p->copy_from(p.get());
    d_z->copy_from(z.get());
    d_rho->copy_from(rho.get());
    d_prev_rho->copy_from(prev_rho.get());

    gko::kernels::reference::cg::step_1(p.get(), z.get(), rho.get(),
                                        prev_rho.get());
    gko::kernels::gpu::cg::step_1(d_p.get(), d_z.get(), d_rho.get(),
                                  d_prev_rho.get());

    auto p_result = Mtx::create(ref);
    auto z_result = Mtx::create(ref);
    p_result->copy_from(d_p.get());
    z_result->copy_from(d_z.get());

    ASSERT_MTX_NEAR(p_result, p, 1e-14);
    ASSERT_MTX_NEAR(z_result, z, 1e-14);
}


TEST_F(Cg, GpuCgStep2IsEquivalentToRef)
{
    std::ranlux48 rand_engine(30);


    auto gen_mtx = [&](int m, int n) {
        return gko::test::generate_random_matrix<Mtx>(
            ref, m, n, std::uniform_int_distribution<>(1, 1),
            std::normal_distribution<>(0.0, 1.0), rand_engine);
    };

    int m = 24;
    int n = 7;

    auto x = gen_mtx(m, n);
    auto r = gen_mtx(m, n);
    auto p = gen_mtx(m, n);
    auto q = gen_mtx(m, n);
    auto beta = gen_mtx(n, 1);
    auto rho = gen_mtx(n, 1);

    auto d_x = Mtx::create(gpu);
    auto d_r = Mtx::create(gpu);
    auto d_p = Mtx::create(gpu);
    auto d_q = Mtx::create(gpu);
    auto d_beta = Mtx::create(gpu, n, 1);
    auto d_rho = Mtx::create(gpu, n, 1);
    d_x->copy_from(x.get());
    d_r->copy_from(r.get());
    d_p->copy_from(p.get());
    d_q->copy_from(q.get());
    d_beta->copy_from(beta.get());
    d_rho->copy_from(rho.get());

    gko::kernels::reference::cg::step_2(x.get(), r.get(), p.get(), q.get(),
                                        beta.get(), rho.get());
    gko::kernels::gpu::cg::step_2(d_x.get(), d_r.get(), d_p.get(), d_q.get(),
                                  d_beta.get(), d_rho.get());

    auto x_result = Mtx::create(ref);
    auto r_result = Mtx::create(ref);
    auto p_result = Mtx::create(ref);
    auto q_result = Mtx::create(ref);
    x_result->copy_from(d_x.get());
    r_result->copy_from(d_r.get());
    p_result->copy_from(d_p.get());
    q_result->copy_from(d_q.get());

    ASSERT_MTX_NEAR(x_result, x, 1e-14);
    ASSERT_MTX_NEAR(r_result, r, 1e-14);
    ASSERT_MTX_NEAR(p_result, p, 1e-14);
    ASSERT_MTX_NEAR(q_result, q, 1e-14);
}


}  // namespace
