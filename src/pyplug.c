#include <Python.h>
#include <dlfcn.h>

#include "pyplug.h"
#include "config.h"

static gboolean 	_pyplug_initialized = FALSE;
static void		*_libpython_handle = NULL;
static GSList		*_dbg_handlers = NULL;

static void
pyplug_debug_misc(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	pyplug_debug(PYPLUG_LEVEL_MISC, format, args);
	va_end(args);
}

static void
pyplug_debug_info(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	pyplug_debug(PYPLUG_LEVEL_INFO, format, args);
	va_end(args);
}

static void
pyplug_debug_warning(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	pyplug_debug(PYPLUG_LEVEL_WARNING, format, args);
	va_end(args);
}

static void
pyplug_debug_error(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	pyplug_debug(PYPLUG_LEVEL_ERROR, format, args);
	va_end(args);
}

static void
pyplug_debug_failure(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	pyplug_debug(PYPLUG_LEVEL_FAILURE, format, args);
	va_end(args);
}

static void
pyplug_debug_pyerr(PyObject *exception)
{
	PyObject *ex_string;
	const char *ex_text;

	ex_string = PyObject_Str(exception);
	ex_text = PyString_AsString(ex_string);

	pyplug_debug_error("%s\n", ex_text);

	Py_DECREF(ex_string);
}

gboolean
pyplug_is_initialized(void)
{
	return _pyplug_initialized;
}

void
pyplug_initialize(void)
{
	if(_dbg_handlers != NULL) {
		g_slist_free(_dbg_handlers);
	}
	_dbg_handlers = g_slist_alloc();
	_pyplug_initialized = TRUE;
}

void
pyplug_finalize(void)
{
	if(_dbg_handlers != NULL) {
		g_slist_free(_dbg_handlers);
		_dbg_handlers = NULL;
	}

	if(pyplug_libpython_is_loaded()) {
		(void)pyplug_libpython_unload();
	}

	_pyplug_initialized = FALSE;
}

void
pyplug_reg_dbgcb (PyPlugDebugFnc callback)
{
	pyplug_debug_misc("Registering function '%#x' as debug handler.\n",
		(unsigned int)callback);
	_dbg_handlers = g_slist_prepend(_dbg_handlers, (gpointer)callback);
}

void
pyplug_unreg_dbgcb (PyPlugDebugFnc callback)
{
	pyplug_debug_misc("Unregistering function '%#x' from debug "
		"handlers.\n", (unsigned int)callback);
	_dbg_handlers = g_slist_remove(_dbg_handlers, (gpointer)callback);
}

typedef struct _PyPlugDebugData {
	PyPlugDebugLevel	level;
	const char		*message;
} PyPlugDebugData;

static void
pyplug_debug_call_handler (gpointer data, gpointer user_data)
{
	PyPlugDebugFnc cb = (PyPlugDebugFnc)data;
	PyPlugDebugData* dbgdata = (PyPlugDebugData*)user_data;

	if(cb != NULL) {
		(*cb)(dbgdata->level, dbgdata->message);
	}
}

void
pyplug_debug (PyPlugDebugLevel level, const char* format, ...)
{
	if(_dbg_handlers != NULL) {
		va_list 	args;
		PyPlugDebugData data;
		char		*message;

		va_start(args, format);
		message = g_strdup_vprintf(format, args);
		va_end(args);

		data.level = level;
		data.message = message;

		g_slist_foreach(_dbg_handlers, pyplug_debug_call_handler, &data);
		g_free(message);

	}
}



gboolean
pyplug_libpython_load (void)
{ 
	pyplug_debug_misc("Loading shared library '%s'.\n", PYTHON_SONAME);
	_libpython_handle = dlopen(PYTHON_SONAME, RTLD_LAZY | RTLD_GLOBAL);
	if(_libpython_handle == NULL) {
		pyplug_debug_error("Error on dlopen '%s': %s\n",
			PYTHON_SONAME, dlerror());
		return FALSE;
	}
	pyplug_debug_info("Shared library '%s' loaded successfully.\n");
	return TRUE;
}

gboolean
pyplug_libpython_unload (void)
{
	int result = 0;
	if(_libpython_handle != NULL) {
		if((result = dlclose(_libpython_handle)) != 0) {
			pyplug_debug_error("Error when dlclose %s: (%d) %s\n",
				PYTHON_SONAME, result, dlerror());
		} else {
			pyplug_debug_info("Shared library '%s' unloaded "
				"successfully.\n", PYTHON_SONAME);

		}
		_libpython_handle = NULL;
	} else {
		pyplug_debug_misc("Shared library '%s' not loaded.\n",
			PYTHON_SONAME);
	}
	return result == 0;
}

gboolean
pyplug_libpython_is_loaded (void)
{
	return _libpython_handle != NULL;
}

//
// Internal structure for holding python interpreter data.
//
typedef struct PYTHON_PLUGIN {
	PyThreadState	*interpreter;
	PyThreadState	*global_interpreter;
	PyObject	*mod_pypurple;
} PythonPluginData;
static PythonPluginData py_plugin;

gboolean
pyplug_init_enviroment ()
{
	pyplug_debug_info("Initializing python enviroment for '%s'.\n",
		Py_GetVersion());

	// Init enviroment if needed 
	if(!Py_IsInitialized()) {
		pyplug_debug_misc("Initalizing python API.\n");
		Py_Initialize();
	}

	// Init python interpreters API so we can create our own subinterpreter
	if(!PyEval_ThreadsInitialized()) {
		pyplug_debug_misc("Initializing python interpreter threads "
			"API.\n");
		PyEval_InitThreads();
	}

	// Obtain global interpreter (required when finalizing enviroment)
	if(py_plugin.global_interpreter == NULL) {
		py_plugin.global_interpreter = PyThreadState_Get();
		(void)PyThreadState_Swap(py_plugin.global_interpreter);
	}
	return Py_IsInitialized() && PyEval_ThreadsInitialized();
}

