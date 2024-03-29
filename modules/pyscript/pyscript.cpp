#include "pyscript.h"
#include "core/os/file_access.h"

Python* Python::singleton = NULL;


inline Ref<PyScript> Python::cast_to_script(const Variant& p_obj)
{
	return Ref<PyScript>(p_obj);
}

inline PyScriptInstance* Python::cast_to_instance(const Variant& p_obj)
{
	Object* obj = p_obj.operator Object *();
	if (obj)
		return dynamic_cast<PyScriptInstance*>(obj->get_script_instance());
	else
		print_error(String("Cast to instance failed! ") + p_obj);
	return NULL;
}

Variant Python::dir(const Variant& p_obj) const
{
	auto script = cast_to_script(p_obj);
	if (script.is_valid())
	{
		if (script->m_obj == NULL)
			return Variant();

		PyObject* dir = PyObject_Dir(script->m_obj);
		Variant ret = PyScript::py2gd(dir);
		Py_XDECREF(dir);
		return ret;
	}

	auto inst = cast_to_instance(p_obj);
	if (inst && inst->is_valid())
	{
		PyObject* dir = PyObject_Dir(inst->get_py_obj());
		Variant ret = PyScript::py2gd(dir);
		Py_XDECREF(dir);
		return ret;
	}

	return Variant();
}

String Python::str(const Variant& p_obj) const
{
	/*auto script = cast_to_script(p_obj);
	PyObject* obj = NULL;
	if (script.is_valid())
	{
		obj = script->m_obj;
	}
	else
	{
		auto inst = cast_to_instance(p_obj);
		if (inst)
		{
			obj = inst->get_py_obj();
		}
	}*/
	String ret = _str(PyScript::gd2py(p_obj));
	if (ret != "")
		return ret;

	return p_obj;
}

String Python::_str(PyObject* p_obj) const
{
	if (!p_obj)
		return "";

	auto strObj = PyObject_Str(p_obj);
	if (strObj)
	{
		auto ret = PyScript::py2gd(strObj);
		if (ret.get_type() == Variant::STRING)
			return ret;
	}
	return "";
}

Ref<Reference> Python::iter(const Variant& p_obj)
{
	auto pyscript = cast_to_script(p_obj);
	PyObject* pyobj = NULL;
	if (pyscript.is_valid())
	{
		pyobj = pyscript->get_module();
	}
	else
	{
		auto pyinst = cast_to_instance(p_obj);
		if (pyinst)
		{
			pyobj = pyinst->get_py_obj();
		}
	}
	

	if (pyobj)
	{
		auto iter = PyObject_GetIter(pyobj);
		if (iter)
		{
			auto ret = PyScript::py2gd(iter);
			Py_DECREF(iter);
			return ret;
		}
	}

	return REF();
}

Variant Python::next(const Variant& p_iter, const Variant& p_default)
{
	auto pyscript = cast_to_script(p_iter);
	PyObject* pyobj = NULL;
	if (pyscript.is_valid())
	{
		pyobj = pyscript->get_module();
	}
	else
	{
		auto pyinst = cast_to_instance(p_iter);
		if (pyinst)
		{
			pyobj = pyinst->get_py_obj();
		}
	}

	if (pyobj && PyIter_Check(pyobj))
	{
		auto result = PyIter_Next(pyobj);
		if (result)
		{
			auto ret = PyScript::py2gd(result);
			Py_XDECREF(result);
			return ret;
		}
	}

	return p_default;
}

bool Python::run_file(String p_path, Vector<String> p_argv)
{
	FileAccess* f = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V(!f, false);

	String s = f->get_as_utf8_string();
	f->close();
	memdelete(f);

	ERR_FAIL_COND_V(s == "", false);

	int argc = p_argv.size() + 1;
	wchar_t** wargv = new wchar_t* [argc];
	/*wargv[0] = Py_DecodeLocale(p_path.utf8().get_data(), nullptr);
	if (wargv[0] == nullptr)
	{
		delete[] wargv;
		return false;
	}
	for (int i = 1; i < argc; ++i)
	{
		wargv[i] = Py_DecodeLocale(p_argv[i - 1].utf8().get_data(), nullptr);
		if (wargv[i] == nullptr)
		{
			for (int j = 0; j < i; ++j)
			{
				PyMem_RawFree(wargv[j]);
				wargv[j] = nullptr;
			}
			delete[] wargv;
			return false;
		}
	}*/
	wargv[0] = p_path.ptrw();
	for (int i = 1; i < argc; ++i)
	{
		wargv[i] = p_argv.ptrw()[i - 1].ptrw();
	}
	PySys_SetArgv(argc, wargv);
	bool ret = PyRun_SimpleString(s.utf8().get_data());

	/*for (int i = 0; i < argc; ++i)
	{
		PyMem_RawFree(wargv[i]);
		wargv[i] = nullptr;
	}*/
	delete[] wargv;
	return ret;
}

void Python::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("dir", "object"), &Python::dir);
	ClassDB::bind_method(D_METHOD("str", "object"), &Python::str);
	ClassDB::bind_method(D_METHOD("iter", "object"), &Python::iter);
	ClassDB::bind_method(D_METHOD("next", "iter", "default"), &Python::next, Variant());
	ClassDB::bind_method(D_METHOD("run_file", "path", "argv"), &Python::run_file);
}

