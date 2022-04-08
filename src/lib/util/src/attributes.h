#pragma once
/** Indicates to the compiler that the function can not ever return, so that any stack restoring or
				 *  return code may be omitted by the compiler in the resulting binary.
				 */
#define TATTR_NO_RETURN __attribute__((noreturn))

/** Indicates that the function returns a value which should not be ignored by the user code. When
				 *  applied, any ignored return value from calling the function will produce a compiler warning.
				 */
#define TATTR_WARN_UNUSED_RESULT __attribute__((warn_unused_result))

/** Indicates that the specified parameters of the function are pointers which should never be \c NULL.
				 *  When applied as a 1-based comma separated list the compiler will emit a warning if the specified
				 *  parameters are known at compiler time to be \c NULL at the point of calling the function.
				 */
#define TATTR_NON_NULL_PTR_ARG(...) __attribute__((nonnull(__VA_ARGS__)))

/** Removes any preamble or postamble from the function. When used, the function will not have any
				 *  register or stack saving code. This should be used with caution, and when used the programmer
				 *  is responsible for maintaining stack and register integrity.
				 */
#define TATTR_NAKED __attribute__((naked))

/** Prevents the compiler from considering a specified function for in-lining. When applied, the given
				 *  function will not be in-lined under any circumstances.
				 */
#define TATTR_NO_INLINE __attribute__((noinline))

/** Forces the compiler to inline the specified function. When applied, the given function will be
				 *  in-lined under all circumstances.
				 */
#define TATTR_ALWAYS_INLINE __attribute__((always_inline))

/** Indicates that the specified function is pure, in that it has no side-effects other than global
				 *  or parameter variable access.
				 */
#define TATTR_PURE __attribute__((pure))

/** Indicates that the specified function is constant, in that it has no side effects other than
				 *  parameter access.
				 */
#define TATTR_CONST __attribute__((const))

/** Marks a given function as deprecated, which produces a warning if the function is called. */
#define TATTR_DEPRECATED __attribute__((deprecated))

/** Marks a function as a weak reference, which can be overridden by other functions with an
				 *  identical name (in which case the weak reference is discarded at link time).
				 */
#define TATTR_WEAK __attribute__((weak))

/** Forces the compiler to not automatically zero the given global variable on startup, so that the
			 *  current RAM contents is retained. Under most conditions this value will be random due to the
			 *  behavior of volatile memory once power is removed, but may be used in some specific circumstances,
			 *  like the passing of values back after a system watchdog reset.
			 */
#define TATTR_NO_INIT __attribute__((section(".noinit")))

/** Places the function in one of the initialization sections, which execute before the main function
			 *  of the application. Refer to the avr-libc manual for more information on the initialization sections.
			 *
			 *  \param[in] SectionIndex  Initialization section number where the function should be placed.
			 */
#define TATTR_INIT_SECTION(SectionIndex) __attribute__((used, naked, section(".init" #SectionIndex)))

/** Marks a function as an alias for another function.
			 *
			 *  \param[in] Func  Name of the function which the given function name should alias.
			 */
#define TATTR_ALIAS(Func) __attribute__((alias(#Func)))

/** Marks a variable or struct element for packing into the smallest space available, omitting any
			 *  alignment bytes usually added between fields to optimize field accesses.
			 */
#define TATTR_PACKED __attribute__((packed))

/** Indicates the minimum alignment in bytes for a variable or struct element.
			 *
			 *  \param[in] Bytes  Minimum number of bytes the item should be aligned to.
			 */
#define TATTR_ALIGNED(Bytes) __attribute__((aligned(Bytes)))
