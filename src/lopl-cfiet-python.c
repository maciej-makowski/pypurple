#define PURPLE_PLUGINS

#include <Python.h>

#include <dlfcn.h>

#include <glib.h>
#include "notify.h"
#include "plugin.h"
#include "version.h"
#include "debug.h"

#include "config.h"
#include "pyplug.h"

#define PYPLUG_PLUGIN_NAME			"Python plugin loader"
#define PYPLUG_PLUGIN_SUMMARY		"Loads python plugins"
#define PYPLUG_PLUGIN_DESCRIPTION	PYPLUG_PLUGIN_SUMMARY

#define PREF_PREFIX		"/plugins/core/" PYPLUG_PLUGIN_ID

#define PREF_PREFIX		"/plugins/core/" PYPLUG_PLUGIN_ID
#define PREF_SYSPATH		PREF_PREFIX "/syspath"
#define PREF_SYSPATH_REPLACE	PREF_PREFIX "/syspath_replace"
#define PREF_FINALIZE_ON_UNLOAD PREF_PREFIX "/finalize_on_unload"

typedef struct _PYPLUG_PLUGIN_DATA {
	PyObject		*module;
	PurplePlugin	*plugin;
} PyplugPluginData;

typedef struct _PYPLUG_DATA {
	GList	*plugins;
} PyplugData;

static PyplugData pyplug_data;
//
// This plugin data.
//
static PurplePlugin *purple_plugin;


// Method for creating libpurple options frame.
static PurplePluginPrefFrame *
pyplug_create_pref(PurplePlugin * plugin)
{
	PurplePluginPrefFrame *frame;
	PurplePluginPref *pref;

	frame = purple_plugin_pref_frame_new();

	pref = purple_plugin_pref_new_with_name_and_label(PREF_SYSPATH,
			"Additional sys.path\n(colon delimited)");
	purple_plugin_pref_frame_add(frame, pref);

	pref = purple_plugin_pref_new_with_name_and_label(PREF_SYSPATH_REPLACE,
			"Replace sys.path with above instead of appending");
	purple_plugin_pref_frame_add(frame, pref);

	pref = purple_plugin_pref_new_with_name_and_label(PREF_FINALIZE_ON_UNLOAD,
			"Finalize python enviroment when unloading plugin");
	purple_plugin_pref_frame_add(frame, pref);

	return frame;
}

void 
pyplug_libpurple_debug_callback(PyPlugDebugLevel level, const char* message)
{
	PurpleDebugLevel purp_level = PURPLE_DEBUG_ALL;
	switch(level) {
		case PYPLUG_LEVEL_ALL:
			purp_level = PURPLE_DEBUG_ALL;
			break;
		case PYPLUG_LEVEL_MISC:
			purp_level = PURPLE_DEBUG_MISC;
			break;
		case PYPLUG_LEVEL_INFO:
			purp_level = PURPLE_DEBUG_INFO;
			break;
		case PYPLUG_LEVEL_WARNING:
			purp_level = PURPLE_DEBUG_WARNING;
			break;
		case PYPLUG_LEVEL_ERROR:
			purp_level = PURPLE_DEBUG_ERROR;
			break;
		case PYPLUG_LEVEL_FAILURE:
			purp_level = PURPLE_DEBUG_FATAL;
			break;
		default:
			purple_debug_warning(PYPLUG_PLUGIN_ID, "Cannot translate pyplug "
				"debug level %d to libpurple's, assuming INFO.\n", level);
			purp_level = PURPLE_DEBUG_INFO;
	}
	purple_debug(purp_level, PYPLUG_PLUGIN_ID, "%s", message);
}	

static gboolean
pyplug_plugin_load(PurplePlugin *plugin)
{
	purple_plugin = plugin;

	if( !pyplug_is_initialized()) {
		pyplug_initialize();
		pyplug_reg_dbgcb (pyplug_libpurple_debug_callback);
	}
	if( !pyplug_libpython_is_loaded() && !pyplug_libpython_load()) {
		purple_debug_error(PYPLUG_PLUGIN_ID,
			"Pyplug filed to load libpython.\n");
		return FALSE;
	}
	if( !pyplug_init_enviroment ()) {
		purple_debug_error(PYPLUG_PLUGIN_ID, 
			"Pyplug failed to initialize enviroment.\n");
		return FALSE;
	}
	if( !pyplug_init_interpreter()) {
		purple_debug_error(PYPLUG_PLUGIN_ID, 
			"Pyplug failed to initialize interpreter.\n");
		return FALSE;
	}
	return TRUE;
}

static gboolean
pyplug_plugin_unload(PurplePlugin* plugin)
{
	pyplug_free_interpreter();
	pyplug_finalize_enviroment(TRUE);
	if( pyplug_libpython_is_loaded()) {
		pyplug_libpython_unload();
	}
	if( pyplug_is_initialized()) {
		pyplug_finalize();
	
	}
	purple_plugin = NULL;
	return TRUE;
}

static void
pyplug_plugin_destroy(PurplePlugin *plugin)
{
	pyplug_free_interpreter();
	pyplug_finalize_enviroment(TRUE);
	if( pyplug_libpython_is_loaded()) {
		pyplug_libpython_unload();
	}
	if( pyplug_is_initialized()) {
		pyplug_finalize();
	
	}
	purple_plugin = NULL;
}

static gboolean
pyplug_check_if_path_in_syspath(const char* path) {
	int index = 0;
	gboolean result = FALSE;
	gchar **syspath = pyplug_syspath_get();

	while(syspath[index] != NULL) {
		if(g_strcmp0(syspath[index], path) == 0) {
			result = TRUE;
			break;
		}
		index++;
	}

	g_strfreev(syspath);
	return result;
}