static PyObject* gd_function(PyObject* p_self, PyObject* p_args)
{
	if (PyLong_Check(p_self))
	{
		ObjectID funcRefId = PyLong_AsUnsignedLongLong(p_self);
		auto funcRef = ObjectDB::get_instance(funcRefId);
		if (ObjectDB::instance_validate(funcRef))
		{
			FuncRef* ptr = dynamic_cast<FuncRef*>(funcRef);
			if (ptr)
			{
				
				Variant::CallError err;
				int argc = PyObject_Size(p_args);
				const Variant** argptrs = NULL;
				Vector<Variant> arglist;
				arglist.resize(argc);
				auto wptr = arglist.ptrw();
				if (argc > 0)
				{
					argptrs = (const Variant**)alloca(sizeof(Variant*) * argc);
					for (int i = 0; i < argc; ++i)
					{
						auto temp = PySequence_Fast_GET_ITEM(p_args, i);
						wptr[i] = PyScript::py2gd(temp);
						argptrs[i] = &(wptr[i]);
					}
				}
				ptr->call_func(argptrs, argc, err);
			}
			
		}
	}
	Py_RETURN_NONE;
}

PyMethodDef gdFuncDef =
{
	"gd_function",
	(PyCFunction)gd_function,
	METH_VARARGS,
	NULL
};

inline PyObject* PyScript::gd2py(const Variant& p_source)
{
	return gd2py(&p_source);
}

PyObject* PyScript::gd2py(const Variant* p_source, bool p_priority)
{
	switch (p_source->get_type())
	{
	case Variant::NIL:
		return Py_None;
	case Variant::BOOL:
		return PyBool_FromLong(p_source->operator bool());
	case Variant::INT:
		return PyLong_FromLong(p_source->operator int64_t());
	case Variant::REAL:
		return PyFloat_FromDouble(p_source->operator double());
	case Variant::STRING:
		return PyUnicode_FromString(p_source->operator String().utf8().get_data());
	case Variant::VECTOR2:
	case Variant::RECT2:
	case Variant::VECTOR3:
	case Variant::TRANSFORM2D:
	case Variant::PLANE:
	case Variant::QUAT:
	case Variant::AABB:
	case Variant::BASIS:
	case Variant::TRANSFORM:
	case Variant::COLOR:
	case Variant::NODE_PATH:
	case Variant::_RID:
		return Py_None;
	case Variant::OBJECT:
	{
		if (p_source->is_ref())
		{
			Ref<PyScript> s(*p_source);
			if (s.is_valid())
			{
				auto mod = s->get_module();
				if (mod)
				{
					Py_INCREF(mod);
					return mod;
				}
				return Py_None;
			}
			Ref<FuncRef> f(*p_source);
			if (f.is_valid())
			{
				auto fObj = func_gd2py(f);
				if (fObj)
				{
					return fObj;
				}
				return Py_None;
			}
		}
		auto inst = Python::cast_to_instance(*p_source);
		if (inst && inst->is_valid())
		{
			Py_INCREF(inst->get_py_obj());
			return inst->get_py_obj();
		}
	} break;
	case Variant::DICTIONARY:
	{
		PyObject* pyDict = PyDict_New();
		Dictionary dict = p_source->operator Dictionary();
		Array keys = dict.keys();
		Array values = dict.values();
		for (int i = 0; i < dict.size(); ++i)
		{
			PyObject* pyValue = gd2py(&(values[i]));
			if (PyDict_SetItem(pyDict, gd2py(&(keys[i]), true), pyValue) != 0)
			{
				print_error("PyDict_SetItem failed!");
			}
			Py_XDECREF(pyValue);
		}

		return pyDict;
	}
	case Variant::ARRAY:
	{
		Array a = p_source->operator Array();
		PyObject* pyList;
		if (p_priority)
		{
			pyList = PyTuple_New(a.size());
			for (int i = 0; i < a.size(); ++i)
			{
				if (PyTuple_SetItem(pyList, i, gd2py(a[i])) != 0)
				{
					print_error("PyTuple_SetItem index error!");
				}
			}
		}
		else
		{
			pyList = PyList_New(a.size());
			for (int i = 0; i < a.size(); ++i)
			{
				if (PyList_SetItem(pyList, i, gd2py(a[i])) != 0)
				{
					print_error("PyList_SetItem index error!");
				}
			}
		}
		
		return pyList;
	}
	case Variant::POOL_BYTE_ARRAY:
	{
		PoolByteArray pba = p_source->operator PoolByteArray();
		PyObject* pyPba = PyByteArray_FromStringAndSize((const char*)pba.read().ptr(), pba.size());
		if (pyPba)
			return pyPba;
		return Py_None;
	}
	case Variant::POOL_INT_ARRAY:
	{
		PoolIntArray a = p_source->operator PoolIntArray();
		PyObject* pyList = PyList_New(a.size());
		for (int i = 0; i < a.size(); ++i)
		{
			if (PyList_SetItem(pyList, i, gd2py(a[i])) != 0)
			{
				print_error("PyList_SetItem index error!");
			}
		}
		return pyList;
	}
	case Variant::POOL_REAL_ARRAY:
	{
		PoolRealArray a = p_source->operator PoolRealArray();
		PyObject* pyList = PyList_New(a.size());
		for (int i = 0; i < a.size(); ++i)
		{
			if (PyList_SetItem(pyList, i, gd2py(a[i])) != 0)
			{
				print_error("PyList_SetItem index error!");
			}
		}
		return pyList;
	}
	case Variant::POOL_STRING_ARRAY:
	{
		PoolStringArray a = p_source->operator PoolStringArray();
		PyObject* pyList = PyList_New(a.size());
		for (int i = 0; i < a.size(); ++i)
		{
			if (PyList_SetItem(pyList, i, gd2py(a[i])) != 0)
			{
				print_error("PyList_SetItem index error!");
			}
		}
		return pyList;
	}
	case Variant::POOL_VECTOR2_ARRAY:
	case Variant::POOL_VECTOR3_ARRAY:
	case Variant::POOL_COLOR_ARRAY:
		return Py_None;
	}
	return Py_None;
}

