#include <Python.h>
#include "pyplug.h"

#include <dlfcn.h>
#include <glib.h>
#include <cutter.h>

#include "../src/config.h"

#include "test.h"

static int _repeats_no 		= REPEATS_NO;
static int _repeats_few 	= REPEATS_FEW;
static int _repeats_many 	= REPEATS_MANY;

static void 
__dummy_free (void *arg)
{ }

static void
_default_repeat_data (void)
{
	cut_add_data("one pass",
		     &_repeats_no,
		     __dummy_free,
		     "few passes",
		     &_repeats_few,
		     __dummy_free,
		     "many passes",
		     &_repeats_many,
		     __dummy_free,
		     NULL);
}

#define DEFAULT_REPEAT_DATA(NAME)	void \
					data_ ## NAME (void) { \
						_default_repeat_data (); \
					}

void cut_setup (void);
void cut_teardown (void);

void test_pyplug_initialized (void);
void test_register_callback (void);

typedef struct _LastDebugCallbackData {
	PyPlugDebugLevel	level;
	char			*message;
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
	pyplug_finalize_enviroment (FALSE);
	pyplug_finalize();
}


DEFAULT_REPEAT_DATA(pyplug_python_load_libpython)
void
test_pyplug_python_load_libpython (int* passes)
{
	int i = 0;
	while(i++ < *passes) {
		cut_assert_true(pyplug_is_initialized());
		cut_assert_false(pyplug_libpython_is_loaded());

		cut_assert_true(pyplug_libpython_load());
		cut_assert_true(pyplug_libpython_is_loaded());
		cut_assert_not_null(dlopen(PYTHON_SONAME, RTLD_NOLOAD));

		cut_assert_true(pyplug_libpython_unload());
		cut_assert_false(pyplug_libpython_is_loaded());
	}
}


DEFAULT_REPEAT_DATA(pyplug_python_load_python_api)
void
test_pyplug_python_load_python_api (int* passes)
{
	int i = 0;
	while(i++ < *passes) {
		cut_assert_false(Py_IsInitialized());
		pyplug_init_enviroment();
		cut_assert_true(Py_IsInitialized());
		pyplug_finalize_enviroment (FALSE);
		cut_assert_false(Py_IsInitialized());
	}
}

DEFAULT_REPEAT_DATA(pyplug_python_unload_keep_python_api)
void
test_pyplug_python_unload_keep_python_api (int* passes)
{
	int i = 0;
	while(i++ < *passes) {
		cut_assert_false(Py_IsInitialized());

		pyplug_init_enviroment();
		cut_assert_true(Py_IsInitialized());

		pyplug_finalize_enviroment (TRUE);
		cut_assert_true(Py_IsInitialized());

		pyplug_init_enviroment();
		cut_assert_true(Py_IsInitialized());

		pyplug_finalize_enviroment (FALSE);
		cut_assert_false(Py_IsInitialized());
	}
}

static void
__test_interpreter_load_and_unload (void)
{
	PyThreadState	*thr_global = NULL,
			*thr_interpreter = NULL,
			*thr_final = NULL;

	thr_global = PyThreadState_Get();
	pyplug_init_interpreter();
	thr_interpreter = PyThreadState_Get();
	cut_assert_not_equal_uint((unsigned int)thr_global, 
		(unsigned int)thr_interpreter);

	pyplug_free_interpreter();
	thr_final = PyThreadState_Get();
	cut_assert_equal_pointer(thr_global, thr_final);
}


DEFAULT_REPEAT_DATA(pyplug_python_load_interpreter)
void 
test_pyplug_python_load_interpreter (int* passes)
{
	int i = 0;

	while(i++ < *passes) {
		cut_assert_false(Py_IsInitialized());
		pyplug_init_enviroment();
		cut_assert_true(Py_IsInitialized());

		__test_interpreter_load_and_unload();

		pyplug_finalize_enviroment (FALSE);
		cut_assert_false(Py_IsInitialized());
	}
} 


DEFAULT_REPEAT_DATA(pyplug_python_load_interpreter_internal)
void 
test_pyplug_python_load_interpreter_internal (int* passes)
{
	int i = 0;

	cut_assert_false(Py_IsInitialized());
	pyplug_init_enviroment();
	cut_assert_true(Py_IsInitialized());

	while(i++ < *passes)
		__test_interpreter_load_and_unload();

	pyplug_finalize_enviroment (FALSE);
	cut_assert_false(Py_IsInitialized());
}

static void
__test_get_syspath (void)
{
	int 	n = 0;
	char	**syspath;

	syspath = pyplug_get_current_syspath();

	cut_assert_not_null(syspath);

	while(syspath[n] != NULL) {
		cut_assert_not_null(syspath[n]);
		cut_assert_operator_int(strlen(syspath[n]), >, 0);
		n++;
	}
	cut_assert_not_equal_int(0, n);

	g_strfreev(syspath);
	syspath = NULL;
}

DEFAULT_REPEAT_DATA(pyplug_python_get_syspath)
void
test_pyplug_python_get_syspath (int *passes)
{
	int 	i = 0;

	while(i++ <= *passes) {

		cut_assert_false(Py_IsInitialized());
		pyplug_init_enviroment();
		cut_assert_true(Py_IsInitialized());
		pyplug_init_interpreter();

		__test_get_syspath();

		pyplug_free_interpreter();
		pyplug_finalize_enviroment(FALSE);
		cut_assert_false(Py_IsInitialized());
	}
}


DEFAULT_REPEAT_DATA(pyplug_python_get_syspath_internal)
void
test_pyplug_python_get_syspath_internal (int *passes)
{
	int 	i = 0;

	cut_assert_false(Py_IsInitialized());
	pyplug_init_enviroment();
	pyplug_libpython_load();
	cut_assert_true(Py_IsInitialized());
	pyplug_init_interpreter();

	while(i++ <= *passes)
		__test_get_syspath();

	pyplug_free_interpreter();
	pyplug_libpython_unload();
	pyplug_finalize_enviroment(FALSE);
	cut_assert_false(Py_IsInitialized());
}