static gchar *
pyplug_get_module_name(const char* filename) {
	int i = 0;
	int len = strlen(filename);
	int ext_pos = len;

	for (i = len; i >= 0; i--) {
		if(filename[i] == '.') {
			ext_pos = i;
			break;
		}
	}

	char *module_name = g_strndup(filename, ext_pos);
	return module_name;
}

static PyplugPluginData*
pyplug_init_plugin_data(PurplePlugin* plugin) {
	PyplugPluginData* data = g_new0(PyplugPluginData, 1);
	data->plugin = plugin;

	pyplug_data.plugins = g_list_append(pyplug_data.plugins, data);

	return data;
}

static PyplugPluginData*
pyplug_get_plugin_data(PurplePlugin* plugin) {
	GList* list = pyplug_data.plugins;

	while(list != NULL) {
		PyplugPluginData* data = (PyplugPluginData*)list->data;
		if(data->plugin == plugin) {
			return data;
		}
		list = list->next;
	}
	return NULL;
}

static void
pyplug_remove_plugin_data(PurplePlugin *plugin) {
	PyplugPluginData* element = pyplug_get_plugin_data(plugin);

	pyplug_data.plugins = g_list_remove(pyplug_data.plugins, element);
	
	if(element->module != NULL) {
		Py_DECREF(element->module);
	}
	g_free(element);
}

static gboolean
pyplug_probe_python_plugin(PurplePlugin *plugin)
{
	gboolean probe_result = TRUE;

	gchar *file_name = g_path_get_basename(plugin->path);
	gchar *module_path = g_path_get_dirname(plugin->path);
	gchar *module_name = pyplug_get_module_name(file_name);
	
	PyObject	*module,
				*error;

	if(!pyplug_check_if_path_in_syspath(module_path)) {
		// Append module path to the syspath
		pyplug_syspath_set_string(module_path, FALSE);
	}

	module = PyImport_ImportModule(module_name);
	if(module == NULL && (error = PyErr_Occurred()) != NULL) {
		PyObject *ex_pystr = PyObject_Str(error);
		gchar *ex_str = PyString_AsString(ex_pystr);

		purple_debug_error(PYPLUG_PLUGIN_ID, "Error while importing python "
			"module '%s': %s\n", module_name, ex_str);

		Py_DECREF(ex_pystr);
		probe_result = FALSE;
	}

	if(error != NULL) {
		Py_DECREF(error);
	}
	if(module != NULL) {
		Py_DECREF(module);
	}

	g_free(module_name);
	g_free(module_path);
	g_free(file_name);

	pyplug_debug_info(PYPLUG_PLUGIN_ID, probe_result
		? "Successfully loaded plugin module '%s'.\n"
		: "Failed to load plugin module '%s'.\n",
		module_name);

	return probe_result;
}

static gboolean
pyplug_load_python_plugin(PurplePlugin *plugin)
{
	return TRUE;
}

static gboolean
pyplug_unload_python_plugin(PurplePlugin *plugin)
{
	return TRUE;
}

static void
pyplug_destroy_python_plugin(PurplePlugin *plugin)
{
}

static PurplePluginLoaderInfo loader_info = {
	NULL,				//exts

	pyplug_probe_python_plugin,
	pyplug_load_python_plugin,
	pyplug_unload_python_plugin,
	pyplug_destroy_python_plugin,

	//padding
	NULL,
	NULL,
	NULL,
	NULL
};

static PurplePluginUiInfo ui_info = {
	pyplug_create_pref,	// get_plugin_pref_frame
	0,			// page_num
	NULL,			// frame

	// padding
	NULL,
	NULL,
	NULL,
	NULL
};

static PurplePluginInfo info = {
	PURPLE_PLUGIN_MAGIC,	// magic
	PURPLE_MAJOR_VERSION,	// major
	PURPLE_MINOR_VERSION,	// minor
	PURPLE_PLUGIN_LOADER, // type
//	PURPLE_PLUGIN_STANDARD,
	NULL,			// ui_requirement
	0,			// flags
	NULL,			// dependencies
	PURPLE_PRIORITY_DEFAULT,// priority

	PYPLUG_PLUGIN_ID,		// id
	PYPLUG_PLUGIN_NAME,		// name
	PYPLUG_PLUGIN_VERSION,		// version
	PYPLUG_PLUGIN_SUMMARY,		// summary
	PYPLUG_PLUGIN_DESCRIPTION,	// description
	PYPLUG_PLUGIN_AUTHOR,		// author
	PYPLUG_PLUGIN_HOMEPAGE,	// homepage

	pyplug_plugin_load,	// load
	pyplug_plugin_unload,	// unload
	pyplug_plugin_destroy,	// destroy

	NULL,			// ui_info
	&loader_info,			// extra_info
	&ui_info,		// prefs_info
	NULL,			// actions

	// padding
	NULL,
	NULL,
	NULL,
	NULL
};

static void
pyplug_register_preferences()
{
	char *path = g_build_filename(purple_user_dir(), "plugins", "python", NULL);
	purple_prefs_add_string(PREF_SYSPATH, path);
	purple_prefs_add_bool(PREF_SYSPATH_REPLACE, FALSE);
	purple_prefs_add_bool(PREF_FINALIZE_ON_UNLOAD, TRUE);
	g_free(path);
}

static void
pyplug_register_extensions()
{
	loader_info.exts = g_list_append(loader_info.exts, "py");
}

static void
init_plugin(PurplePlugin *plugin)
{
	pyplug_register_preferences();
	pyplug_register_extensions();
}

PURPLE_INIT_PLUGIN(PYPLUG_PLUGIN_ID, init_plugin, info);
