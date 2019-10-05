/* ************************************************************************
 * Copyright 2016-2019 Advanced Micro Devices, Inc.
 * ************************************************************************ */
#include "Tensile.h"
#include "gemm.hpp"
#include "handle.h"
#include "logging.h"
#include "rocblas.h"
#include "utility.h"
#include <sys/time.h>

namespace
{

    template <typename>
    static constexpr char rocblas_gemm_strided_batched_name[] = "unknown";

    template <>
    static constexpr char rocblas_gemm_strided_batched_name<rocblas_half>[]
        = "rocblas_hgemm_strided_batched";

    template <>
    static constexpr char rocblas_gemm_strided_batched_name<float>[]
        = "rocblas_sgemm_strided_batched";

    template <>
    static constexpr char rocblas_gemm_strided_batched_name<double>[]
        = "rocblas_dgemm_strided_batched";

    template <>
    static constexpr char rocblas_gemm_strided_batched_name<rocblas_float_complex>[]
        = "rocblas_cgemm_strided_batched";

    template <>
    static constexpr char rocblas_gemm_strided_batched_name<rocblas_double_complex>[]
        = "rocblas_zgemm_strided_batched";

    /*******************************************************************************
    * Strided / Batched GEMM implementation
    ******************************************************************************/
    template <typename T>
    rocblas_status rocblas_gemm_strided_batched_impl(rocblas_handle    handle,
                                                     rocblas_operation trans_a,
                                                     rocblas_operation trans_b,
                                                     rocblas_int       m,
                                                     rocblas_int       n,
                                                     rocblas_int       k,
                                                     const T*          alpha,
                                                     const T*          A,
                                                     rocblas_int       ld_a,
                                                     rocblas_stride    stride_a,
                                                     const T*          B,
                                                     rocblas_int       ld_b,
                                                     rocblas_stride    stride_b,
                                                     const T*          beta,
                                                     T*                C,
                                                     rocblas_int       ld_c,
                                                     rocblas_stride    stride_c,
                                                     rocblas_int       b_c)