Variant PyScript::py2gd(PyObject* p_source)
{
	if (p_source == NULL || p_source == Py_None)
		return Variant();
	if (p_source == Py_False)
		return false;
	if (p_source == Py_True)
		return true;

	if (PyType_Check(p_source))
	{
		//PyTypeObject* pyTp = Py_TYPE(p_source);
		Ref<PyScript> script = memnew(PyScript);
		script->set_module(p_source);
		return script;
	}
	else if (PyLong_Check(p_source))
	{
		return PyLong_AsLongLong(p_source);
	}
	else if (PyFloat_Check(p_source))
	{
		return PyFloat_AsDouble(p_source);
	}
	else if (PyByteArray_Check(p_source))
	{
		PoolByteArray pba;
		pba.resize(PyByteArray_Size(p_source));
		auto ptr = pba.write().ptr();
		char* bytes = PyByteArray_AsString(p_source);
		memcpy(ptr, bytes, sizeof(uint8_t) * pba.size());
		delete[] bytes;
		return pba;
	}
	else if (PyUnicode_Check(p_source))
	{
		Py_ssize_t strSize;
		auto str = PyUnicode_AsUTF8AndSize(p_source, &strSize);
		String ret;
		ret.parse_utf8(str, strSize);
		return ret;
	}
	else if (PyTuple_Check(p_source))
	{
		Array a;
		a.resize(PyTuple_GET_SIZE(p_source));
		for (int i = 0; i < a.size(); ++i)
		{
			a[i] = py2gd(PyTuple_GET_ITEM(p_source, i));
		}
		return a;
	}
	else if (PyList_Check(p_source))
	{
		Array a;
		a.resize(PyList_GET_SIZE(p_source));
		for (int i = 0; i < a.size(); ++i)
		{
			a[i] = py2gd(PyList_GET_ITEM(p_source, i));
		}
		return a;
	}
	else if (PyDict_Check(p_source))
	{
		Dictionary d;
		Variant k;
		PyObject* key, * value;
		Py_ssize_t pos = 0;
		
		while (PyDict_Next(p_source, &pos, &key, &value))
		{
			k = py2gd(key);
			if (k.get_type() != Variant::NIL)
			{
				d[k] = py2gd(value);
			}
		}
		return d;
	}
	else if (PySet_Check(p_source))
	{
		Array a;
		a.resize(PySet_GET_SIZE(p_source));
		PyObject* iter = PyObject_GetIter(p_source);
		PyObject* item = NULL;
		int i = 0;
		if (iter)
			item = PyIter_Next(iter);
		while (item)
		{
			a[i++] = py2gd(item);
			Py_DECREF(item);
			item = PyIter_Next(iter);
		}
		Py_XDECREF(iter);
	}
	else if (PyModule_Check(p_source))
	{
		Ref<PyScript> script = memnew(PyScript);
		script->set_module(p_source);
		return script;
	}
	PyScriptInstance* pyInst = memnew(PyScriptInstance);
	Reference* owner = memnew(Reference);
	pyInst->m_owner = owner;
	Ref<PyScript> script = memnew(PyScript);
	PyObject* tp = PyObject_Type(p_source);
	script->set_module(tp);
	pyInst->m_script = script;
	pyInst->m_owner->set_script_instance(pyInst);
	Py_INCREF(p_source);
	pyInst->m_obj = p_source;
	return REF(owner);
}

int PyScript::get_py_func_argc(PyObject* p_func)
{
	/*if (!PyCallable_Check(p_func))
		return -1;*/

	int keep = 0;
	if (PyInstanceMethod_Check(p_func))
	{
		p_func = PyInstanceMethod_GET_FUNCTION(p_func);
	}
	else if (PyMethod_Check(p_func))
	{
		p_func = PyMethod_GET_FUNCTION(p_func);
		keep = 1;
	}
	//else if (PyCFunction_Check(p_func))
	//{
	//	return -1;
	//}

	if (PyFunction_Check(p_func))
	{
		auto co = (PyCodeObject*)PyFunction_GetCode(p_func);
		return co->co_argcount/* + co->co_kwonlyargcount*/ - keep;
	}
	return -1;
}

int PyScript::get_py_func_defc(PyObject* p_func)
{
	if (!PyCallable_Check(p_func))
		return 0;

	if (PyInstanceMethod_Check(p_func))
	{
		p_func = PyInstanceMethod_GET_FUNCTION(p_func);
	}
	else if (PyMethod_Check(p_func))
	{
		p_func = PyMethod_GET_FUNCTION(p_func);
	}

	if (PyFunction_Check(p_func))
	{
		auto co = PyFunction_GetDefaults(p_func);
		if (co)
		{
			return PyObject_Size(co)/* - (PyCodeObject*)PyFunction_GetCode(p_func)->co_kwonlyargcount*/;
		}
	}
	return 0;
}