void
pyplug_finalize_enviroment (gboolean keep_python)
{
	pyplug_debug_info("Finalizing python enviroment for '%s'.\n",
		Py_GetVersion());

	// Load global interpreter before finalization (Py_Finalize may 
	// cause SIGSEGV otherwise)
	(void)PyThreadState_Swap(py_plugin.global_interpreter);

	// Finalize enviroment if we are allowed to do it
	if(!keep_python) {
		pyplug_debug_misc("Unloading python API.");
		Py_Finalize();
		py_plugin.global_interpreter = NULL;
	}
}

static void 
pyplug_activate_interpreter (void)
{
	pyplug_debug_info("Activating interpreter for '%s'.\n",
			Py_GetVersion());
	(void)PyThreadState_Swap(py_plugin.interpreter);
}

gboolean
pyplug_init_interpreter (void)
{
	pyplug_debug_info("Initializing interpreter for '%s'.\n", 
			Py_GetVersion());

	if(py_plugin.interpreter == NULL) {
		py_plugin.interpreter = Py_NewInterpreter();

		if(py_plugin.interpreter != NULL) {
			pyplug_debug_misc("Interpreter successfully initialized"
				"under 0x%X.\n",
				(unsigned int)py_plugin.interpreter);
			pyplug_activate_interpreter();
			return TRUE;
		} else {
			pyplug_debug_error("Could not initialize new "
				"interpreter.\n");
			return FALSE;
		}
	}
}

void 
pyplug_free_interpreter (void)
{
	pyplug_debug_info("Releasing interpreter for '%s'.\n", 
			Py_GetVersion());
	if(py_plugin.interpreter != NULL) {
		pyplug_debug_misc("Releasing interpreter under 0x%X.\n",
				(unsigned int)py_plugin.interpreter);

		if(PyThreadState_Get() != py_plugin.interpreter)
			// interpreter must hold GIL during unloading
			pyplug_activate_interpreter();

		Py_EndInterpreter(py_plugin.interpreter);
		// activate global interpreter
		PyThreadState_Swap(py_plugin.global_interpreter);
	}
	py_plugin.interpreter = NULL;
}

#define PY_MDL_SYS	"sys"

#define PY_SMBL_PATH	"path"


gchar**
pyplug_syspath_get(void)
{
	gint count = 0, 
	     i = 0;
	gchar** elements = NULL;
	PyObject *imp_sys, 
		 *dic_sys, 
		 *lst_path;

	imp_sys = PyImport_ImportModule(PY_MDL_SYS);
	dic_sys = PyModule_GetDict(imp_sys);
	lst_path = PyDict_GetItemString(dic_sys, PY_SMBL_PATH);

	count = PyList_Size(lst_path);
	elements = g_new0(gchar*, count+1);

	for(i = 0; i < count; i++) {
		PyObject* item = PyList_GetItem(lst_path, i);
		const char* val = PyString_AsString(item);
		elements[i] = g_strdup(val);
	}
	elements[count] = NULL;

	Py_DECREF(imp_sys);
	return elements;
}


gboolean
pyplug_syspath_set_stringv(const gchar **paths, gboolean replace)
{
	int 		i = 0,
			pyresult = 0;
	PyObject	*new_syspath,
			*mod_sys,
			*dic_sys,
			*exc_error;

	new_syspath = PyList_New(0);

	if(!replace) {
		gchar **syspaths = pyplug_syspath_get();

		i = 0;
		while(syspaths != NULL && syspaths[i] != NULL) {
			PyObject* string = PyString_FromString(syspaths[i]);
			PyList_Append(new_syspath, string);
			i++;
		}
		g_strfreev(syspaths);
	}

	i = 0;
	while(paths != NULL && paths[i] != NULL) {
		PyObject* string = PyString_FromString(paths[i]);
		PyList_Append(new_syspath, string);
		i++;
	}
			
	mod_sys = PyImport_ImportModule(PY_MDL_SYS);
	if(mod_sys == NULL && (exc_error = PyErr_Occurred()) != NULL) {
		pyplug_debug_error("Could not import module '%s'.\n", PY_MDL_SYS);
		return FALSE;
	}

	dic_sys = PyModule_GetDict(mod_sys);
	pyresult = PyDict_SetItemString(dic_sys, PY_SMBL_PATH, new_syspath);
	if(pyresult != 0 && (exc_error = PyErr_Occurred()) != NULL) {
		pyplug_debug_error("Could not obtain symbol '%s' from module "
			"'%s' dictionary.\n", PY_SMBL_PATH, PY_MDL_SYS);
		Py_DECREF(mod_sys);
		return FALSE;
	}

	Py_DECREF(mod_sys);
	return TRUE;
}

gboolean
pyplug_syspath_set_string(const gchar *syspath, gboolean replace)
{
	gboolean result = FALSE;
	gchar **split = NULL;

	split = g_strsplit(syspath, ":", -1);
	result = pyplug_syspath_set_stringv(split, replace);
	g_strfreev(split);
	return result;
}

gboolean
pyplug_import_module(const gchar* module_name)
{
	PyObject	*module,
			*error;

	module = PyImport_ImportModule(module_name);
	if(module == NULL && (error = PyErr_Occurred()) != NULL) {
		pyplug_debug_pyerr(error);
		return FALSE;
	}

	Py_DECREF(module);
	return TRUE;
}
