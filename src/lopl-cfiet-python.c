/*
 * TODO: Make it work on Python 3.0 (UNICODE mothafucka, do you use it?!)
 * TODO: Refactor function names, private ones should not pollute file with pyplug_ prefix
 * TODO: Move defines somewhere else
 */
#include <Python.h>

#include <dlfcn.h>

#include <glib.h>

#include "purple.h"

#include "config.h"
#include "pyplug.h"

#define PYPLUG_PLUGIN_NAME			"Python plugin loader"
#define PYPLUG_PLUGIN_SUMMARY		"Loads python plugins"
#define PYPLUG_PLUGIN_DESCRIPTION	PYPLUG_PLUGIN_SUMMARY \
	"\nSupported python version: %s."

#define PREF_PREFIX		"/plugins/core/" PYPLUG_PLUGIN_ID
#define PREF_SYSPATH		PREF_PREFIX "/syspath"
#define PREF_SYSPATH_REPLACE	PREF_PREFIX "/syspath_replace"
#define PREF_FINALIZE_ON_UNLOAD PREF_PREFIX "/finalize_on_unload"

#define PYPLUG_MODULE_ID			"ID"
#define PYPLUG_MODULE_NAME			"NAME"
#define PYPLUG_MODULE_SUMMARY		"SUMMARY"
#define PYPLUG_MODULE_DESCRIPTION	"DESCRIPTION"
#define PYPLUG_MODULE_VERSION		"VERSION"
#define PYPLUG_MODULE_AUTHOR		"AUTHOR"
#define PYPLUG_MODULE_HOMEPAGE		"HOMEPAGE"
#define PYPLUG_MODULE_TYPE			"TYPE"
#define PYPLUG_MODULE_PRIORITY		"PRIORITY"

#define PYPLUG_PURPLE_VERSION		"PURPLE_VERSION"
#define PYPLUG_PURPLE_MAGIC			"PURPLE_MAGIC"
#define PYPLUG_LIBPURPLE_SONAME 	"LIBPURPLE_SONAME"

#define PYPLUG_MODULE_ONINIT		"on_init"
#define PYPLUG_MODULE_ONLOAD		"on_load"
#define PYPLUG_MODULE_ONUNLOAD		"on_unload"
#define PYPLUG_MODULE_ONDESTROY		"on_destroy"

typedef struct _PYPLUG_PLUGIN_DATA {
	PurplePlugin	*plugin;
	PyObject		*module;
	PyObject		*on_init;
	PyObject		*on_load;
	PyObject		*on_unload;
	PyObject		*on_destroy;
	char			*py_module_name;
} PyplugPluginData;

typedef struct _PYPLUG_DATA {
	GList	*plugins;
} PyplugData;

static PyplugData pyplug_data;
//
// This plugin data.
//
static PurplePlugin *purple_plugin;

static gboolean module_load_wrapper(PurplePlugin *plugin);
static gboolean module_unload_wrapper(PurplePlugin *plugin);
static void module_destroy_wrapper(PurplePlugin *plugin);

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

	g_free(plugin->info->name);
	plugin->info->name = NULL;

	g_free(plugin->info->summary);
	plugin->info->summary = NULL;

	g_free(plugin->info->description);
	plugin->info->description = NULL;

	purple_plugin = NULL;
}