Variant PyScript::call_py_func(PyObject* p_func, const Variant** p_args, int p_argcount, Variant::CallError& r_error, PyObject* p_kwargs)
{
	if (!p_func || !PyCallable_Check(p_func))
	{
		r_error.error = Variant::CallError::CALL_ERROR_INSTANCE_IS_NULL;
		return Variant();
	}
	
	/*if (PyType_Check(func))
	{
		PyObject* pRet;
		if (p_argcount > 0)
		{
			PyObject* args = PyTuple_New(p_argcount);
			for (int i = 0; i < p_argcount; ++i)
			{
				PyTuple_SetItem(args, i, gd2py(p_args[i]));
			}
			pRet = PyObject_Call(func, args, NULL);
		}
		else
		{
			pRet = PyObject_CallFunction(func, NULL);
		}

		Variant ret = py2gd(pRet);
		Py_XDECREF(pRet);
		Py_XDECREF(func);
		r_error.argument = 0;
		r_error.error = Variant::CallError::CALL_OK;
		return ret;
	}
	else*/
	//{
	int argc = get_py_func_argc(p_func);
	if (argc >= 0)
	{
		/*if (p_argcount > argc)
		{
			r_error.error = Variant::CallError::CALL_ERROR_TOO_MANY_ARGUMENTS;
			r_error.argument = argc;
			return Variant();
		}*/
		int defc = get_py_func_defc(p_func);
		if (p_argcount < argc - defc)
		{
			r_error.error = Variant::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS;
			r_error.argument = argc;
			return Variant();
		}
	}

	PyObject* args = PyTuple_New(p_argcount);
	for (int i = 0; i < p_argcount; ++i)
	{
		PyTuple_SetItem(args, i, gd2py(p_args[i]));
	}

	PyObject* pRet = PyObject_Call(p_func, args, p_kwargs);
	Variant ret = py2gd(pRet);
	Py_XDECREF(pRet);
	Py_XDECREF(args);
	r_error.argument = argc;
	r_error.error = Variant::CallError::CALL_OK;
	return ret;
	//}

	r_error.error = Variant::CallError::CALL_ERROR_INSTANCE_IS_NULL;
	return Variant();
}

PyObject* PyScript::func_gd2py(Ref<FuncRef> p_funcRef)
{
	PyObject* inst = PyLong_FromUnsignedLongLong(p_funcRef->get_instance_id());
	if (!inst)
		return NULL;
	PyObject* ret = PyCFunction_New(&gdFuncDef, inst);
	//PyObject_SetAttrString(inst, p_method.utf8().get_data(), m_obj);
	Py_DECREF(inst);
	return ret;
}

bool PyScript::_get(const StringName& p_name, Variant& r_ret) const
{
	if (!is_valid())
		return false;
	
	PyObject* mod = get_module();
	auto prop = String(p_name);
	PyObject* attr = NULL;
	if (PyObject_HasAttrString(mod, prop.utf8().get_data()))
	{
		attr = PyObject_GetAttrString(mod, prop.utf8().get_data());
		if (PyFunction_Check(attr) || PyInstanceMethod_Check(attr) || PyMethod_Check(attr))
		{
			Py_XDECREF(attr);
			attr = NULL;
		}
	}

	if (!attr)
		return false;

	r_ret = PyScript::py2gd(attr);
	Py_XDECREF(attr);
	return true;
}

bool PyScript::_set(const StringName& p_name, const Variant& p_value)
{
	if (!is_valid())
		return false;

	PyObject* mod = get_module();
	auto prop = String(p_name);
	auto propUtf8 = prop.utf8();
	PyObject* v = PyScript::gd2py(p_value);
	PyObject_SetAttrString(mod, propUtf8.get_data(), v);
	Py_XDECREF(v);
	return true;
	/*if (PyObject_HasAttrString(mod, propUtf8.get_data()))
	{
		PyObject* v = PyScript::gd2py(p_value);
		PyObject_SetAttrString(mod, propUtf8.get_data(), v);
		Py_XDECREF(v);
		return true;
	}
	PyObject* dict = PyObject_GenericGetDict(mod, NULL);
	if (!dict)
		return false;
	PyObject* v = PyScript::gd2py(p_value);
	PyDict_SetItemString(dict, propUtf8.get_data(), v);
	PyObject_GenericSetDict(mod, dict, NULL);
	Py_XDECREF(dict);
	Py_XDECREF(v);
	return true;*/
}

//void PyScript::_get_property_list(List<PropertyInfo>* p_properties) const
//{
//	print_error("_get_property_list");
//}

Variant PyScript::call(const StringName& p_method, const Variant** p_args, int p_argcount, Variant::CallError& r_error)
{
	if (!is_valid())
		return Script::call(p_method, p_args, p_argcount, r_error);

	PyObject* kwarg = NULL;
	auto method = p_method.operator String();
	if (p_method == "call_with_kwarg")
	{
		if (p_argcount < 2)
		{
			r_error.error = Variant::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS;
			r_error.argument = 0;
			return Variant();
		}

		if (p_args[p_argcount - 1]->get_type() != Variant::DICTIONARY)
		{
			r_error.error = Variant::CallError::CALL_ERROR_INVALID_ARGUMENT;
			r_error.argument = p_argcount - 1;
			r_error.expected = Variant::DICTIONARY;
			return Variant();
		}

		if (p_args[0]->get_type() != Variant::STRING)
		{
			r_error.error = Variant::CallError::CALL_ERROR_INVALID_ARGUMENT;
			r_error.argument = 0;
			r_error.expected = Variant::STRING;
			return Variant();
		}

		kwarg = gd2py(p_args[p_argcount - 1]);
		method = p_args[0]->operator String();
		p_argcount -= 2;
		++p_args;
	}

	PyObject* mod = get_module();
	Variant ret;
	if (PyObject_HasAttrString(mod, method.utf8().get_data()))
	{
		PyObject* func = PyObject_GetAttrString(mod, method.utf8().get_data());
		if (func)
		{
			ret = call_py_func(func, p_args, p_argcount, r_error, kwarg);
			Py_DECREF(func);
			if (r_error.error == Variant::CallError::CALL_OK)
			{
				Py_XDECREF(kwarg);
				return ret;
			}
		}
	}

	if ((method == "new" && PyType_Check(mod)) || (method == "call_self" && PyCallable_Check(mod)))
	{
		ret = call_py_func(mod, p_args, p_argcount, r_error, kwarg);
		if (r_error.error == Variant::CallError::CALL_OK)
		{
			Py_XDECREF(kwarg);
			return ret;
		}
	}
	Py_XDECREF(kwarg);

	if (r_error.error == Variant::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS ||
		r_error.error == Variant::CallError::CALL_ERROR_TOO_MANY_ARGUMENTS)
	{
		return Variant();
	}

	if (PyIter_Check(mod))
	{
		if (p_method == "get_next")
		{
			if (p_argcount > 0)
			{
				r_error.error = Variant::CallError::CALL_ERROR_TOO_MANY_ARGUMENTS;
				r_error.argument = 0;
				return Variant();
			}
			//PyObject* iter = PyObject_GetIter(mod);
			PyObject* pRet = PyIter_Next(mod);
			Variant ret = PyScript::py2gd(pRet);
			Py_XDECREF(pRet);
			//Py_DECREF(iter);
			r_error.argument = 0;
			r_error.error = Variant::CallError::CALL_OK;
			return ret;
		}
	}

	return Script::call(p_method, p_args, p_argcount, r_error);
}

