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

typedef void (*PyPlugDebugFnc)(PyPlugDebugLevel level, const char* message);


gboolean
pyplug_is_initialized (void);

void
pyplug_initialize (void);

void
pyplug_finalize (void);

void
pyplug_reg_dbgcb (PyPlugDebugFnc callback);

void
pyplug_unreg_dbgcb (PyPlugDebugFnc callback);

void
pyplug_debug (PyPlugDebugLevel level, const char* format, ...);

gboolean
pyplug_libpython_load (void);

gboolean
pyplug_libpython_unload (void);

gboolean
pyplug_libpython_is_loaded (void);

gboolean
pyplug_init_enviroment (void);

void
pyplug_finalize_enviroment (gboolean keep_python);

gboolean
pyplug_init_interpreter(void);

void
pyplug_free_interpreter(void);

gchar**
pyplug_get_current_syspath(void);

#endif // __PYPLUG_H__
