/**
 * pyplug.h - general utilities for loading python enviroment
 *
 * @author Maciej Makowski
 *
 * @date 16.03.2011
 */
#ifndef __PYPLUG_H__
#define __PYPLUG_H__

#include <stdarg.h>
#include <glib.h>

typedef enum _PyPlugDebugLevel {
	PYPLUG_LEVEL_ALL = 0,
	PYPLUG_LEVEL_MISC,
	PYPLUG_LEVEL_INFO,
	PYPLUG_LEVEL_WARNING,
	PYPLUG_LEVEL_ERROR,
	PYPLUG_LEVEL_FAILURE
} PyPlugDebugLevel;

/**
 * Type for debugging callbacks.
 */
typedef void (*PyPlugDebugFnc)(PyPlugDebugLevel level, const char* message);

/**
 * Checks if pyplug enviroment have been already initialized.
 */
gboolean
pyplug_is_initialized (void);

/**
 * Initializes pyplug enviroment.
 */
void
pyplug_initialize (void);

/**
 * Finalizes pyplug enviroment. No pyplug calls should be made unless
 * enviroment has been reinitialized.
 */
void
pyplug_finalize (void);

/**
 * Registers debug callback for pyplug, so it can output debugging
 * messages.
 */
void
pyplug_reg_dbgcb (PyPlugDebugFnc callback);

/**
 * Unregisters debug callback for pyplug.
 */
void
pyplug_unreg_dbgcb (PyPlugDebugFnc callback);

/**
 * Loads libpython shared object to the process global scope. This is
 * required since some Python/C packages are linked without reference
 * to libpython (I think it was about ctypes) and they segfault on
 * import.
 */
gboolean
pyplug_libpython_load (void);

/**
 * Unloads libpython shared object from process global scope. This
 * should be only used when the enviroment is unloaded.
 */
gboolean
pyplug_libpython_unload (void);

/**
 * Checks if python enviroment has been initialized.
 */
gboolean
pyplug_libpython_is_loaded (void);

/**
 * Initializes python enviroment.
 */
gboolean
pyplug_init_enviroment (void);

/**
 * Finalizes python enviroment. This should remove all the memory allocated
 * by python, so no python object should be accessed after this call.
 */
void
pyplug_finalize_enviroment (gboolean keep_python);

/**
 * Initializes python interpreter thread.
 */
gboolean
pyplug_init_interpreter(void);

/**
 * Deletes python interpreter thread. Most of the memory alocated in thread
 * should be released, so no python object operating in pyplug context
 * should be referenced after this call.
 */
void
pyplug_free_interpreter(void);

/**
 * Returns NULL terminated array of strings, being elements of python
 * sys.path.
 */
gchar**
pyplug_syspath_get(void);

/**
 * Sets syspath from NULL terminated array of strings.
 *
 * @replace - determines if elements should be apended to syspath or should
 * it be replaced.
 */
gboolean
pyplug_syspath_set_stringv(const char **paths, gboolean replace);

/**
 * Sets syspath from string.
 *
 * @replace - determines if elements should be apended to syspath or should
 * it be replaced.
 */
gboolean
pyplug_syspath_set_string(const char *paths, gboolean replace);

/**
 * Imports python module with given name to the interpreter thread.
 */
gboolean
pyplug_import_module(const gchar* module);

#endif // __PYPLUG_H__