void PyScript::_bind_methods()
{
	//ClassDB::bind_vararg_method(METHOD_FLAGS_DEFAULT, "new", &PyScript::_new, MethodInfo("new"));
	//ClassDB::bind_method(D_METHOD("dir"), &PyScript::dir);
}

inline String PyScript::get_module_name() const
{
	return m_moduleName;
	//if (!m_module || (!PyModule_Check(m_module) && !PyType_Check(m_module)))
	//	return String();

	//if (PyType_Check(m_module))
	//{
	//	return Py_TYPE(m_module)->tp_name;
	//}

	//PyObject* fno = PyModule_GetFilenameObject(m_module);
	//if (!fno || !PyUnicode_Check(fno))
	//	return String();

	//String ret = PyUnicode_AS_UNICODE(fno);
	//Py_DECREF(fno);
	//return ret;
}

inline PyObject* PyScript::get_module() const
{
	return m_obj;
}

void PyScript::set_module(PyObject* p_module)
{
	if (m_obj)
	{
		if (PyModule_Check(m_obj))
			Py_DECREF(m_obj);
		m_obj = NULL;
	}
	
	m_obj = p_module;
	if (m_obj)
	{
		if (PyType_Check(m_obj))
		{
			Py_XINCREF(p_module);
			m_moduleName = Py_TYPE(m_obj)->tp_name;
		}
		else if (PyModule_Check(m_obj))
		{
			Py_XINCREF(p_module);
			m_moduleName = PyModule_GetName(m_obj);
		}
		else
		{
			m_moduleName = "";
		}
	}
	else
	{
		m_moduleName = "";
	}
	set_name(m_moduleName);
}

Vector<PyScript::MethodData> PyScript::get_methods_data() const
{
	Vector<MethodData> ret;
	if (!is_valid())
		return ret;
	PyObject* mod = get_module();
	PyObject* dict = PyObject_GenericGetDict(mod, NULL);
	if (!dict)
		return ret;

	PyObject* key, * value;
	Py_ssize_t pos = 0;

	while (PyDict_Next(dict, &pos, &key, &value))
	{
		if (PyFunction_Check(value) || PyMethod_Check(value) || PyInstanceMethod_Check(value) || PyType_Check(value))
		{
			MethodData md;
			md.name = py2gd(key);
			md.argc = get_py_func_argc(value);
			ret.push_back(md);
		}
	}
	Py_XDECREF(dict);
	return ret;
}

Vector<String> PyScript::get_properties() const
{
	Vector<String> ret;
	if (!is_valid())
		return ret;
	PyObject* mod = get_module();
	PyObject* dict = PyObject_GenericGetDict(mod, NULL);
	if (!dict)
		return ret;

	PyObject* key, * value;
	Py_ssize_t pos = 0;

	while (PyDict_Next(dict, &pos, &key, &value))
	{
		if (PyFunction_Check(value) || PyMethod_Check(value) || PyInstanceMethod_Check(value))
		{
			continue;
		}

		ret.push_back(py2gd(key));
	}
	Py_XDECREF(dict);
	return ret;
}

void PyScript::free()
{
	Py_XDECREF(m_obj);
	m_obj = NULL;
}

//Variant PyScript::_new(const Variant** p_args, int p_argcount, Variant::CallError& r_error)
//{
//	Reference* owner = memnew(Reference);
//	ScriptInstance* inst = _instance_create(p_args, p_argcount, owner, r_error);
//	if (!inst)
//	{
//		print_error("[_new] create instance failed!");
//		memdelete(owner);
//		return Variant();
//	}
//	print_error("[_new] create instance success!");
//	return REF(owner);
//}

//ScriptInstance* PyScript::_instance_create(const Variant** p_args, int p_argcount, Object* p_owner, Variant::CallError& r_error)
//{
//	r_error.error = Variant::CallError::CALL_OK;
//	r_error.argument = 0;
//	if (!can_instance())
//	{
//		print_error("[_instance_crearte] can not instance");
//		return NULL;
//	}
//
//	PyScriptInstance* pyInst = memnew(PyScriptInstance);
//	PyObject* mod = get_module();
//	int argcount = 0;
//	if (PyObject_HasAttrString(mod, "__init__"))
//	{
//		PyObject* initMethod = PyObject_GetAttrString(mod, "__init__");
//		if (PyInstanceMethod_Check(initMethod))
//		{
//			argcount = get_py_func_argc(initMethod) - 1;
//		}
//		Py_XDECREF(initMethod);
//	}
//
//	if (p_argcount == argcount)
//	{
//		PyObject* args = PyTuple_New(argcount + 1);
//		for (int i = 0; i < argcount; ++i)
//		{
//			PyTuple_SET_ITEM(args, i + 1, gd2py(p_args[i]));
//		}
//		PyObject* kwd = NULL;
//
//		pyInst->m_obj = PyType_GenericNew(Py_TYPE(mod), args, kwd);
//		Py_XDECREF(args);
//		if (PyObject_HasAttrString(mod, "__init__"))
//		{
//			PyObject* initMethod = PyObject_GetAttrString(mod, "__init__");
//			if (PyInstanceMethod_Check(initMethod))
//			{
//				PyTuple_SET_ITEM(args, 0, pyInst->m_obj);
//				PyObject_Call(initMethod, args, kwd);
//				print_error("[_instance_crearte] call init");
//			}
//			
//			Py_XDECREF(initMethod);
//		}
//	}
//	else if (p_argcount < argcount)
//	{
//		r_error.error = Variant::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS;
//		memdelete(pyInst);
//		print_error("[_instance_crearte] CALL_ERROR_TOO_FEW_ARGUMENTS: " + Variant(argcount).operator String());
//		return NULL;
//	}
//	else
//	{
//		r_error.error = Variant::CallError::CALL_ERROR_TOO_MANY_ARGUMENTS;
//		print_error("[_instance_crearte] CALL_ERROR_TOO_MANY_ARGUMENTS: " + Variant(argcount).operator String());
//		memdelete(pyInst);
//		return NULL;
//	}
//
//	pyInst->m_owner = p_owner;
//	pyInst->m_script = Ref<PyScript>(this);
//	pyInst->m_owner->set_script_instance(pyInst);
//	return pyInst;
//}

