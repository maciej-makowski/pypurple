#include <pyprpl-module.h>

/**
 * Default purple plugin version for freashly created module. This should be
 * changed by module in on_init.
 */
#define PYPRPL_MODULE_DEFAULT_VERSION "0.0"

/**
 * Defautl purple plugin values for info fields. This should be space and not
 * empty string, since it seems to be some buffer overflow bug in 
 * libpurple/pidgin. 
 */
#define PYPRPL_MODULE_DEFAULT_INFO_VALUE " "

/**
 * Internal structure for managing module plugin data.
 */ 
typedef struct _PYPRPL_PLUGIN_DATA {
	PurplePlugin		*plugin;
	PyObject		*module;
	PyObject		*on_init;
	PyObject		*on_load;
	PyObject		*on_unload;
	PyObject		*on_destroy;
	char			*py_module_name;
} PyplugPluginData;

/**
 * Container for data of all managed plugins.
 */ 
typedef struct _PYPRPL_DATA {
	GList	*plugins;
} PyplugData;
static PyplugData pyplug_data;

/**
 * Creates default purple plugin info for python module.
 */ 
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
	plugin_info->version = g_strdup(PYPRPL_MODULE_DEFAULT_VERSION);
	plugin_info->summary = g_strdup(PYPRPL_MODULE_DEFAULT_INFO_VALUE);
	plugin_info->description = g_strdup(PYPRPL_MODULE_DEFAULT_INFO_VALUE);
	plugin_info->author = g_strdup(PYPRPL_MODULE_DEFAULT_INFO_VALUE);
	plugin_info->homepage = g_strdup(PYPRPL_MODULE_DEFAULT_INFO_VALUE);

	return plugin_info;
}

/**
 * Creates dictionary with some immutable enviroment information passed to
 * the plugin.
 * 
 * For now it seems useful to pass python version and python plugin magic,
 * as well as name of libpurple shared library (since it is platform dependent)
 * so it can be loaded by python ctypes module.
 */
static PyObject *
create_module_data_dict(PyplugPluginData *data)
{
	PyObject *dict = PyDict_New();
	PyDict_SetItemString(dict, PYPRPL_PURPLE_MAGIC,
			PyInt_FromLong(PURPLE_PLUGIN_MAGIC));
	PyDict_SetItemString(dict, PYPRPL_PURPLE_VERSION,
			Py_BuildValue("(ii)", PURPLE_MAJOR_VERSION, PURPLE_MINOR_VERSION));
	PyDict_SetItemString(dict, PYPRPL_LIBPURPLE_SONAME, PyString_FromString(LIBPURPLE_SONAME));
	return dict;
}

/**
 * Initializes python plugin data for the module thet will be managed by this library.
 */ 
static PyplugPluginData*
init_plugin_data(PurplePlugin* plugin, PyObject *module, const char *module_name)
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

/**
 * Finds python plugin data for appropriate PurplePlugin instance.
 */
static PyplugPluginData*
get_plugin_data(PurplePlugin* plugin)
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

/**
 * Removes the python plugin data and releases all it's memory.
 */ 
static void
remove_plugin_data(PurplePlugin *plugin)
{
	PyplugPluginData* element = get_plugin_data(plugin);

	pyplug_data.plugins = g_list_remove(pyplug_data.plugins, element);

	g_free(element->py_module_name);

	Py_XDECREF(element->on_init);
	Py_XDECREF(element->on_load);
	Py_XDECREF(element->on_unload);
	Py_XDECREF(element->on_destroy);
	Py_XDECREF(element->module);
	
	g_free(element);
}

/**
 * Serches module namespace for appropriate object then checks if it is
 * callable.
 */ 
static PyObject *
find_module_method(PyObject *module, const char *module_path,
		const char *method_name)
{
	PyObject *module_dict,
			 *error,
			 *method;

	module_dict = PyModule_GetDict(module);
	method = PyDict_GetItemString(module_dict, method_name);
	if(method == NULL && (error = PyErr_Occurred()) != NULL) {
		gchar* ex_str = pyprpl_get_python_error();
		purple_debug_warning(PYPRPL_PLUGIN_ID, "Error while searching method "
			"'%s' in %s: %s\n", method_name, module_path, ex_str);
		PyMem_Free(ex_str);
		Py_DECREF(error);
		PyErr_Clear();
	} else if (method == Py_None) {
		purple_debug_warning(PYPRPL_PLUGIN_ID, "Method '%s' is not " "defined "
			"in %s.\n", method_name, module_path);
		method = NULL;
	} else if (!PyCallable_Check(method)) {
		purple_debug_warning(PYPRPL_PLUGIN_ID, "Member '%s' in %s is not "
			"callable.\n", method_name, module_path);
		method = NULL;
	}

	// If we have the method, than we need to increase it's reference count
	if(method != NULL) {
		Py_INCREF(method);
	}

	return method;
}

/**
 * Probes module for appropriate loader methods.
 */
