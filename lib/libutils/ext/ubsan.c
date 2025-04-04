// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2016, Linaro Limited
 */

#include <atomic.h>
#include <compiler.h>
#include <string.h>
#include <trace.h>
#include <types_ext.h>
#include <util.h>

#if defined(__KERNEL__)
# include <kernel/panic.h>
#elif defined(__LDELF__)
# include <ldelf_syscalls.h>
#else
# include <utee_syscalls.h>
#endif

#define UBSAN_LOC_REPORTED BIT32(31)

struct source_location {
	const char *file_name;
	uint32_t line;
	uint32_t column;
};

static void __noreturn ubsan_panic(void)
{
#if defined(__KERNEL__)
	panic();
#elif defined(__LDELF__)
	_ldelf_panic(2);
#else
	_utee_panic(TEE_ERROR_GENERIC);
#endif
	/*
	 * _ldelf_panic and _utee_panic are not marked as noreturn,
	 * however they should be. To prevent "‘noreturn’ function
	 * does return" warning the while loop is used.
	 */
	while (1)
		;
}

static bool was_already_reported(struct source_location *loc)
{
	uint32_t column = loc->column;

	if (column & UBSAN_LOC_REPORTED)
		return true;

	return !atomic_cas_u32(&loc->column, &column,
			       column | UBSAN_LOC_REPORTED);
}

struct type_descriptor {
	uint16_t type_kind;
	uint16_t type_info;
	char type_name[1];
};

struct type_mismatch_data {
	struct source_location loc;
	struct type_descriptor *type;
	unsigned long alignment;
	unsigned char type_check_kind;
};

struct overflow_data {
	struct source_location loc;
	struct type_descriptor *type;
};

struct shift_out_of_bounds_data {
	struct source_location loc;
	struct type_descriptor *lhs_type;
	struct type_descriptor *rhs_type;
};

struct out_of_bounds_data {
	struct source_location loc;
	struct type_descriptor *array_type;
	struct type_descriptor *index_type;
};

struct unreachable_data {
	struct source_location loc;
};

struct vla_bound_data {
	struct source_location loc;
	struct type_descriptor *type;
};

struct invalid_value_data {
	struct source_location loc;
	struct type_descriptor *type;
};

struct nonnull_arg_data {
	struct source_location loc;
};

struct invalid_builtin_data {
	struct source_location loc;
	unsigned char kind;
};

/*
 * When compiling with -fsanitize=undefined the compiler expects functions
 * with the following signatures. The functions are never called directly,
 * only when undefined behavior is detected in instrumented code.
 */
void __ubsan_handle_type_mismatch(struct type_mismatch_data *data,
				  unsigned long ptr);
void __ubsan_handle_type_mismatch_v1(void *data_, void *ptr);
void __ubsan_handle_add_overflow(void *data_, void *lhs, void *rhs);
void __ubsan_handle_sub_overflow(void *data_, void *lhs, void *rhs);
void __ubsan_handle_mul_overflow(void *data_, void *lhs, void *rhs);
void __ubsan_handle_negate_overflow(void *data_, void *old_val);
void __ubsan_handle_divrem_overflow(void *data_, void *lhs, void *rhs);
void __ubsan_handle_pointer_overflow(void *data_, void *lhs, void *rhs);
void __ubsan_handle_shift_out_of_bounds(void *data_, void *lhs, void *rhs);
void __ubsan_handle_out_of_bounds(void *data_, void *idx);
void __ubsan_handle_builtin_unreachable(void *data_);
void __ubsan_handle_missing_return(void *data_);
void __ubsan_handle_vla_bound_not_positive(void *data_, void *bound);
void __ubsan_handle_load_invalid_value(void *data_, void *val);
void __ubsan_handle_nonnull_arg(void *data_
#if __GCC_VERSION < 60000
				, size_t arg_no
#endif
			       );
void __ubsan_handle_invalid_builtin(void *data_);

static bool should_panic = true;