void PyScript::set_path(const String& p_path, bool p_take_over)
{
	//String modName;
	//if (p_path.begins_with("py://"))
	//{
	//	modName = p_path.substr(5);
	//}
	//else
	//{
	//	modName = p_path;
	//}
	//if (modName.ends_with(".py"))
	//{
	//	modName = modName.substr(0, modName.length() - 3);
	//} else if (modName.ends_with(".pyc"))
	//{
	//	modName = modName.substr(0, modName.length() - 4);
	//}
	PyObject* mod = PyImport_ImportModule(p_path.utf8().get_data());
	set_module(mod);
	Py_XDECREF(mod);
}

//void PyScript::load_module(const String& p_module)
//{
//	Py_XDECREF(m_module);
//	m_module = PyImport_ImportModule(p_module.utf8().get_data());
//}

ScriptInstance* PyScript::instance_create(Object* p_this)
{
	/*if (!can_instance())
		return NULL;

	print_error("Instance_create");
	Variant::CallError unchecked_error;
	return _instance_create(NULL, 0, p_this, unchecked_error);*/
	return NULL;
}

bool PyScript::instance_has(const Object* p_this) const
{
	/*if (!can_instance())
		return false;*/
	PyScriptInstance* pyInst = dynamic_cast<PyScriptInstance*>(p_this->get_script_instance());

	return pyInst && pyInst->m_script->get_module_name() == get_module_name();
}

Error PyScript::reload(bool p_keep_state)
{
	PyObject* mod = get_module();
	if (!mod)
		return FAILED;

	if (PyModule_Check(mod))
	{
		m_obj = PyImport_ReloadModule(mod);
		Py_XDECREF(mod);
		if (!m_obj)
		{
			m_moduleName = "";
			set_name("");
			//print_error("Reload failed!");
			return FAILED;
		}
		//print_error("Reload success");
	}		
	/*else if (PyType_Check(mod))
	{
		PyTypeObject* tp = Py_TYPE(mod);
		PyObject* mmod = PyType_GetModule(tp);
		if (!mmod)
			return FAILED;
		PyObject* newMod = PyImport_ReloadModule(mod);
		if (!newMod)
			return FAILED;
		Py_XDECREF(newMod);
	}*/
	return OK;
}

bool PyScript::has_method(const StringName& p_method) const
{
	if (!is_valid())
		return false;
	PyObject* mod = get_module();
	PyObject* dict = PyObject_GenericGetDict(mod, NULL);
	auto utf8 = p_method.operator String().utf8();
	PyObject* methName = PyUnicode_FromStringAndSize(utf8.get_data(), utf8.length());
	bool ret = false;
	if (dict && PyDict_Contains(dict, methName))
	{
		PyObject* meth = PyObject_GenericGetAttr(mod, methName);
		ret = meth && (PyFunction_Check(meth) || PyInstanceMethod_Check(meth) || PyMethod_Check(meth) || PyType_Check(meth));
		Py_XDECREF(meth);
	}
	Py_XDECREF(dict);
	Py_DECREF(methName);
	return ret;
}

MethodInfo PyScript::get_method_info(const StringName& p_method) const
{
	if (!is_valid())
		return MethodInfo();
	PyObject* mod = get_module();
	String method = String(p_method);
	auto utf8 = method.utf8();
	PyObject* funcName = PyUnicode_FromStringAndSize(utf8.get_data(), utf8.length());
	PyObject* func = PyObject_GenericGetAttr(mod, funcName);
	Py_DECREF(funcName);
	if (!func || (!PyFunction_Check(func) && !PyMethod_Check(func) && !PyInstanceMethod_Check(func) && !PyType_Check(func)))
	{
		Py_XDECREF(func);
		return MethodInfo();
	}

	MethodInfo mi(p_method);
	int argc = get_py_func_argc(func);
	for (int i = 0; i < argc; ++i)
	{
		mi.arguments.push_back(PropertyInfo());
	}

	Py_XDECREF(func);
	return mi;
}

bool PyScript::is_tool() const
{
	return false;
}

bool PyScript::is_valid() const
{
	return get_module();
}

bool PyScript::get_property_default_value(const StringName& p_property, Variant& r_value) const
{
	return false;
}

void PyScript::get_script_method_list(List<MethodInfo>* p_list) const
{
	if (!p_list || !is_valid())
		return;

	auto methodData = get_methods_data();
	for (int i = 0; i < methodData.size(); ++i)
	{
		MethodInfo mi(methodData[i].name);
		for (int j = 0; j < methodData[i].argc; ++j)
		{
			mi.arguments.push_back(PropertyInfo());
		}
		p_list->push_back(mi);
	}
}

