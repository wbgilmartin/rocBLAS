

#include <chrono>

#undef TRACEPOINT_PROVIDER
#define TRACEPOINT_PROVIDER rocblas_tracing

#undef TRACEPOINT_INCLUDE
#define TRACEPOINT_INCLUDE "./rocBLAS-tp.hpp"

#if !defined(_ROCBLAS_TP_H) || defined(TRACEPOINT_HEADER_MULTI_READ)
#define _ROCBLAS_TP_H

#include <lttng/tracepoint.h>

TRACEPOINT_EVENT(rocblas_tracing,
                 trace_time,
                 TP_ARGS(double, trace_duration_arg, char*, trace_function_arg),
                 TP_FIELDS(ctf_string(trace_function, trace_function_arg)
                               ctf_float(double, trace_duration, trace_duration_arg)))

#endif /* _ROCBLAS_TP_H */

#include <lttng/tracepoint-event.h>
