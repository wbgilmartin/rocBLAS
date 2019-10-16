#ifdef USE_TENSILE_HOST

#include "tensile_host.hpp"
#include "rocblas.h"
#include <Tensile/Contractions.hpp>
#include <Tensile/EmbeddedLibrary.hpp>
#include <Tensile/MasterSolutionLibrary.hpp>
#include <Tensile/Tensile.hpp>
#include <Tensile/TensorDescriptor.hpp>
#include <Tensile/Utils.hpp>
#include <Tensile/hip/HipHardware.hpp>
#include <Tensile/hip/HipSolutionAdapter.hpp>
#include <Tensile/hip/HipUtils.hpp>
#include <glob.h>
#include <iostream>
#include <string>

template <typename>
static constexpr auto tensile_datatype = nullptr;

template <>
static constexpr auto tensile_datatype<rocblas_half> = Tensile::DataType::Half;

template <>
static constexpr auto tensile_datatype<float> = Tensile::DataType::Float;

template <>
static constexpr auto tensile_datatype<double> = Tensile::DataType::Double;

template <>
static constexpr auto tensile_datatype<rocblas_float_complex> = Tensile::DataType::ComplexFloat;

template <>
static constexpr auto tensile_datatype<rocblas_double_complex> = Tensile::DataType::ComplexDouble;

// return the value category for a value, such as whether it's 0 or 1
template <typename T>
constexpr auto value_category(const T& beta)
{
    return beta == T(0) ? 0.0 : beta == T(1) ? 1.0 : -12345.0;
}

template <typename T>
auto create_gemm_contraction_problem_strided_batched(rocblas_operation trans_a,
                                                     rocblas_operation trans_b,
                                                     size_t            m,
                                                     size_t            n,
                                                     size_t            k,
                                                     const T           alpha,
                                                     const T*          A,
                                                     size_t            ld_a,
                                                     size_t            stride_a,
                                                     const T*          B,
                                                     size_t            ld_b,
                                                     size_t            stride_b,
                                                     const T           beta,
                                                     T*                C,
                                                     size_t            ld_c,
                                                     size_t            stride_c,
                                                     size_t            batchSize)
{
    auto transposeA = trans_a != rocblas_operation_none;
    auto transposeB = trans_b != rocblas_operation_none;

    auto dt      = tensile_datatype<T>;
    auto problem = Tensile::ContractionProblem::GEMM_Strides(transposeA,
                                                             transposeB,
                                                             dt,
                                                             dt,
                                                             dt,
                                                             dt,
                                                             m,
                                                             n,
                                                             k,
                                                             batchSize,
                                                             ld_a,
                                                             stride_a,
                                                             ld_b,
                                                             stride_b,
                                                             ld_c,
                                                             stride_c,
                                                             ld_c,
                                                             stride_c,
                                                             value_category(beta));

    return problem;
}

// construct the gemm contraction problem
template <typename T>
auto create_gemm_contraction_problem(rocblas_operation trans_a,
                                     rocblas_operation trans_b,
                                     size_t            m,
                                     size_t            n,
                                     size_t            k,
                                     const T           alpha,
                                     const T*          A,
                                     size_t            ld_a,
                                     const T*          B,
                                     size_t            ld_b,
                                     const T           beta,
                                     T*                C,
                                     size_t            ld_c)
{
    auto transposeA = trans_a != rocblas_operation_none;
    auto transposeB = trans_b != rocblas_operation_none;

    Tensile::ContractionProblem::FreeIndices freeIndex(2);
    Tensile::ContractionProblem::BoundIndex  boundIndex;

    freeIndex[0].isA = true;
    freeIndex[0].i = freeIndex[0].c = freeIndex[0].d = 0;
    freeIndex[1].isA                                 = false;
    freeIndex[1].i = freeIndex[1].c = freeIndex[1].d = 1;

    Tensile::TensorDescriptor a, b;
    auto                      dt = tensile_datatype<T>;

    if(transposeA)
    {
        a              = {dt, {k, m}, {1, ld_a}};
        freeIndex[0].i = 1;
        boundIndex.a   = 0;
    }
    else
    {
        a              = {dt, {m, k}, {1, ld_a}};
        freeIndex[0].i = 0;
        boundIndex.a   = 1;
    }

    if(transposeB)
    {
        b              = {dt, {n, k}, {1, ld_b}};
        freeIndex[1].i = 0;
        boundIndex.b   = 1;
    }
    else
    {
        b              = {dt, {k, n}, {1, ld_b}};
        freeIndex[1].i = 1;
        boundIndex.b   = 0;
    }

    Tensile::ContractionProblem::FreeIndices  freeIndices{freeIndex};
    Tensile::ContractionProblem::BatchIndices batchIndices;
    Tensile::ContractionProblem::BoundIndices boundIndices{boundIndex};

    batchIndices.push_back({2, 2, 2, 2});

    Tensile::TensorDescriptor c{dt, {m, n}, {1, ld_c}};

    auto batchCount = 1;

    a.appendDim(batchCount);
    b.appendDim(batchCount);
    c.appendDim(batchCount);

    Tensile::TensorOps aops;
    if(is_complex<T> && trans_a == rocblas_operation_conjugate_transpose)
        aops = {Tensile::TensorOp::Type::ComplexConjugate};

    Tensile::TensorOps bops;
    if(is_complex<T> && trans_b == rocblas_operation_conjugate_transpose)
        bops = {Tensile::TensorOp::Type::ComplexConjugate};

    return Tensile::ContractionProblem(a,
                                       aops,
                                       b,
                                       bops,
                                       c,
                                       {},
                                       c,
                                       {},
                                       freeIndices,
                                       batchIndices,
                                       boundIndices,
                                       value_category(beta));
}

