#ifndef __PYPRPL_UTILS_H
#define __PYPRPL_UTILS_H

#include <Python.h>
#include "purple.h"

#include "config.h"
#include "pyplug.h"

#define PYPRPL_MODULE_ID			"ID"
#define PYPRPL_MODULE_NAME			"NAME"
#define PYPRPL_MODULE_SUMMARY		"SUMMARY"
#define PYPRPL_MODULE_DESCRIPTION	"DESCRIPTION"
#define PYPRPL_MODULE_VERSION		"VERSION"
#define PYPRPL_MODULE_AUTHOR		"AUTHOR"
#define PYPRPL_MODULE_HOMEPAGE		"HOMEPAGE"
#define PYPRPL_MODULE_TYPE			"TYPE"
#define PYPRPL_MODULE_PRIORITY		"PRIORITY"

#define PYPRPL_PURPLE_VERSION		"PURPLE_VERSION"
#define PYPRPL_PURPLE_MAGIC			"PURPLE_MAGIC"
#define PYPRPL_LIBPURPLE_SONAME 	"LIBPURPLE_SONAME"

#define PYPRPL_MODULE_ONINIT		"on_init"
#define PYPRPL_MODULE_ONLOAD		"on_load"
#define PYPRPL_MODULE_ONUNLOAD		"on_unload"
#define PYPRPL_MODULE_ONDESTROY		"on_destroy"

/**
 * Probes and initalizes python module.
 */
gboolean pyprpl_probe_python_plugin(PurplePlugin *plugin);

/**
 * Wraps loading of python module.
 */
gboolean pyprpl_module_load_wrapper(PurplePlugin *plugin);

/**
 * Wraps unloading of python module.
 */ 
gboolean pyprpl_module_unload_wrapper(PurplePlugin *plugin);

/**
 * Wraps destruction of python module.
 */
void pyprpl_module_destroy_wrapper(PurplePlugin *plugin);

#endif // __PYPRPLE_UTILS_H
