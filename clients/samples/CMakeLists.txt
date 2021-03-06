# ########################################################################
# Copyright 2016-2020 Advanced Micro Devices, Inc.
# ########################################################################

set( rocblas_samples_common ../common/utility.cpp )

set( THREADS_PREFER_PTHREAD_FLAG ON )
find_package( Threads REQUIRED )

# C example
add_executable( example-c-dgeam example_c_dgeam.c )
target_compile_definitions( example-c-dgeam PRIVATE __HIP_PLATFORM_HCC__ )
target_compile_options( example-c-dgeam PRIVATE -std=c11 )

# C++ examples
add_executable( example-sscal example_sscal.cpp ${rocblas_samples_common} )
add_executable( example-scal-template example_scal_template.cpp ${rocblas_samples_common} )
add_executable( example-solver-rocblas example_solver_rocblas.cpp ${rocblas_samples_common} )

# Fortran examples
add_executable( example-fortran-axpy example_fortran_axpy.f90 $<TARGET_OBJECTS:rocblas_fortran>)
add_executable( example-fortran-scal example_fortran_scal.f90 $<TARGET_OBJECTS:rocblas_fortran>)
add_executable( example-fortran-gemv example_fortran_gemv.f90 $<TARGET_OBJECTS:rocblas_fortran>)

if( BUILD_WITH_TENSILE )
  add_executable( example-sgemm example_sgemm.cpp ${rocblas_samples_common} )
  add_executable( example-sgemm-strided-batched example_sgemm_strided_batched.cpp ${rocblas_samples_common} )
  add_executable( example-gemm-ext2 example_gemm_ext2.cpp ${rocblas_samples_common} )
  set( sample_list_tensile example-sgemm example-sgemm-strided-batched example-gemm-ext2 )
endif( )

set( sample_list_fortran example-fortran-axpy example-fortran-scal example-fortran-gemv )
set( sample_list_c example-c-dgeam )
set( sample_list_base example-sscal example-scal-template example-solver-rocblas )

set( sample_list_all ${sample_list_base} ${sample_list_tensile} ${sample_list_fortran} ${sample_list_c} )
set( sample_list_hip_device ${sample_list_base} ${sample_list_tensile} )

foreach( exe ${sample_list_all} )
  target_link_libraries( ${exe} PRIVATE rocblas_fortran_client roc::rocblas )

  set_target_properties( ${exe} PROPERTIES CXX_EXTENSIONS NO )
  set_target_properties( ${exe} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging" )

  if (NOT ${exe} STREQUAL "example-c-dgeam")
    target_compile_definitions( ${exe} PRIVATE ROCM_USE_FLOAT16 )
  endif()

  target_include_directories( ${exe}
    PRIVATE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../library/include>
      )

  target_include_directories( ${exe}
    SYSTEM PRIVATE
      $<BUILD_INTERFACE:${HIP_INCLUDE_DIRS}>
      $<BUILD_INTERFACE:${HCC_INCLUDE_DIRS}>
      )

  if( CUDA_FOUND )
    target_include_directories( ${exe}
      PRIVATE
        $<BUILD_INTERFACE:${CUDA_INCLUDE_DIRS}>
      )
    target_compile_definitions( ${exe} PRIVATE __HIP_PLATFORM_NVCC__ )
    target_link_libraries( ${exe} PRIVATE ${CUDA_LIBRARIES} )
  else( )
    # auto set in hip_common.h
    #target_compile_definitions( ${exe} PRIVATE __HIP_PLATFORM_HCC__ )
  endif( )

  if( CMAKE_CXX_COMPILER MATCHES ".*/hcc$" )
    # Remove following when hcc is fixed; hcc emits following spurious warning ROCm v1.6.1
    # "clang-5.0: warning: argument unused during compilation: '-isystem ${ROCM_PATH}/include'"
    # include order workaround to force /opt/rocm/include later in order to ignore installed rocblas
    set(CMAKE_CXX_FLAGS "-isystem /opt/rocm/include ${CMAKE_CXX_FLAGS}")
    target_compile_options( ${exe} PRIVATE -Wno-unused-command-line-argument )
    target_include_directories( ${exe} PRIVATE ${ROCM_PATH}/hsa/include)
  elseif( CMAKE_COMPILER_IS_GNUCXX OR CXX_VERSION_STRING MATCHES "clang" )
    # GCC or hip-clang needs specific flags to turn on f16c intrinsics
    target_compile_options( ${exe} PRIVATE -mf16c )
  endif( )
endforeach( )

foreach( exe ${sample_list_hip_device} )
  if( NOT CUDA_FOUND )
    target_link_libraries( ${exe} PRIVATE hip::device )
  endif()
endforeach( )

if( CMAKE_CXX_COMPILER MATCHES ".*/hcc$" )
  # include order workaround to force /opt/rocm/include later in order to ignore installed rocblas
  set(CMAKE_CXX_FLAGS "-isystem /opt/rocm/include ${CMAKE_CXX_FLAGS}")
endif()
