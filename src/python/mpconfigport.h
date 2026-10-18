#include <port/mpconfigport_common.h>
#define MICROPY_CONFIG_ROM_LEVEL (MICROPY_CONFIG_ROM_LEVEL_CORE_FEATURES)
#define MICROPY_ENABLE_COMPILER (1)
#define MICROPY_ENABLE_GC (1)
#define MICROPY_PY_GC (1)
#define MICROPY_NLR_SETJMP (1)
#define MICROPY_GCREGS_SETJMP (1)
#define MICROPY_FLOAT_IMPL (MICROPY_FLOAT_IMPL_DOUBLE)
#define MICROPY_LONGINT_IMPL (MICROPY_LONGINT_IMPL_MPZ)
#define MICROPY_PY_SYS (1)
#define MICROPY_PY_SYS_PLATFORM "thinkcad"
#define MICROPY_PY_SYS_STDIO (0)
#define MICROPY_PY_BUILTINS_INPUT (0)
#define MICROPY_PY_IO (0)
#define MICROPY_PY_STRUCT (1)
#define MICROPY_ERROR_REPORTING (MICROPY_ERROR_REPORTING_DETAILED)

#define MICROPY_STACK_CHECK (1)
#define MICROPY_PORT_CONSTANTS { MP_ROM_QSTR(MP_QSTR___thinkcad__), MP_ROM_TRUE },
// Windows targets are little-endian; MSVC has no <endian.h>.
#ifdef _WIN32
#define MP_ENDIANNESS_LITTLE (1)
#endif
// Compatibility definitions follow the upstream Windows port (MIT license).
#ifdef _MSC_VER
#define MP_NORETURN __declspec(noreturn)
#define MP_WEAK
#define MP_NOINLINE __declspec(noinline)
#define MP_ALWAYSINLINE __forceinline
#define MP_LIKELY(x) (x)
#define MP_UNLIKELY(x) (x)
#define MP_SSIZE_MAX INTPTR_MAX
#ifndef _SSIZE_T_DEFINED
typedef intptr_t ssize_t;
#define _SSIZE_T_DEFINED
#endif
#endif