    {
        if(!handle)
            return rocblas_status_invalid_handle;
        RETURN_ZERO_DEVICE_MEMORY_SIZE_IF_QUERIED(handle);

        auto layer_mode = handle->layer_mode;

        if(layer_mode
           & (rocblas_layer_mode_log_trace | rocblas_layer_mode_log_bench
              | rocblas_layer_mode_log_profile))
        {
            auto trans_a_letter = rocblas_transpose_letter(trans_a);
            auto trans_b_letter = rocblas_transpose_letter(trans_b);

            if(handle->pointer_mode == rocblas_pointer_mode_host)
            {
                if(layer_mode & rocblas_layer_mode_log_trace)
                    log_trace(handle,
                              rocblas_gemm_strided_batched_name<T>,
                              trans_a,
                              trans_b,
                              m,
                              n,
                              k,
                              *alpha,
                              A,
                              ld_a,
                              stride_a,
                              B,
                              ld_b,
                              stride_b,
                              *beta,
                              C,
                              ld_c,
                              stride_c,
                              b_c);

                if(layer_mode & rocblas_layer_mode_log_bench)
                {
                    std::stringstream alphass;
                    alphass << "--alpha " << std::real(*alpha);
                    if(std::imag(*alpha) != 0)
                        alphass << " --alphai " << std::imag(*alpha);

                    std::stringstream betass;
                    betass << "--beta " << std::real(*beta);
                    if(std::imag(*beta) != 0)
                        betass << " --betai " << std::imag(*beta);

                    log_bench(handle,
                              "./rocblas-bench -f gemm_strided_batched -r",
                              rocblas_precision_string<T>,
                              "--transposeA",
                              trans_a_letter,
                              "--transposeB",
                              trans_b_letter,
                              "-m",
                              m,
                              "-n",
                              n,
                              "-k",
                              k,
                              alphass.str(),
                              "--lda",
                              ld_a,
                              "--stride_a",
                              stride_a,
                              "--ldb",
                              ld_b,
                              "--stride_b",
                              stride_b,
                              betass.str(),
                              "--ldc",
                              ld_c,
                              "--stride_c",
                              stride_c,
                              "--batch",
                              b_c);
                }
            }
            else
            {
                if(layer_mode & rocblas_layer_mode_log_trace)
                {
                    log_trace(handle,
                              rocblas_gemm_strided_batched_name<T>,
                              trans_a,
                              trans_b,
                              m,
                              n,
                              k,
                              alpha,
                              A,
                              ld_a,
                              stride_a,
                              B,
                              ld_b,
                              stride_b,
                              beta,
                              C,
                              ld_c,
                              stride_c,
                              b_c);
                }
            }

            if(layer_mode & rocblas_layer_mode_log_profile)
            {
                log_profile(handle,
                            rocblas_gemm_strided_batched_name<T>,
                            "transA",
                            trans_a_letter,
                            "transB",
                            trans_b_letter,
                            "M",
                            m,
                            "N",
                            n,
                            "K",
                            k,
                            "lda",
                            ld_a,
                            "stride_a",
                            stride_a,
                            "ldb",
                            ld_b,
                            "stride_b",
                            stride_b,
                            "ldc",
                            ld_c,
                            "stride_c",
                            stride_c,
                            "batch_count",
                            b_c);
            }
        }

        rocblas_status validArgs = validateArgs(handle,
                                                trans_a,
                                                trans_b,
                                                m,
                                                n,
                                                k,
                                                alpha,
                                                A,
                                                ld_a,
                                                stride_a,
                                                B,
                                                ld_b,
                                                stride_b,
                                                beta,
                                                C,
                                                ld_c,
                                                stride_c,
                                                b_c);

        if(validArgs != rocblas_status_success)
            return validArgs;

        return rocblas_gemm_template<false, true>(handle,
                                                  trans_a,
                                                  trans_b,
                                                  m,
                                                  n,
                                                  k,
                                                  alpha,
                                                  0,
                                                  A,
                                                  0,
                                                  ld_a,
                                                  stride_a,
                                                  B,
                                                  0,
                                                  ld_b,
                                                  stride_b,
                                                  beta,
                                                  0,
                                                  C,
                                                  0,
                                                  ld_c,
                                                  stride_c,
                                                  b_c);
    }