static gboolean
find_loader_methods(PyObject *module, PyplugPluginData *plugin_data)
{
	if(!PyModule_Check(module))
		return FALSE;

	plugin_data->on_init = find_module_method(module, plugin_data->plugin->path,
		PYPRPL_MODULE_ONINIT);
	plugin_data->on_load = find_module_method(module, plugin_data->plugin->path,
		PYPRPL_MODULE_ONLOAD);
	plugin_data->on_unload = find_module_method(module, plugin_data->plugin
		->path, PYPRPL_MODULE_ONUNLOAD);
	plugin_data->on_destroy = find_module_method(module, plugin_data->plugin
		->path, PYPRPL_MODULE_ONDESTROY);

	if(plugin_data->on_load == NULL || plugin_data->on_unload == NULL) {
		purple_debug_error(PYPRPL_PLUGIN_ID, "Required methods not defined "
			"in module %s.\n", plugin_data->plugin->path);
		return FALSE;
	}

	plugin_data->plugin->info->load = plugin_data->on_load != NULL 
		? pyprpl_module_load_wrapper : NULL;
	plugin_data->plugin->info->unload = plugin_data->on_unload != NULL 
		? pyprpl_module_unload_wrapper : NULL;
	plugin_data->plugin->info->destroy = plugin_data->on_destroy != NULL 
		? pyprpl_module_destroy_wrapper : NULL;

	return TRUE;
}


#define SET_STRING_IN_PLUGIN_DICT(KEY, VALUE, DEFAULT) \
	PyDict_SetItemString(dict, KEY, \
			PyString_FromString( \
				plugin->VALUE == NULL || strlen(plugin->VALUE) == 0 \
				? DEFAULT \
				: plugin->VALUE) \
			);

#define SET_INT_IN_PLUGIN_DICT(KEY, VALUE) \
	PyDict_SetItemString(dict, KEY, PyInt_FromLong(plugin->VALUE));

/**
 * Converts PurplePluginInfo to a dictionary so it can be passed to the python code for
 * required modifications.
 */