template <typename T>
auto ConstructTensileProblem(RocblasContractionProblem<T>* problem)
{
    Tensile::ContractionProblem tensile_problem;
    switch(problem->problem_type)
    {
    case GEMM:
        tensile_problem = create_gemm_contraction_problem<T>(problem->trans_a,
                                                             problem->trans_b,
                                                             problem->m,
                                                             problem->n,
                                                             problem->k,
                                                             problem->alpha,
                                                             problem->A,
                                                             problem->ld_a,
                                                             problem->B,
                                                             problem->ld_b,
                                                             problem->beta,
                                                             problem->C,
                                                             problem->ld_c);
        break;
    case GEMMStridedBatch:
        tensile_problem = create_gemm_contraction_problem_strided_batched(problem->trans_a,
                                                                          problem->trans_b,
                                                                          problem->m,
                                                                          problem->n,
                                                                          problem->k,
                                                                          problem->alpha,
                                                                          problem->A,
                                                                          problem->ld_a,
                                                                          problem->stride_a,
                                                                          problem->B,
                                                                          problem->ld_b,
                                                                          problem->stride_b,
                                                                          problem->beta,
                                                                          problem->C,
                                                                          problem->ld_c,
                                                                          problem->stride_c,
                                                                          problem->batch_size);
        break;
    }

    return tensile_problem;
}

template <typename T>
struct rocblas_to_tensile_type
{
    using type = T;
};

template <>
struct rocblas_to_tensile_type<rocblas_float_complex>
{
    using type = std::complex<float>;
};

template <>
struct rocblas_to_tensile_type<rocblas_double_complex>
{
    using type = std::complex<double>;
};

template <>
struct rocblas_to_tensile_type<rocblas_half>
{
    using type = Tensile::Half;
};

template <typename T>
auto GetTensileInputs(RocblasContractionProblem<T>* problem)
{
    using tensile_type = typename rocblas_to_tensile_type<T>::type;
    Tensile::TypedContractionInputs<tensile_type> inputs;
    switch(problem->problem_type)
    {
    case GEMM:
    case GEMMStridedBatch:
        inputs.a     = reinterpret_cast<const tensile_type*>(problem->A);
        inputs.b     = reinterpret_cast<const tensile_type*>(problem->B);
        inputs.c     = reinterpret_cast<tensile_type*>(problem->C);
        inputs.d     = reinterpret_cast<tensile_type*>(problem->C);
        inputs.alpha = static_cast<tensile_type>(problem->alpha);
        inputs.beta  = static_cast<tensile_type>(problem->beta);
        break;
    }

    return inputs;
}

struct TensileHostImpl : TensileHost
{
    void initializeHost(const char* lib_path)
    {
        std::string path(lib_path);
        std::string dir = path + "/*co";

        glob_t glob_result;
        glob(dir.c_str(), GLOB_TILDE, NULL, &glob_result);
        for(unsigned int i = 0; i < glob_result.gl_pathc; ++i)
            adapter.loadCodeObjectFile(glob_result.gl_pathv[i]);
        globfree(&glob_result);

        std::string filename = path + "/TensileLibrary.yaml";
        library              = std::dynamic_pointer_cast<
            Tensile::MasterSolutionLibrary<Tensile::ContractionProblem>>(
            Tensile::LoadLibraryFile<Tensile::ContractionProblem>(filename));

        hardware = Tensile::hip::GetCurrentDevice();
    }

    std::shared_ptr<Tensile::MasterSolutionLibrary<Tensile::ContractionProblem>> library;
    std::shared_ptr<Tensile::Hardware>                                           hardware;
    Tensile::hip::SolutionAdapter                                                adapter;

    ~TensileHostImpl() override = default; // Ensure virtual base class
};

template <typename T>
rocblas_status TensileHostCall<T>::runContractionProblem(RocblasContractionProblem<T>* problem,
                                                         TensileHost*                  host)
try
{
    auto tensile_problem = ConstructTensileProblem<T>(problem);
    auto inputs          = GetTensileInputs<T>(problem);
    auto hosti           = dynamic_cast<TensileHostImpl*>(host);
    if(!hosti)
        return rocblas_status_internal_error;

    auto solution = hosti->library->findBestSolution(tensile_problem, *hosti->hardware);
    auto result   = solution->solve(tensile_problem, inputs, *hosti->hardware);
    hosti->adapter.launchKernels(result);
    return rocblas_status_success;
}
catch(...)
{
    return rocblas_status_internal_error;
}

TensileHost* createTensileHost()
{
    return new TensileHostImpl();
}

template <typename T>
rocblas_status callTensileContraction(RocblasContractionProblem<T>* problem, TensileHost* host)
{
    TensileHostCall<T> hostCaller;
    return hostCaller.runContractionProblem(problem, host);
}

template rocblas_status callTensileContraction(RocblasContractionProblem<rocblas_half>* problem,
                                               TensileHost*                             host);

template rocblas_status callTensileContraction(RocblasContractionProblem<float>* problem,
                                               TensileHost*                      host);

template rocblas_status callTensileContraction(RocblasContractionProblem<double>* problem,
                                               TensileHost*                       host);

template rocblas_status
    callTensileContraction(RocblasContractionProblem<rocblas_float_complex>* problem,
                           TensileHost*                                      host);

template rocblas_status
    callTensileContraction(RocblasContractionProblem<rocblas_double_complex>* problem,
                           TensileHost*                                       host);

#endif
