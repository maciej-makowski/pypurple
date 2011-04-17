#ifndef __PYPRPL_UTIL_H
#define __PYPRPL_UTIL_H

#include <Python.h>
#include "purple.h"

#include "config.h"
#include "pyplug.h"

gchar *
pyprpl_get_module_name(const char* filename);

/**
 * Utility function that gets the latest python error from the interpreter
 * and returns it as a string.
 */
gchar *
pyprpl_get_python_error(void);

gboolean
pyprpl_check_if_path_in_syspath(const char* path);

#endif // __PYPRPL_UTIL_H