    /*******************************************************************************
    * Batched / Strided GEMM Kernel name implementation
    ******************************************************************************/
    template <typename T>
    rocblas_status rocblas_gemm_strided_batched_kernel_name_impl(rocblas_handle    handle,
                                                                 rocblas_operation trans_a,
                                                                 rocblas_operation trans_b,
                                                                 rocblas_int       m,
                                                                 rocblas_int       n,
                                                                 rocblas_int       k,
                                                                 const T*          alpha,
                                                                 const T*          A,
                                                                 rocblas_int       ld_a,
                                                                 rocblas_stride    stride_a,
                                                                 const T*          B,
                                                                 rocblas_int       ld_b,
                                                                 rocblas_stride    stride_b,
                                                                 const T*          beta,
                                                                 T*                C,
                                                                 rocblas_int       ld_c,
                                                                 rocblas_stride    stride_c,
                                                                 rocblas_int       b_c)
    {
        if(!handle)
            return rocblas_status_invalid_handle;
        RETURN_ZERO_DEVICE_MEMORY_SIZE_IF_QUERIED(handle);

        auto layer_mode = handle->layer_mode;

        if(layer_mode
           & (rocblas_layer_mode_log_trace | rocblas_layer_mode_log_bench
              | rocblas_layer_mode_log_profile))
        {
            auto trans_a_letter = rocblas_transpose_letter(trans_a);
            auto trans_b_letter = rocblas_transpose_letter(trans_b);

            if(handle->pointer_mode == rocblas_pointer_mode_host)
            {
                if(layer_mode & rocblas_layer_mode_log_trace)
                    log_trace(handle,
                              rocblas_gemm_strided_batched_name<T>,
                              trans_a,
                              trans_b,
                              m,
                              n,
                              k,
                              *alpha,
                              A,
                              ld_a,
                              stride_a,
                              B,
                              ld_b,
                              stride_b,
                              *beta,
                              C,
                              ld_c,
                              stride_c,
                              b_c);

                if(layer_mode & rocblas_layer_mode_log_bench)
                    log_bench(handle,
                              "./rocblas-bench -f gemm_strided_batched -r",
                              rocblas_precision_string<T>,
                              "--transposeA",
                              trans_a_letter,
                              "--transposeB",
                              trans_b_letter,
                              "-m",
                              m,
                              "-n",
                              n,
                              "-k",
                              k,
                              "--alpha",
                              *alpha,
                              "--lda",
                              ld_a,
                              "--bsa",
                              stride_a,
                              "--ldb",
                              ld_b,
                              "--bsb",
                              stride_b,
                              "--beta",
                              *beta,
                              "--ldc",
                              ld_c,
                              "--bsc",
                              stride_c,
                              "--batch",
                              b_c);
            }
            else
            {
                if(layer_mode & rocblas_layer_mode_log_trace)
                    log_trace(handle,
                              rocblas_gemm_strided_batched_name<T>,
                              trans_a,
                              trans_b,
                              m,
                              n,
                              k,
                              alpha,
                              A,
                              ld_a,
                              stride_a,
                              B,
                              ld_b,
                              stride_b,
                              beta,
                              C,
                              ld_c,
                              stride_c,
                              b_c);
            }

            if(layer_mode & rocblas_layer_mode_log_profile)
                log_profile(handle,
                            rocblas_gemm_strided_batched_name<T>,
                            "transA",
                            trans_a_letter,
                            "transB",
                            trans_b_letter,
                            "M",
                            m,
                            "N",
                            n,
                            "K",
                            k,
                            "lda",
                            ld_a,
                            "stride_a",
                            stride_a,
                            "ldb",
                            ld_b,
                            "stride_b",
                            stride_b,
                            "ldc",
                            ld_c,
                            "stride_c",
                            stride_c,
                            "batch_count",
                            b_c);
        }

        rocblas_status validArgs = validateArgs(handle,
                                                trans_a,
                                                trans_b,
                                                m,
                                                n,
                                                k,
                                                alpha,
                                                A,
                                                ld_a,
                                                stride_a,
                                                B,
                                                ld_b,
                                                stride_b,
                                                beta,
                                                C,
                                                ld_c,
                                                stride_c,
                                                b_c);

        if(validArgs != rocblas_status_success)
            return validArgs;

        rocblas_gemm_kernel_name_template<false, T>(
            trans_a, trans_b, m, n, k, ld_a, stride_a, ld_b, stride_b, ld_c, stride_c, b_c);

        return validArgs;
    }

}