void PyScript::get_script_property_list(List<PropertyInfo>* p_list) const
{
	if (!p_list || !is_valid())
		return;

	auto prop = get_properties();
	for (int i = 0; i < prop.size(); ++i)
	{
		PropertyInfo pi;
		pi.name = prop[i];
		p_list->push_back(pi);
	}
}

void PyScript::get_constants(Map<StringName, Variant>* p_constants)
{
	/*if (!p_constants || !is_valid())
		return;

	print_error("get_constants");
	PyObject* mod = get_module();
	PyObject* attrs = NULL;
	if (PyModule_Check(mod))
	{
		attrs = PyModule_GetDict(mod);
	}
	else if (PyType_Check(mod))
	{
		attrs = Py_TYPE(mod)->tp_dict;
	}
	if (!attrs)
		return;

	PyObject* key, * value;
	Py_ssize_t pos = 0;

	while (PyDict_Next(attrs, &pos, &key, &value))
	{
		if (PyCallable_Check(value))
		{
			continue;
		}

		p_constants->insert(String(PyUnicode_AS_UNICODE(key)), py2gd(value));
	}*/
}

void PyScript::get_members(Set<StringName>* p_members)
{
	if (!p_members || !is_valid())
		return;

	auto prop = get_properties();
	for (int i = 0; i < prop.size(); ++i)
	{
		p_members->insert(prop[i]);
	}
}

PyScript::PyScript()
{	
}

PyScript::~PyScript()
{
	if (m_obj)
	{
		if (PyModule_Check(m_obj))
			Py_DECREF(m_obj);
		m_obj = NULL;
	}
}
	

bool PyScriptInstance::set(const StringName& p_name, const Variant& p_value)
{
	if (!is_valid())
		return false;

	PyObject* obj = get_py_obj();
	auto prop = String(p_name);
	auto propUtf8 = prop.utf8();
	PyObject* v = PyScript::gd2py(p_value);
	PyObject_SetAttrString(obj, propUtf8.get_data(), v);
	Py_XDECREF(v);
	return true;
	/*if (PyObject_HasAttrString(obj, propUtf8.get_data()))
	{
		PyObject* v = PyScript::gd2py(p_value);
		PyObject_SetAttrString(obj, propUtf8.get_data(), v);
		Py_XDECREF(v);
		return true;
	}
	PyObject* dict = PyObject_GenericGetDict(obj, NULL);
	if (!dict)
		return false;
	PyObject* v = PyScript::gd2py(p_value);
	PyDict_SetItemString(dict, propUtf8.get_data(), v);
	PyObject_GenericSetDict(obj, dict, NULL);
	Py_XDECREF(dict);
	Py_XDECREF(v);
	return true;*/
}

bool PyScriptInstance::get(const StringName& p_name, Variant& r_ret) const
{
	auto na = String(p_name);
	auto utf8 = na.utf8();
	PyObject* mod = m_script->get_module();
	PyObject* obj = get_py_obj();
	if (!obj || !mod)
		return false;

	auto prop = String(p_name);
	PyObject* attr = NULL;
	if (PyObject_HasAttrString(obj, prop.utf8().get_data()))
	{
		attr = PyObject_GetAttrString(obj, prop.utf8().get_data());
		if (PyFunction_Check(attr) || PyInstanceMethod_Check(attr) || PyMethod_Check(attr))
		{
			Py_XDECREF(attr);
			attr = NULL;
		}
	}

	//PyObject* attrName = PyUnicode_FromStringAndSize(utf8.get_data(), utf8.length());
	//PyObject* dict = PyObject_GenericGetDict(obj, NULL);
	//if (dict)
	//{
	//	if (PyDict_Contains(dict, attrName))
	//	{
	//		attr = PyObject_GenericGetAttr(obj, attrName);
	//		if (PyFunction_Check(attr) || PyInstanceMethod_Check(attr) || PyMethod_Check(attr))
	//		{
	//			Py_XDECREF(attr);
	//			attr = NULL;
	//		}
	//	}
	//}
	//Py_XDECREF(dict);
	//Py_DECREF(attrName);
	if (!attr)
		return false;

	r_ret = PyScript::py2gd(attr);
	Py_XDECREF(attr);
	return true;
}

void PyScriptInstance::get_property_list(List<PropertyInfo>* p_properties) const
{
	if (!is_valid())
		return;

	PyObject* obj = get_py_obj();
	PyObject* attrs = PyObject_GenericGetDict(obj, NULL);
	if (!attrs)
	{
		return;
	}
	PyObject* key, * value;
	Py_ssize_t pos = 0;

	while (PyDict_Next(attrs, &pos, &key, &value))
	{
		if (PyFunction_Check(value) || PyMethod_Check(value) || PyInstanceMethod_Check(value))
		{
			continue;
		}

		PropertyInfo pi;
		pi.name = PyScript::py2gd(key);
		p_properties->push_back(pi);
	}
}

Variant::Type PyScriptInstance::get_property_type(const StringName& p_name, bool* r_is_valid) const
{
	if (r_is_valid)
		*r_is_valid = false;
	return Variant::NIL;
}

