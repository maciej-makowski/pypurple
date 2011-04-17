/*
 * TODO: Make it work on Python 3.0 (UNICODE mothafucka, do you use it?!)
 */
#include <Python.h>

#include <dlfcn.h>

#include <glib.h>

#include "purple.h"

#include "config.h"
#include "pyprpl-module.h"

#define PYPRPL_PLUGIN_NAME			"Python plugin loader"
#define PYPRPL_PLUGIN_SUMMARY		"Loads python plugins"
#define PYPRPL_PLUGIN_DESCRIPTION	PYPRPL_PLUGIN_SUMMARY \
									"\nSupported python version: %s."

#define PREF_PREFIX					"/plugins/core/" PYPRPL_PLUGIN_ID
#define PREF_SYSPATH				PREF_PREFIX "/syspath"
#define PREF_SYSPATH_REPLACE		PREF_PREFIX "/syspath_replace"
#define PREF_FINALIZE_ON_UNLOAD 	PREF_PREFIX "/finalize_on_unload"

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

static void 
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
			purple_debug_warning(PYPRPL_PLUGIN_ID, "Cannot translate pyplug "
				"debug level %d to libpurple's, assuming INFO.\n", level);
			purp_level = PURPLE_DEBUG_INFO;
	}
	purple_debug(purp_level, PYPRPL_PLUGIN_ID, "%s", message);
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
		purple_debug_error(PYPRPL_PLUGIN_ID,
			"Pyplug filed to load libpython.\n");
		return FALSE;
	}
	if( !pyplug_init_enviroment ()) {
		purple_debug_error(PYPRPL_PLUGIN_ID, 
			"Pyplug failed to initialize enviroment.\n");
		return FALSE;
	}
	if( !pyplug_init_interpreter()) {
		purple_debug_error(PYPRPL_PLUGIN_ID, 
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

	g_free(plugin->info->name);
	plugin->info->name = NULL;

	g_free(plugin->info->summary);
	plugin->info->summary = NULL;

	g_free(plugin->info->description);
	plugin->info->description = NULL;

	purple_plugin = NULL;
}

static PurplePluginLoaderInfo loader_info = 
{
	NULL,				//exts

	pyprpl_probe_python_plugin,
	pyprpl_module_load_wrapper,
	pyprpl_module_unload_wrapper,
	pyprpl_module_destroy_wrapper,

	//padding
	NULL,
	NULL,
	NULL,
	NULL
};

static PurplePluginUiInfo ui_info = 
{
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

	PYPRPL_PLUGIN_ID,		// id
	NULL,		// name
	PYPRPL_PLUGIN_VERSION,		// version
	NULL,		// summary
	NULL,	// description
	PYPRPL_PLUGIN_AUTHOR,		// author
	PYPRPL_PLUGIN_HOMEPAGE,	// homepage

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
pyplug_init_plugin_info()
{
	int i = 0;
	gchar* version = g_strdup(Py_GetVersion());
	for(i = 0; i < strlen(version); i++) {
		if(version[i] == '\n') {
			version[i] = ' ';
			break;
		}
	}

	info.name = g_strdup(PYPRPL_PLUGIN_NAME);
	info.summary = g_strdup(PYPRPL_PLUGIN_SUMMARY);
	info.description = g_strdup_printf(PYPRPL_PLUGIN_DESCRIPTION,
		version);

	g_free(version);
}

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
pyplug_init_plugin(PurplePlugin *plugin)
{
	pyplug_register_preferences();
	pyplug_register_extensions();
	pyplug_init_plugin_info();
}

PURPLE_INIT_PLUGIN(PYPRPL_PLUGIN_ID, pyplug_init_plugin, info);