extern "C" {

/*******************************************************************************
 * Strided_Batched GEMM APIs
 ******************************************************************************/

rocblas_status rocblas_hgemm_strided_batched(rocblas_handle      handle,
                                             rocblas_operation   trans_a,
                                             rocblas_operation   trans_b,
                                             rocblas_int         m,
                                             rocblas_int         n,
                                             rocblas_int         k,
                                             const rocblas_half* alpha,
                                             const rocblas_half* A,
                                             rocblas_int         ld_a,
                                             rocblas_stride      stride_a,
                                             const rocblas_half* B,
                                             rocblas_int         ld_b,
                                             rocblas_stride      stride_b,
                                             const rocblas_half* beta,
                                             rocblas_half*       C,
                                             rocblas_int         ld_c,
                                             rocblas_stride      stride_c,
                                             rocblas_int         b_c)
{
#ifdef USE_TENSILE_HOST
    TensileHostCall<rocblas_half>           hostCall;
    RocblasContractionProblem<rocblas_half> problem(ContractionProblemType::GEMMStridedBatch,
                                                    trans_a,
                                                    trans_b,
                                                    m,
                                                    n,
                                                    k,
                                                    alpha,
                                                    A,
                                                    ld_a,
                                                    stride_a,
                                                    B,
                                                    ld_b,
                                                    stride_b,
                                                    beta,
                                                    C,
                                                    ld_c,
                                                    stride_c,
                                                    b_c);

    return callTensileContraction_half(&problem, handle->host);
#else

    return rocblas_gemm_strided_batched_impl<rocblas_half>(handle,
                                                           trans_a,
                                                           trans_b,
                                                           m,
                                                           n,
                                                           k,
                                                           alpha,
                                                           A,
                                                           ld_a,
                                                           stride_a,
                                                           B,
                                                           ld_b,
                                                           stride_b,
                                                           beta,
                                                           C,
                                                           ld_c,
                                                           stride_c,
                                                           b_c);
#endif
}

rocblas_status rocblas_sgemm_strided_batched(rocblas_handle    handle,
                                             rocblas_operation trans_a,
                                             rocblas_operation trans_b,
                                             rocblas_int       m,
                                             rocblas_int       n,
                                             rocblas_int       k,
                                             const float*      alpha,
                                             const float*      A,
                                             rocblas_int       ld_a,
                                             rocblas_stride    stride_a,
                                             const float*      B,
                                             rocblas_int       ld_b,
                                             rocblas_stride    stride_b,
                                             const float*      beta,
                                             float*            C,
                                             rocblas_int       ld_c,
                                             rocblas_stride    stride_c,
                                             rocblas_int       b_c)
{

#ifdef USE_TENSILE_HOST
    TensileHostCall<float>           hostCall;
    RocblasContractionProblem<float> problem(ContractionProblemType::GEMMStridedBatch,
                                             trans_a,
                                             trans_b,
                                             m,
                                             n,
                                             k,
                                             alpha,
                                             A,
                                             ld_a,
                                             stride_a,
                                             B,
                                             ld_b,
                                             stride_b,
                                             beta,
                                             C,
                                             ld_c,
                                             stride_c,
                                             b_c);

    return callTensileContraction_float(&problem, handle->host);
#else

    return rocblas_gemm_strided_batched_impl<float>(handle,
                                                    trans_a,
                                                    trans_b,
                                                    m,
                                                    n,
                                                    k,
                                                    alpha,
                                                    A,
                                                    ld_a,
                                                    stride_a,
                                                    B,
                                                    ld_b,
                                                    stride_b,
                                                    beta,
                                                    C,
                                                    ld_c,
                                                    stride_c,
                                                    b_c);

#endif
}

rocblas_status rocblas_dgemm_strided_batched(rocblas_handle    handle,
                                             rocblas_operation trans_a,
                                             rocblas_operation trans_b,
                                             rocblas_int       m,
                                             rocblas_int       n,
                                             rocblas_int       k,
                                             const double*     alpha,
                                             const double*     A,
                                             rocblas_int       ld_a,
                                             rocblas_stride    stride_a,
                                             const double*     B,
                                             rocblas_int       ld_b,
                                             rocblas_stride    stride_b,
                                             const double*     beta,
                                             double*           C,
                                             rocblas_int       ld_c,
                                             rocblas_stride    stride_c,
                                             rocblas_int       b_c)
{
#ifdef USE_TENSILE_HOST
    TensileHostCall<double>           hostCall;
    RocblasContractionProblem<double> problem(ContractionProblemType::GEMMStridedBatch,
                                              trans_a,
                                              trans_b,
                                              m,
                                              n,
                                              k,
                                              alpha,
                                              A,
                                              ld_a,
                                              stride_a,
                                              B,
                                              ld_b,
                                              stride_b,
                                              beta,
                                              C,
                                              ld_c,
                                              stride_c,
                                              b_c);

    return callTensileContraction_double(&problem, handle->host);
#else
    return rocblas_gemm_strided_batched_impl<double>(handle,
                                                     trans_a,
                                                     trans_b,
                                                     m,
                                                     n,
                                                     k,
                                                     alpha,
                                                     A,
                                                     ld_a,
                                                     stride_a,
                                                     B,
                                                     ld_b,
                                                     stride_b,
                                                     beta,
                                                     C,
                                                     ld_c,
                                                     stride_c,
                                                     b_c);
#endif
}

rocblas_status rocblas_cgemm_strided_batched(rocblas_handle               handle,
                                             rocblas_operation            trans_a,
                                             rocblas_operation            trans_b,
                                             rocblas_int                  m,
                                             rocblas_int                  n,
                                             rocblas_int                  k,
                                             const rocblas_float_complex* alpha,
                                             const rocblas_float_complex* A,
                                             rocblas_int                  ld_a,
                                             rocblas_stride               stride_a,
                                             const rocblas_float_complex* B,
                                             rocblas_int                  ld_b,
                                             rocblas_stride               stride_b,
                                             const rocblas_float_complex* beta,
                                             rocblas_float_complex*       C,
                                             rocblas_int                  ld_c,
                                             rocblas_stride               stride_c,
                                             rocblas_int                  b_c)
{
#ifdef USE_TENSILE_HOST
    TensileHostCall<rocblas_float_complex>           hostCall;
    RocblasContractionProblem<rocblas_float_complex> problem(
        ContractionProblemType::GEMMStridedBatch,
        trans_a,
        trans_b,
        m,
        n,
        k,
        alpha,
        A,
        ld_a,
        stride_a,
        B,
        ld_b,
        stride_b,
        beta,
        C,
        ld_c,
        stride_c,
        b_c);

    return callTensileContraction_float_complex(&problem, handle->host);
#else
    return rocblas_gemm_strided_batched_impl<rocblas_float_complex>(handle,
                                                                    trans_a,
                                                                    trans_b,
                                                                    m,
                                                                    n,
                                                                    k,
                                                                    alpha,
                                                                    A,
                                                                    ld_a,
                                                                    stride_a,
                                                                    B,
                                                                    ld_b,
                                                                    stride_b,
                                                                    beta,
                                                                    C,
                                                                    ld_c,
                                                                    stride_c,
                                                                    b_c);
#endif
}

rocblas_status rocblas_zgemm_strided_batched(rocblas_handle                handle,
                                             rocblas_operation             trans_a,
                                             rocblas_operation             trans_b,
                                             rocblas_int                   m,
                                             rocblas_int                   n,
                                             rocblas_int                   k,
                                             const rocblas_double_complex* alpha,
                                             const rocblas_double_complex* A,
                                             rocblas_int                   ld_a,
                                             rocblas_stride                stride_a,
                                             const rocblas_double_complex* B,
                                             rocblas_int                   ld_b,
                                             rocblas_stride                stride_b,
                                             const rocblas_double_complex* beta,
                                             rocblas_double_complex*       C,
                                             rocblas_int                   ld_c,
                                             rocblas_stride                stride_c,
                                             rocblas_int                   b_c)
{
#ifdef USE_TENSILE_HOST
    TensileHostCall<rocblas_double_complex>           hostCall;
    RocblasContractionProblem<rocblas_double_complex> problem(
        ContractionProblemType::GEMMStridedBatch,
        trans_a,
        trans_b,
        m,
        n,
        k,
        alpha,
        A,
        ld_a,
        stride_a,
        B,
        ld_b,
        stride_b,
        beta,
        C,
        ld_c,
        stride_c,
        b_c);

    return callTensileContraction_double_complex(&problem, handle->host);
#else

    return rocblas_gemm_strided_batched_impl<rocblas_double_complex>(handle,
                                                                     trans_a,
                                                                     trans_b,
                                                                     m,
                                                                     n,
                                                                     k,
                                                                     alpha,
                                                                     A,
                                                                     ld_a,
                                                                     stride_a,
                                                                     B,
                                                                     ld_b,
                                                                     stride_b,
                                                                     beta,
                                                                     C,
                                                                     ld_c,
                                                                     stride_c,
                                                                     b_c);
#endif
}

/*******************************************************************************
 * Strided Batched GEMM Kernel name APIs
 ******************************************************************************/
rocblas_status rocblas_hgemm_strided_batched_kernel_name(rocblas_handle      handle,
                                                         rocblas_operation   trans_a,
                                                         rocblas_operation   trans_b,
                                                         rocblas_int         m,
                                                         rocblas_int         n,
                                                         rocblas_int         k,
                                                         const rocblas_half* alpha,
                                                         const rocblas_half* A,
                                                         rocblas_int         ld_a,
                                                         rocblas_stride      stride_a,
                                                         const rocblas_half* B,
                                                         rocblas_int         ld_b,
                                                         rocblas_stride      stride_b,
                                                         const rocblas_half* beta,
                                                         rocblas_half*       C,
                                                         rocblas_int         ld_c,
                                                         rocblas_stride      stride_c,
                                                         rocblas_int         b_c)
{
    return rocblas_gemm_strided_batched_kernel_name_impl<rocblas_half>(handle,
                                                                       trans_a,
                                                                       trans_b,
                                                                       m,
                                                                       n,
                                                                       k,
                                                                       alpha,
                                                                       A,
                                                                       ld_a,
                                                                       stride_a,
                                                                       B,
                                                                       ld_b,
                                                                       stride_b,
                                                                       beta,
                                                                       C,
                                                                       ld_c,
                                                                       stride_c,
                                                                       b_c);
}

rocblas_status rocblas_sgemm_strided_batched_kernel_name(rocblas_handle    handle,
                                                         rocblas_operation trans_a,
                                                         rocblas_operation trans_b,
                                                         rocblas_int       m,
                                                         rocblas_int       n,
                                                         rocblas_int       k,
                                                         const float*      alpha,
                                                         const float*      A,
                                                         rocblas_int       ld_a,
                                                         rocblas_stride    stride_a,
                                                         const float*      B,
                                                         rocblas_int       ld_b,
                                                         rocblas_stride    stride_b,
                                                         const float*      beta,
                                                         float*            C,
                                                         rocblas_int       ld_c,
                                                         rocblas_stride    stride_c,
                                                         rocblas_int       b_c)
{
    return rocblas_gemm_strided_batched_kernel_name_impl<float>(handle,
                                                                trans_a,
                                                                trans_b,
                                                                m,
                                                                n,
                                                                k,
                                                                alpha,
                                                                A,
                                                                ld_a,
                                                                stride_a,
                                                                B,
                                                                ld_b,
                                                                stride_b,
                                                                beta,
                                                                C,
                                                                ld_c,
                                                                stride_c,
                                                                b_c);
}

rocblas_status rocblas_dgemm_strided_batched_kernel_name(rocblas_handle    handle,
                                                         rocblas_operation trans_a,
                                                         rocblas_operation trans_b,
                                                         rocblas_int       m,
                                                         rocblas_int       n,
                                                         rocblas_int       k,
                                                         const double*     alpha,
                                                         const double*     A,
                                                         rocblas_int       ld_a,
                                                         rocblas_stride    stride_a,
                                                         const double*     B,
                                                         rocblas_int       ld_b,
                                                         rocblas_stride    stride_b,
                                                         const double*     beta,
                                                         double*           C,
                                                         rocblas_int       ld_c,
                                                         rocblas_stride    stride_c,
                                                         rocblas_int       b_c)
{
    return rocblas_gemm_strided_batched_kernel_name_impl<double>(handle,
                                                                 trans_a,
                                                                 trans_b,
                                                                 m,
                                                                 n,
                                                                 k,
                                                                 alpha,
                                                                 A,
                                                                 ld_a,
                                                                 stride_a,
                                                                 B,
                                                                 ld_b,
                                                                 stride_b,
                                                                 beta,
                                                                 C,
                                                                 ld_c,
                                                                 stride_c,
                                                                 b_c);
}
}