void PyScriptInstance::get_method_list(List<MethodInfo>* p_list) const
{
	if (!is_valid())
		return;

	p_list->push_back(MethodInfo("call_with_kwarg"));

	PyObject* obj = get_py_obj();
	PyObject* attrs = PyObject_GenericGetDict(obj, NULL);
	if (!attrs)
	{
		return;
	}

	PyObject* key, * value;
	Py_ssize_t pos = 0;
	while (PyDict_Next(attrs, &pos, &key, &value))
	{
		if (PyFunction_Check(value) || PyMethod_Check(value) || PyInstanceMethod_Check(value) || PyType_Check(value))
		{
			MethodInfo mi(PyScript::py2gd(key));
			int argc = PyScript::get_py_func_argc(value);
			for (int i = 0; i < argc; ++i)
			{
				mi.arguments.push_back(PropertyInfo());
			}
			p_list->push_back(mi);
		}
	}
	Py_XDECREF(attrs);
}

bool PyScriptInstance::has_method(const StringName& p_method) const
{
	if (!is_valid())
		return false;

	if (p_method == "call_with_kwarg")
		return true;

	PyObject* obj = get_py_obj();
	auto method = String(p_method);
	bool ret = false;
	
	if (PyObject_HasAttrString(obj, method.utf8().get_data()))
	{
		PyObject* attr = PyObject_GetAttrString(obj, method.utf8().get_data());
		ret = attr && (PyFunction_Check(attr) || PyMethod_Check(attr) || PyInstanceMethod_Check(attr) || PyType_Check(attr));
		Py_XDECREF(attr);
	}
	if (!ret && PyIter_Check(obj))
	{
		if (p_method == "next")
			ret = true;
	}
	return ret;
}

Variant PyScriptInstance::call(const StringName& p_method, const Variant** p_args, int p_argcount, Variant::CallError& r_error)
{
	if (!is_valid())
	{
		r_error.error = Variant::CallError::CALL_ERROR_INVALID_METHOD;
		return Variant();
	}

	PyObject* kwarg = NULL;
	auto method = p_method.operator String();
	if (p_method == "call_with_kwarg")
	{
		if (p_argcount < 2)
		{
			r_error.error = Variant::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS;
			r_error.argument = 0;
			return Variant();
		}

		if (p_args[p_argcount - 1]->get_type() != Variant::DICTIONARY)
		{
			r_error.error = Variant::CallError::CALL_ERROR_INVALID_ARGUMENT;
			r_error.argument = p_argcount - 1;
			r_error.expected = Variant::DICTIONARY;
			return Variant();
		}

		if (p_args[0]->get_type() != Variant::STRING)
		{
			r_error.error = Variant::CallError::CALL_ERROR_INVALID_ARGUMENT;
			r_error.argument = 0;
			r_error.expected = Variant::STRING;
			return Variant();
		}

		PyObject* kwarg = PyScript::gd2py(p_args[p_argcount - 1]);
		method = p_args[0]->operator String();
		p_argcount -= 2;
		++p_args;
	}

	PyObject* obj = get_py_obj();
	
	Variant ret;
	if (PyObject_HasAttrString(obj, method.utf8().get_data()))
	{
		PyObject* func = PyObject_GetAttrString(obj, method.utf8().get_data());
		if (func)
		{
			ret = PyScript::call_py_func(func, p_args, p_argcount, r_error, kwarg);
			Py_DECREF(func);
			if (r_error.error == Variant::CallError::CALL_OK)
			{
				Py_XDECREF(kwarg);
				return ret;
			}
		}
	}

	if ((method == "new" && PyType_Check(obj)) || (method == "call_self" && PyCallable_Check(obj)))
	{
		ret = PyScript::call_py_func(obj, p_args, p_argcount, r_error, kwarg);
		if (r_error.error == Variant::CallError::CALL_OK)
		{
			Py_XDECREF(kwarg);
			return ret;
		}
	}

	Py_XDECREF(kwarg);

	if (r_error.error == Variant::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS ||
		r_error.error == Variant::CallError::CALL_ERROR_TOO_MANY_ARGUMENTS)
	{
		return Variant();
	}

	
	if (PyIter_Check(obj))
	{
		if (p_method == "get_next")
		{
			if (p_argcount > 0)
			{
				r_error.error = Variant::CallError::CALL_ERROR_TOO_MANY_ARGUMENTS;
				r_error.argument = 0;
				return Variant();
			}
			//PyObject* iter = PyObject_GetIter(obj);
			PyObject* pRet = PyIter_Next(obj);
			Variant ret = PyScript::py2gd(pRet);
			Py_XDECREF(pRet);
			//Py_DECREF(iter);
			r_error.argument = 0;
			r_error.error = Variant::CallError::CALL_OK;
			return ret;
		}
	}
	r_error.error = Variant::CallError::CALL_ERROR_INVALID_METHOD;
	r_error.argument = 0;
	return Variant();
}

String PyScriptInstance::to_string(bool* r_valid)
{
	if (m_script.is_null())
	{
		if (r_valid)
			*r_valid = false;
		return String();
	}
	if (r_valid)
		*r_valid = true;
	return "[PyScriptInstance(" + Python::get_singleton()->_str(m_obj) + "):" + Variant(m_owner->get_instance_id()) + "]";
}

Ref<Script> PyScriptInstance::get_script() const
{
	return m_script;
}

MultiplayerAPI::RPCMode PyScriptInstance::get_rpc_mode(const StringName& p_method) const
{
	return MultiplayerAPI::RPC_MODE_DISABLED;
}

MultiplayerAPI::RPCMode PyScriptInstance::get_rset_mode(const StringName& p_variable) const
{
	return MultiplayerAPI::RPC_MODE_DISABLED;
}

void PyScriptInstance::set_py_obj(PyObject* p_obj)
{
	free();
	m_obj = p_obj;
}

PyScriptInstance& PyScriptInstance::operator =(const PyScriptInstance& p_rhs)
{
	set_py_obj(p_rhs.m_obj);
	Py_XINCREF(m_obj);
	return *this;
}

PyScriptInstance::PyScriptInstance()
{
}

PyScriptInstance::~PyScriptInstance()
{
	free();
}