static PyObject *
plugin_to_pydict(PurplePluginInfo *plugin, const char *module_name)
{
	PyObject *dict = PyDict_New();

	plugin->type = PURPLE_PLUGIN_STANDARD;
	plugin->priority = PURPLE_PRIORITY_DEFAULT;

	SET_INT_IN_PLUGIN_DICT(PYPRPL_MODULE_TYPE, type);
	SET_INT_IN_PLUGIN_DICT(PYPRPL_MODULE_PRIORITY, priority);

	SET_STRING_IN_PLUGIN_DICT(PYPRPL_MODULE_ID, id, module_name);
	SET_STRING_IN_PLUGIN_DICT(PYPRPL_MODULE_NAME, name, module_name);
	SET_STRING_IN_PLUGIN_DICT(PYPRPL_MODULE_SUMMARY, summary, "");
	SET_STRING_IN_PLUGIN_DICT(PYPRPL_MODULE_DESCRIPTION, description, "");
	SET_STRING_IN_PLUGIN_DICT(PYPRPL_MODULE_AUTHOR, author, "");
	SET_STRING_IN_PLUGIN_DICT(PYPRPL_MODULE_VERSION, version, "");
	SET_STRING_IN_PLUGIN_DICT(PYPRPL_MODULE_HOMEPAGE, homepage, "");

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
/**
 * Applies modifications made on the python side to the PurplePluginInfo.
 */
static void
update_module_plugin_data(PurplePluginInfo *info, PyObject *dict)
{
	UPDATE_PLUGIN_INFO_INT(type, PYPRPL_MODULE_TYPE);
	UPDATE_PLUGIN_INFO_INT(priority, PYPRPL_MODULE_PRIORITY);

	UPDATE_PLUGIN_INFO_STRING(id, PYPRPL_MODULE_ID);
	UPDATE_PLUGIN_INFO_STRING(name, PYPRPL_MODULE_NAME);
	UPDATE_PLUGIN_INFO_STRING(description, PYPRPL_MODULE_DESCRIPTION);
	UPDATE_PLUGIN_INFO_STRING(summary, PYPRPL_MODULE_SUMMARY);
	UPDATE_PLUGIN_INFO_STRING(author, PYPRPL_MODULE_AUTHOR);
	UPDATE_PLUGIN_INFO_STRING(version, PYPRPL_MODULE_VERSION);
	UPDATE_PLUGIN_INFO_STRING(homepage, PYPRPL_MODULE_HOMEPAGE);
}

/**
 * This is callback type to call after calling module loder method. Decided to
 * use callback so there is no need for duplicating large amounds of code that
 * calls the appropriate module loader method just to execute some slightly
 * different logic depending on function type.
 *
 * For now it is only called after on_init, but may be useful in future.
 */
typedef void (*OnAfterUtilFunc)(PyplugPluginData *plugin, gboolean result,
		PyObject *plugin_dict);

/**
 * Calls function as module loader function. It assumes that function needs 2
 * parameters:
 *	- the PythonPluginInfo converted to a python dictionary,
 *	- additional dictionary that contains some immutable enviroment data,
 *	  for example python version
 */
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

	plugin_info_dict = plugin_to_pydict(pyplugin->plugin->info, pyplugin->py_module_name);
	data_dict = create_module_data_dict(pyplugin);
	call_tuple = PyTuple_New(2);
	PyTuple_SetItem(call_tuple, 0, plugin_info_dict);
	PyTuple_SetItem(call_tuple, 1, data_dict);
	init_ret = PyObject_CallObject(function, call_tuple);

	if(init_ret == NULL && (error = PyErr_Occurred()) != NULL) {
		gchar *estr = pyprpl_get_python_error();
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

/**
 * Callback to be executed after on_init have been called. We ignore the return value, since
 * the method could have correctly modified the PythonPluginInfo dictionary.
 */ 
static void
on_after_module_init(PyplugPluginData *pyplugin, gboolean result, PyObject *plugin_dict)
{
	update_module_plugin_data(pyplugin->plugin->info, plugin_dict);
}

/**
 * Types of module loader functions.
 */
typedef enum _PyplugModuleFunctionType {
	On_Init = 0,
	On_Load,
	On_Unload,
	On_Destroy
} PyplugModuleFunctionType;

typedef PyObject* (*PyplugModuleUtilFunctionSelector)(PyplugPluginData *plugin);

static PyObject * 
on_init_selector(PyplugPluginData *pyplugin)
{
	return pyplugin->on_init;
}

static PyObject * 
on_load_selector(PyplugPluginData *pyplugin)
{
	return pyplugin->on_load;
}

static PyObject * 
on_unload_selector(PyplugPluginData *pyplugin)
{
	return pyplugin->on_unload;
}

static PyObject * 
on_destroy_selector(PyplugPluginData *pyplugin)
{
	return pyplugin->on_destroy;
}

typedef struct _PyplugModuleFunctionSelection {
	PyplugModuleUtilFunctionSelector function;
	const char *function_name;
	OnAfterUtilFunc callback;
} PyplugModuleFunctionSelection;

static PyplugModuleFunctionSelection _MODULE_FUNCTION_SELECTORS[] = {
	{on_init_selector, "on_init", on_after_module_init},
	{on_load_selector, "on_load", NULL},
	{on_unload_selector, "on_unload", NULL},
	{on_destroy_selector, "on_destroy", NULL}
};

/**
 * Selects and calls appropriate plugin function according to assumed convention.
 */
static gboolean
call_module_util_function(PyplugPluginData *pyplugin, PyplugModuleFunctionType type)
{
	const PyplugModuleFunctionSelection *selector = &_MODULE_FUNCTION_SELECTORS[type];
	PyObject *func_object = selector->function(pyplugin);
	const gchar *func_name = selector->function_name;
	OnAfterUtilFunc after_util_cb = selector->callback;

	return call_module_python_function(func_object, pyplugin, after_util_cb);
}

gboolean
pyprpl_probe_python_plugin(PurplePlugin *plugin)
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
	module_name = pyprpl_get_module_name(file_name);

	if(!pyprpl_check_if_path_in_syspath(module_path)) {
		// Append module path to the syspath
		pyplug_syspath_set_string(module_path, FALSE);
	}

	module = PyImport_ImportModule(module_name);
	if(module == NULL && (error = PyErr_Occurred()) != NULL) {
		gchar *ex_str = pyprpl_get_python_error();
		purple_debug_error(PYPRPL_PLUGIN_ID, "Error while importing python "
			"module '%s': %s\n", module_name, ex_str);
		plugin->error = g_strdup(ex_str);
		PyMem_Free(ex_str);
		probe_result = FALSE;
	}

	pyplugin_data = init_plugin_data(plugin, module, module_name);
	if(probe_result && !find_loader_methods(module, pyplugin_data)) {
		plugin->error = "Module does not define required methods.";
		probe_result = FALSE;
	}

	if(probe_result) {
		probe_result = call_module_util_function(pyplugin_data, On_Init);
	}

	if(!probe_result) {
		remove_plugin_data(plugin);
		plugin->unloadable = !probe_result;
	}

	purple_debug_info(PYPRPL_PLUGIN_ID, probe_result
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

gboolean
pyprpl_module_load_wrapper(PurplePlugin *plugin)
{
	PyplugPluginData *data = get_plugin_data(plugin);
	if(data != NULL) {
		return call_module_util_function(data, On_Load);
	} else {
		return FALSE;
	}
}

gboolean
pyprpl_module_unload_wrapper(PurplePlugin *plugin)
{
	PyplugPluginData *data = get_plugin_data(plugin);
	if(data != NULL) {
		return call_module_util_function(data, On_Unload);
	} else {
		return FALSE;
	}
}

void
pyprpl_module_destroy_wrapper(PurplePlugin *plugin)
{
	PyplugPluginData *data = get_plugin_data(plugin);
	if(data != NULL) {
		call_module_util_function(data, On_Destroy);
	}
}