static void ubsan_handle_error(const char *func, struct source_location *loc,
			       bool panic_flag)
{
	const char *f = func;
	const char func_prefix[] = "__ubsan_handle";

	if (was_already_reported(loc))
		return;

	if (!memcmp(f, func_prefix, sizeof(func_prefix) - 1))
		f += sizeof(func_prefix);

	EMSG_RAW("Undefined behavior %s at %s:%" PRIu32 " col %" PRIu32,
		 f, loc->file_name, loc->line, loc->column & ~UBSAN_LOC_REPORTED);

	if (panic_flag)
		ubsan_panic();
}

void __ubsan_handle_type_mismatch(struct type_mismatch_data *data,
				  unsigned long ptr __unused)
{
	ubsan_handle_error(__func__, &data->loc, should_panic);
}

void __ubsan_handle_type_mismatch_v1(void *data_, void *ptr __unused)
{
	struct type_mismatch_data *data = data_;

	ubsan_handle_error(__func__, &data->loc, should_panic);
}

void __ubsan_handle_add_overflow(void *data_, void *lhs __unused,
				 void *rhs __unused)
{
	struct overflow_data *data = data_;

	ubsan_handle_error(__func__, &data->loc, should_panic);
}

void __ubsan_handle_sub_overflow(void *data_, void *lhs __unused,
				 void *rhs __unused)
{
	struct overflow_data *data = data_;

	ubsan_handle_error(__func__, &data->loc, should_panic);
}

void __ubsan_handle_mul_overflow(void *data_, void *lhs __unused,
				 void *rhs __unused)
{
	struct overflow_data *data = data_;

	ubsan_handle_error(__func__, &data->loc, should_panic);
}

void __ubsan_handle_negate_overflow(void *data_, void *old_val __unused)
{
	struct overflow_data *data = data_;

	ubsan_handle_error(__func__, &data->loc, should_panic);
}

void __ubsan_handle_divrem_overflow(void *data_, void *lhs __unused,
				    void *rhs __unused)
{
	struct overflow_data *data = data_;

	ubsan_handle_error(__func__, &data->loc, should_panic);
}

void __ubsan_handle_pointer_overflow(void *data_, void *lhs __unused,
				     void *rhs __unused)
{
	struct overflow_data *data = data_;

	ubsan_handle_error(__func__, &data->loc, should_panic);
}

void __ubsan_handle_shift_out_of_bounds(void *data_, void *lhs __unused,
					void *rhs __unused)
{
	struct shift_out_of_bounds_data *data = data_;

	ubsan_handle_error(__func__, &data->loc, should_panic);
}

void __ubsan_handle_out_of_bounds(void *data_, void *idx __unused)
{
	struct out_of_bounds_data *data = data_;

	ubsan_handle_error(__func__, &data->loc, should_panic);
}

void __noreturn __ubsan_handle_builtin_unreachable(void *data_)
{
	struct unreachable_data *data = data_;

	ubsan_handle_error(__func__, &data->loc, false);
	ubsan_panic();
}

void __noreturn __ubsan_handle_missing_return(void *data_)
{
	struct unreachable_data *data = data_;

	ubsan_handle_error(__func__, &data->loc, false);
	ubsan_panic();
}

void __ubsan_handle_vla_bound_not_positive(void *data_, void *bound __unused)
{
	struct vla_bound_data *data = data_;

	ubsan_handle_error(__func__, &data->loc, should_panic);
}

void __ubsan_handle_load_invalid_value(void *data_, void *val __unused)
{
	struct invalid_value_data *data = data_;

	ubsan_handle_error(__func__, &data->loc, should_panic);
}

void __ubsan_handle_nonnull_arg(void *data_
#if __GCC_VERSION < 60000
				, size_t arg_no __unused
#endif
			       )
{
	struct nonnull_arg_data *data = data_;

	ubsan_handle_error(__func__, &data->loc, should_panic);
}

void __ubsan_handle_invalid_builtin(void *data_)
{
	struct invalid_builtin_data *data = data_;

	ubsan_handle_error(__func__, &data->loc, should_panic);
}
