#include "pyprpl-util.h"

gchar *
pyprpl_get_module_name(const char* filename)
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

/**
 * Utility function that calls str(...) on python module and
 * returns the result.
 */ 
static gchar*
stringify_pyobject(PyObject *o)
{
	PyObject *pystr = PyObject_Str(o);
	char *str = PyString_AsString(pystr);
	Py_DECREF(pystr);
	return str;
}

gchar*
pyprpl_get_python_error(void)
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

gboolean
pyprpl_check_if_path_in_syspath(const char* path)
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