static gboolean
pyplug_check_if_path_in_syspath(const char* path)
{
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
pyplug_get_module_name(const char* filename)
{
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

#define PYPLUG_MODULE_DEFAULT_VERSION "0.0"
#define PYPLUG_MODULE_DEFAULT_INFO_VALUE " "

static PurplePluginInfo *
create_module_plugin_info(const gchar * module_name)
{
	PurplePluginInfo * plugin_info = g_new0(PurplePluginInfo, 1);

	plugin_info->magic = PURPLE_PLUGIN_MAGIC;
	plugin_info->major_version = PURPLE_MAJOR_VERSION;
	plugin_info->minor_version = PURPLE_MINOR_VERSION;
	plugin_info->type = PURPLE_PLUGIN_STANDARD;
	plugin_info->priority = PURPLE_PRIORITY_DEFAULT;

	plugin_info->id = g_strdup(module_name);
	plugin_info->name = g_strdup(module_name);
	plugin_info->version = g_strdup(PYPLUG_MODULE_DEFAULT_VERSION);
	plugin_info->summary = g_strdup(PYPLUG_MODULE_DEFAULT_INFO_VALUE);
	plugin_info->description = g_strdup(PYPLUG_MODULE_DEFAULT_INFO_VALUE);
	plugin_info->author = g_strdup(PYPLUG_MODULE_DEFAULT_INFO_VALUE);
	plugin_info->homepage = g_strdup(PYPLUG_MODULE_DEFAULT_INFO_VALUE);

	return plugin_info;
}

static PyObject *
create_module_data_dict(PyplugPluginData *data)
{
	PyObject *dict = PyDict_New();
	PyDict_SetItemString(dict, PYPLUG_PURPLE_MAGIC,
			PyInt_FromLong(PURPLE_PLUGIN_MAGIC));
	PyDict_SetItemString(dict, PYPLUG_PURPLE_VERSION,
			Py_BuildValue("(ii)", PURPLE_MAJOR_VERSION, PURPLE_MINOR_VERSION));
	PyDict_SetItemString(dict, PYPLUG_LIBPURPLE_SONAME, PyString_FromString(LIBPURPLE_SONAME));
	return dict;
}

static PyplugPluginData*
pyplug_init_plugin_data(PurplePlugin* plugin, PyObject *module, const char *module_name)
{
	PyplugPluginData* data = g_new0(PyplugPluginData, 1);
	data->plugin = plugin;
	data->plugin->info = create_module_plugin_info(module_name);
	data->py_module_name = g_strdup(module_name);
	data->module = module;
	Py_INCREF(data->module);

	pyplug_data.plugins = g_list_append(pyplug_data.plugins, data);

	return data;
}

static PyplugPluginData*
pyplug_get_plugin_data(PurplePlugin* plugin)
{
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
pyplug_remove_plugin_data(PurplePlugin *plugin)
{
	PyplugPluginData* element = pyplug_get_plugin_data(plugin);

	pyplug_data.plugins = g_list_remove(pyplug_data.plugins, element);

	Py_XDECREF(element->on_init);
	Py_XDECREF(element->on_load);
	Py_XDECREF(element->on_unload);
	Py_XDECREF(element->on_destroy);
	Py_XDECREF(element->module);
	
	g_free(element);
}

static gchar*
stringify_pyobject(PyObject *o)
{
	PyObject *pystr = PyObject_Str(o);
	char *str = PyString_AsString(pystr);
	Py_DECREF(pystr);
	return str;
}

static gchar*
get_python_error(void)
{
	PyObject *type = NULL,
			 *value = NULL,
			 *stacktrace = NULL;
	gchar*	error = NULL,
			*str_type = NULL,
			*str_value = NULL;

	PyErr_Fetch(&type, &value, &stacktrace);
	if(type != NULL && value != NULL) {
		PyErr_NormalizeException(&type, &value, &stacktrace);
		str_type = stringify_pyobject(type);
		str_value = stringify_pyobject(value);

		error = g_strdup_printf("%s: %s", str_type, str_value);

		Py_XDECREF(stacktrace);
		Py_DECREF(value);
		Py_DECREF(type);
	}
	return error;
}

static PyObject *
pyplug_find_method(PyObject *module, const char *module_path,
		const char *method_name)
{
	PyObject *module_dict,
			 *error,
			 *method;

	module_dict = PyModule_GetDict(module);
	method = PyDict_GetItemString(module_dict, method_name);
	if(method == NULL && (error = PyErr_Occurred()) != NULL) {
		gchar* ex_str = get_python_error();
		purple_debug_warning(PYPLUG_PLUGIN_ID, "Error while searching method "
			"'%s' in %s: %s\n", method_name, module_path, ex_str);
		PyMem_Free(ex_str);
		Py_DECREF(error);
		PyErr_Clear();
	} else if (method == Py_None) {
		purple_debug_warning(PYPLUG_PLUGIN_ID, "Method '%s' is not " "defined "
			"in %s.\n", method_name, module_path);
		method = NULL;
	} else if (!PyCallable_Check(method)) {
		purple_debug_warning(PYPLUG_PLUGIN_ID, "Member '%s' in %s is not "
			"callable.\n", method_name, module_path);
		method = NULL;
	}

	// If we have the method, than we need to increase it's reference count
	if(method != NULL) {
		Py_INCREF(method);
	}

	return method;
}

static PyObject *
pyplug_find_property(PyObject *module, const char *module_path,
		const char *property_name)
{
	PyObject *module_dict = NULL,
			 *error = NULL,
			 *property = NULL;

	module_dict = PyModule_GetDict(module);
	property = PyDict_GetItemString(module_dict, property_name);
	if(property == NULL && (error = PyErr_Occurred()) != NULL) {
		gchar* ex_str = get_python_error();
		purple_debug_warning(PYPLUG_PLUGIN_ID, "Error while searching method "
			"'%s' in %s: %s\n", property_name, module_path, ex_str);
		PyMem_Free(ex_str);
		Py_DECREF(error);
		PyErr_Clear();
	} else if (property == Py_None) {
		purple_debug_warning(PYPLUG_PLUGIN_ID, "Property '%s' is not " "defined "
			"in %s.\n", property_name, module_path);
		property = NULL;
	}

	// If we have the method, than we need to increase it's reference count
	if(property != NULL) {
		Py_INCREF(property);
	}
	return property;
}

static char *
pyplug_get_string_property(PyplugPluginData *plugin_data, const char *property_name)
{
	char *str = NULL;
	const char *module_path = plugin_data->plugin->path;
	PyObject *module = plugin_data->module,
			 *property = NULL;

	property = pyplug_find_property(module, module_path,
			property_name);
	if(property != NULL && PyString_Check(property)) {
		char *prop_str = PyString_AsString(property);
		str = g_strdup(prop_str);
	}
	Py_XDECREF(property);
	return str;
}

static gboolean
pyplug_find_loader_methods(PyObject *module, PyplugPluginData *plugin_data)
{
	if(!PyModule_Check(module))
		return FALSE;

	plugin_data->on_init = pyplug_find_method(module, plugin_data->plugin->path,
		PYPLUG_MODULE_ONINIT);
	plugin_data->on_load = pyplug_find_method(module, plugin_data->plugin->path,
		PYPLUG_MODULE_ONLOAD);
	plugin_data->on_unload = pyplug_find_method(module, plugin_data->plugin
		->path, PYPLUG_MODULE_ONUNLOAD);
	plugin_data->on_destroy = pyplug_find_method(module, plugin_data->plugin
		->path, PYPLUG_MODULE_ONDESTROY);

	if(plugin_data->on_load == NULL || plugin_data->on_unload == NULL) {
		purple_debug_error(PYPLUG_PLUGIN_ID, "Required methods not defined "
			"in module %s.\n", plugin_data->plugin->path);
		return FALSE;
	}

	plugin_data->plugin->info->load = plugin_data->on_load != NULL 
		? module_load_wrapper : NULL;
	plugin_data->plugin->info->unload = plugin_data->on_unload != NULL 
		? module_unload_wrapper : NULL;
	plugin_data->plugin->info->destroy = plugin_data->on_destroy != NULL 
		? module_destroy_wrapper : NULL;

	return TRUE;
}

#define SET_STRING_IN_PLUGIN_DICT(KEY, VALUE, DEFAULT) \
	PyDict_SetItemString(dict, KEY, \
			PyString_FromString( \
				plugin->info->VALUE == NULL || strlen(plugin->info->VALUE) == 0 \
				? DEFAULT \
				: plugin->info->VALUE) \
			);

#define SET_INT_IN_PLUGIN_DICT(KEY, VALUE) \
	PyDict_SetItemString(dict, KEY, PyInt_FromLong(plugin->info->VALUE));

static PyObject *
plugin_to_pydict(PurplePlugin *plugin, const char *module_name)
{
	PyObject *dict = PyDict_New();

	plugin->info->type = PURPLE_PLUGIN_STANDARD;
	plugin->info->priority = PURPLE_PRIORITY_DEFAULT;

	SET_INT_IN_PLUGIN_DICT(PYPLUG_MODULE_TYPE, type);
	SET_INT_IN_PLUGIN_DICT(PYPLUG_MODULE_PRIORITY, priority);

	SET_STRING_IN_PLUGIN_DICT(PYPLUG_MODULE_ID, id, module_name);
	SET_STRING_IN_PLUGIN_DICT(PYPLUG_MODULE_NAME, name, module_name);
	SET_STRING_IN_PLUGIN_DICT(PYPLUG_MODULE_SUMMARY, summary, "");
	SET_STRING_IN_PLUGIN_DICT(PYPLUG_MODULE_DESCRIPTION, description, "");
	SET_STRING_IN_PLUGIN_DICT(PYPLUG_MODULE_AUTHOR, author, "");
	SET_STRING_IN_PLUGIN_DICT(PYPLUG_MODULE_VERSION, version, "");
	SET_STRING_IN_PLUGIN_DICT(PYPLUG_MODULE_HOMEPAGE, homepage, "");

	return dict;
}

#define UPDATE_PLUGIN_INFO_INT(field, key) { \
		PyObject *value = PyDict_GetItemString(dict, key); \
		if(value != Py_None) { \
			info->field = PyInt_AsLong(value); \
		} \
	}

#define UPDATE_PLUGIN_INFO_STRING(field, key) { \
		PyObject *value = PyDict_GetItemString(dict, key); \
		if(value != Py_None) { \
			g_free(info->field); \
			info->field = PyString_AsString(value); \
		} \
	}

static void
update_module_plugin_data(PurplePluginInfo *info, PyObject *dict)
{
	UPDATE_PLUGIN_INFO_INT(type, PYPLUG_MODULE_TYPE);
	UPDATE_PLUGIN_INFO_INT(priority, PYPLUG_MODULE_PRIORITY);

	UPDATE_PLUGIN_INFO_STRING(id, PYPLUG_MODULE_ID);
	UPDATE_PLUGIN_INFO_STRING(name, PYPLUG_MODULE_NAME);
	UPDATE_PLUGIN_INFO_STRING(description, PYPLUG_MODULE_DESCRIPTION);
	UPDATE_PLUGIN_INFO_STRING(summary, PYPLUG_MODULE_SUMMARY);
	UPDATE_PLUGIN_INFO_STRING(author, PYPLUG_MODULE_AUTHOR);
	UPDATE_PLUGIN_INFO_STRING(version, PYPLUG_MODULE_VERSION);
	UPDATE_PLUGIN_INFO_STRING(homepage, PYPLUG_MODULE_HOMEPAGE);
}

typedef void (*OnAfterUtilFunc)(PyplugPluginData *plugin, gboolean result,
		PyObject *plugin_dict);

static gboolean
call_module_python_function(PyObject *function, PyplugPluginData *pyplugin, 
		OnAfterUtilFunc after_func_cb) 
{
	gboolean result = FALSE;
	PyObject *error = NULL,
			 *plugin_info_dict = NULL,
			 *data_dict = NULL,
			 *call_tuple = NULL,
			 *init_ret = NULL;

	plugin_info_dict = plugin_to_pydict(pyplugin->plugin, pyplugin->py_module_name);
	data_dict = create_module_data_dict(pyplugin);
	call_tuple = PyTuple_New(2);
	PyTuple_SetItem(call_tuple, 0, plugin_info_dict);
	PyTuple_SetItem(call_tuple, 1, data_dict);
	init_ret = PyObject_CallObject(function, call_tuple);

	if(init_ret == NULL && (error = PyErr_Occurred()) != NULL) {
		gchar *estr = get_python_error();
		pyplugin->plugin->error = estr;
		purple_debug_error(pyplugin->plugin->info->id, "Error while loading "
			"plugin module %s: %s\n", pyplugin->py_module_name, estr);
	}

	if(init_ret != NULL) {
		result = (init_ret != Py_None) && (init_ret == Py_True);
	} else {
		result = FALSE;
	}

	if(after_func_cb != NULL) {
		after_func_cb(pyplugin, result, plugin_info_dict);
	}

	Py_XDECREF(data_dict);
	Py_XDECREF(plugin_info_dict);
	Py_XDECREF(init_ret);
	Py_XDECREF(call_tuple);
	Py_XDECREF(error);

	return result;
}

void
on_after_module_init(PyplugPluginData *pyplugin, gboolean result, PyObject *plugin_dict)
{
	update_module_plugin_data(pyplugin->plugin->info, plugin_dict);
}

typedef enum _PyplugModuleFunctionType {
	On_Init = 1,
	On_Load,
	On_Unload,
	On_Destroy
} PyplugModuleFunctionType;

static gboolean
call_module_util_function(PyplugPluginData *pyplugin, PyplugModuleFunctionType type)
{
	PyObject *func_object;
	gchar *func_name;
	OnAfterUtilFunc after_util_cb = NULL;

	switch(type) {
		case On_Init:
			func_object = pyplugin->on_init;
			func_name = "on_init";
			after_util_cb = on_after_module_init;
			break;
		case On_Load:
			func_object = pyplugin->on_load;
			func_name = "on_load";
			break;
		case On_Unload:
			func_object = pyplugin->on_unload;
			func_name = "on_unload";
			break;
		case On_Destroy:
			func_object = pyplugin->on_destroy;
			func_name = "on_destroy";
			break;
		default:
			return FALSE;
	}
	return call_module_python_function(func_object, pyplugin, after_util_cb);
}

static gboolean
module_load_wrapper(PurplePlugin *plugin)
{
	PyplugPluginData *data = pyplug_get_plugin_data(plugin);
	if(data != NULL) {
		return call_module_util_function(data, On_Load);
	} else {
		return FALSE;
	}
}

static gboolean
module_unload_wrapper(PurplePlugin *plugin)
{
	PyplugPluginData *data = pyplug_get_plugin_data(plugin);
	if(data != NULL) {
		return call_module_util_function(data, On_Unload);
	} else {
		return FALSE;
	}
}

static void
module_destroy_wrapper(PurplePlugin *plugin)
{
	PyplugPluginData *data = pyplug_get_plugin_data(plugin);
	if(data != NULL) {
		call_module_util_function(data, On_Destroy);
	}
}

static gboolean
pyplug_probe_python_plugin(PurplePlugin *plugin)
{
	gboolean probe_result = TRUE;
	PyObject	*module = NULL,
				*error = NULL;
	PyplugPluginData *pyplugin_data = NULL;

	gchar *file_name = NULL,
		  *module_path = NULL,
		  *module_name = NULL;

	file_name = g_path_get_basename(plugin->path);
	module_path = g_path_get_dirname(plugin->path);
	module_name = pyplug_get_module_name(file_name);

	if(!pyplug_check_if_path_in_syspath(module_path)) {
		// Append module path to the syspath
		pyplug_syspath_set_string(module_path, FALSE);
	}

	module = PyImport_ImportModule(module_name);
	if(module == NULL && (error = PyErr_Occurred()) != NULL) {
		gchar *ex_str = get_python_error();
		purple_debug_error(PYPLUG_PLUGIN_ID, "Error while importing python "
			"module '%s': %s\n", module_name, ex_str);
		plugin->error = g_strdup(ex_str);
		PyMem_Free(ex_str);
		probe_result = FALSE;
	}

	pyplugin_data = pyplug_init_plugin_data(plugin, module, module_name);
	if(probe_result && !pyplug_find_loader_methods(module, pyplugin_data)) {
		plugin->error = "Module does not define required methods.";
		probe_result = FALSE;
	}

	if(probe_result) {
		probe_result = call_module_util_function(pyplugin_data, On_Init);
	}

	if(!probe_result) {
		pyplug_remove_plugin_data(plugin);
		plugin->unloadable = !probe_result;
	}

	purple_debug_info(PYPLUG_PLUGIN_ID, probe_result
		? "Successfully loaded plugin module '%s'.\n"
		: "Failed to load plugin module '%s'.\n",
		module_name);

	Py_XDECREF(error);
	Py_XDECREF(module);

	g_free(module_name);
	g_free(module_path);
	g_free(file_name);

	return purple_plugin_register(plugin);
}

static gboolean
pyplug_load_python_plugin(PurplePlugin *plugin)
{
	PyplugPluginData *plugin_data = pyplug_get_plugin_data(plugin);
	if(plugin_data == NULL)
		return FALSE;

	if(plugin_data->on_load == NULL) {
		purple_debug_error(PYPLUG_PLUGIN_ID, "Module %s does not define "
			"'on_init' function.\n", plugin_data->py_module_name);
		return FALSE;
	}



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

static PurplePluginLoaderInfo loader_info = 
{
	NULL,				//exts

	pyplug_probe_python_plugin,
	module_load_wrapper,
	module_unload_wrapper,
	module_destroy_wrapper,

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

	PYPLUG_PLUGIN_ID,		// id
	NULL,		// name
	PYPLUG_PLUGIN_VERSION,		// version
	NULL,		// summary
	NULL,	// description
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

	info.name = g_strdup(PYPLUG_PLUGIN_NAME);
	info.summary = g_strdup(PYPLUG_PLUGIN_SUMMARY);
	info.description = g_strdup_printf(PYPLUG_PLUGIN_DESCRIPTION,
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

PURPLE_INIT_PLUGIN(PYPLUG_PLUGIN_ID, pyplug_init_plugin, info);
