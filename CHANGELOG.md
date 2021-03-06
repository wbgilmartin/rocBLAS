# Change Log for rocBLAS
 
Full documentation for rocBLAS is available at [rocblas.readthedocs.io](https://rocblas.readthedocs.io/en/latest/).
 
## [(Unreleased) rocBLAS 2.34.0 for ROCm 4.0.0]
### Added
__anchor__ac
- Add changelog

__anchor__t

__anchor__w

__anchor__d

__anchor__n

__anchor__l

__anchor__al

 
 
## [(Unreleased) rocBLAS 2.32.0 for ROCm 3.10.0]
### Added
__anchor__ac
- Improved performance of gemm_batched for NN, general m, n, k, small m, n, k

__anchor__t

__anchor__w

__anchor__d

__anchor__n

__anchor__l

__anchor__al

 
 
## [rocBLAS 2.30.0 for ROCm 3.9.0]
### Added
- Slight improvements to FP16 Megatron BERT performance on MI50
- Improvements to FP16 Transformer performance on MI50
- Slight improvements to FP32 Transformer performance on MI50
- Improvements to FP32 DLRM Terabyte performance on gfx908
 
__anchor__ac

__anchor__t

__anchor__w

__anchor__d

__anchor__n

__anchor__l

__anchor__al

 
## [rocBLAS 2.28.0 for ROCm 3.8.0]
### Added
- added two functions: 
  - rocblas_status rocblas_set_atomics_mode(rocblas_atomics_mode mode);
  - rocblas_status rocblas_get_atomics_mode(rocblas_atomics_mode mode);
- added enum rocblas_atomics_mode. It can have two values
  - rocblas_atomics_allowed
  - rocblas_atomics_not_allowed
  The default is rocblas_atomics_not_allowed
- function rocblas_Xdgmm algorithm corrected and incx=0 support added
- dependencies:
  - rocblas-tensile internal component requires msgpack instead of LLVM
- Moved the following files from /opt/rocm/include to /opt/rocm/include/internal:
  - rocblas-auxillary.h
  - rocblas-complex-types.h
  - rocblas-functions.h
  - rocblas-types.h
  - rocblas-version.h
  - rocblas_bfloat16.h
  These files should NOT be included directly as this may lead to errors. Instead, /opt/rocm/include/rocblas.h should be included directly. /opt/rocm/include/rocblas_module.f90 can also be direcly used.

## [rocBLAS 2.26.0 for ROCm 3.7.0]
### Added
- Improvements to rocblas_Xgemm_batched performance for small m, n, k.
- Improvements to rocblas_Xgemv_batched  and rocblas_Xgemv_strided_batched performance for small m (QMCPACK use).
- Improvements to rocblas_Xdot (batched and non-batched) performance when both incx and incy are 1
- Improvements to FP32 ONNX BERT performance for MI50
- Significant improvements to FP32 Resnext, Inception Convolution performance for gfx908
- Slight improvements to FP32 DLRM Terabyte performance for gfx908
- Significant improvements to FP32 BDAS performance for gfx908
- Significant improvements to FP32 BDAS performance for MI50 and MI60
- Added substitution method for small trsm sizes with m <= 64 && n <= 64. Increases performance drastically for small batched trsm.

## [rocBLAS 2.24.0 for ROCm 3.6.0]
### Added
- Improvements to User Guide and Design Document
- L1 dot function optimized to utilize shuffle instructions (  improvements on bf16, f16, f32 data types )
- L1 dot function added x dot x optimized kernel
- Standardization of L1 rocblas-bench to use device pointer mode to focus on GPU memory bandwidth
- Adjustments for hipcc (hip-clang) compiler as standard build compiler and Centos8 support
- Added Fortran interface for all rocBLAS functions

## [rocBLAS 2.22.0 for ROCm 3.5.0]
### Added
- add geam complex, geam_batched, and geam_strided_batched
- add dgmm, dgmm_batched, and dgmm_strided_batched
- Optimized performance
  - ger
    - rocblas_sger, rocblas_dger,
    - rocblas_sger_batched, rocblas_dger_batched
    - rocblas_sger_strided_batched, rocblas_dger_strided_batched
  - geru 
    - rocblas_cgeru, rocblas_zgeru
    - rocblas_cgeru_batched, rocblas_zgeru_batched
    - rocblas_cgeru_strided_batched, rocblas_zgeru_strided_batched
  - gerc 
    - rocblas_cgerc, rocblas_zgerc
    - rocblas_cgerc_batched, rocblas_zgerc_batched
    - rocblas_cgerc_strided_batched, rocblas_zgerc_strided_batched
  - symv
    - rocblas_ssymv, rocblas_dsymv, rocblas_csymv, rocblas_zsymv,
    - rocblas_ssymv_batched, rocblas_dsymv_batched, rocblas_csymv_batched, rocblas_zsymv_batched,
    - rocblas_ssymv_strided_batched, rocblas_dsymv_strided_batched, rocblas_csymv_strided_batched, rocblas_zsymv_strided_batched,
  - sbmv 
    - rocblas_ssbmv, rocblas_dsbmv,
    - rocblas_ssbmv_batched, rocblas_dsbmv_batched,
    - rocblas_ssbmv_strided_batched, rocblas_dsbmv_strided_batched,
  - spmv 
    - rocblas_sspmv, rocblas_dspmv,
    - rocblas_sspmv_batched, rocblas_dspmv_batched,
    - rocblas_sspmv_strided_batched, rocblas_dspmv_strided_batched,
- improved documentation
- Fix argument checking in functions to match legacy BLAS
- Fixed conjugate-transpose version of geam

### Known Issues
- Compilation for GPU Targets:
When using the install.sh script for "all" GPU Targets, which is the default, you must first set an environment variable HCC_AMDGPU_TARGET listing the GPU targets, e.g.  HCC_AMDGPU_TARGET=gfx803,gfx900,gfx906,gfx908
If building for a specific architecture(s) using the  -a | --architecture flag, you should also set the environment variable HCC_AMDGPU_TARGET to match.  
Mismatching the environment variable to the -a flag architectures creates builds that may result in SEGFAULTS when running on GPUs which weren't specified.
