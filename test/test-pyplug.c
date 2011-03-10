#include <pyplug.h>
#include <glib.h>
#include <cutter.h>

#include "test.h"

void cut_setup (void);
void cut_teardown (void);

void test_pyplug_initialized (void);
void test_register_callback (void);

typedef struct _LastDebugCallbackData {
	PyPlugDebugLevel	level;
	const char		*message;
} LastDebugCallbackData;
static LastDebugCallbackData _last_dbg_data = { -1, NULL };

void
cut_setup (void)
{
	pyplug_initialize();
}

void
cut_teardown (void)
{
	pyplug_finalize();
	_last_dbg_data.level = -1;
}

void
test_pyplug_initialized (void)
{
	cut_assert_true(pyplug_is_initialized());
}

static void
_test_debug_call(PyPlugDebugLevel level, const char *message) {
	_last_dbg_data.level = level;
	_last_dbg_data.message = cut_take_strdup(message);
}

#define _TEST_STRING_000	"Test 000"
#define _TEST_STRING_001	"Test 001"

void 
test_pyplug_debug_callback (void)
{
	cut_assert_true(pyplug_is_initialized());

	pyplug_reg_dbgcb(_test_debug_call);
	pyplug_debug(PYPLUG_LEVEL_INFO, _TEST_STRING_000);

	cut_assert_equal_uint(PYPLUG_LEVEL_INFO, _last_dbg_data.level);
	cut_assert_equal_string(_TEST_STRING_000, _last_dbg_data.message);

	pyplug_unreg_dbgcb(_test_debug_call);
	pyplug_debug(PYPLUG_LEVEL_FAILURE, _TEST_STRING_001);

	cut_assert_not_equal_uint(PYPLUG_LEVEL_FAILURE, _last_dbg_data.level);
	cut_assert_not_equal_string(_TEST_STRING_001, _last_dbg_data.message);
}

static void
test_pyplug_debug_level(PyPlugDebugLevel level)
{
	cut_assert_true(pyplug_is_initialized());

	pyplug_reg_dbgcb(_test_debug_call);
	pyplug_debug(level, _TEST_STRING_001);

	cut_assert_equal_uint(level, _last_dbg_data.level);
	cut_assert_equal_string(_TEST_STRING_001, _last_dbg_data.message);

}

void
test_pyplug_debug_all (void)
{
	test_pyplug_debug_level(PYPLUG_LEVEL_ALL);
}

void
test_pyplug_debug_misc (void)
{
	test_pyplug_debug_level(PYPLUG_LEVEL_MISC);
}

void
test_pyplug_debug_info (void)
{
	test_pyplug_debug_level(PYPLUG_LEVEL_INFO);
}

void
test_pyplug_debug_warning (void)
{
	test_pyplug_debug_level(PYPLUG_LEVEL_WARNING);
}

void
test_pyplug_debug_error (void)
{
	test_pyplug_debug_level(PYPLUG_LEVEL_ERROR);
}

void
test_pyplug_debug_failure (void)
{
	test_pyplug_debug_level(PYPLUG_LEVEL_FAILURE);
}

void
test_pyplug_debug_format (void)
{
	const char 	*format = "%d %x %.2f\n";
	int 		i = 10;
	float		f = 0.2;

	const char	*pattern;
	pattern = 	cut_take_printf(format, i, i, f);

	pyplug_reg_dbgcb(_test_debug_call);
	pyplug_debug(PYPLUG_LEVEL_INFO, format, i, i, f);
	cut_assert_equal_uint(PYPLUG_LEVEL_INFO, _last_dbg_data.level);
	cut_assert_equal_string(pattern, _last_dbg_data.message);
	pyplug_unreg_dbgcb(_test_debug_call);
}

