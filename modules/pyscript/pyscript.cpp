#include "pyscript.h"
#include "core/os/file_access.h"

inline PyObject* PyScript::gd2py(const Variant& p_source)
{
	return gd2py(&p_source);
}

PyObject* PyScript::gd2py(const Variant* p_source)
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
			Ref<PyScript> s(p_source->operator RefPtr());
			if (s.is_valid())
			{
				auto mod = s->get_module();
				if (mod)
				{
					return mod;
				}
			}
		}
		Object* obj = p_source->operator Object * ();
		if (obj && obj->get_script_instance())
		{
			PyScriptInstance* inst = dynamic_cast<PyScriptInstance*>(obj->get_script_instance());
			if (inst)
			{
				auto obj = inst->get_py_obj();
				if (obj)
				{
					return obj;
				}
			}
		}
	} break;
	case Variant::DICTIONARY:
	{
		PyObject* pyDict = PyDict_New();
		Dictionary dict = p_source->operator Dictionary();
		for (int i = 0; i < dict.size(); ++i)
		{
			PyObject* pyKey = gd2py(dict.keys()[i]);
			if (pyKey != Py_None)
			{
				PyDict_SetItem(pyDict, pyKey, gd2py(dict[dict.keys()[i]]));
				Py_XDECREF(pyKey);
			}	
		}
		return pyDict;
	}
	case Variant::ARRAY:
	{
		Array a = p_source->operator Array();
		PyObject* pyList = PyList_New(a.size());
		for (int i = 0; i < a.size(); ++i)
		{
			PyList_SET_ITEM(pyList, i, gd2py(a[i]));
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
			PyList_SET_ITEM(pyList, i, gd2py(a[i]));
		}
		return pyList;
	}
	case Variant::POOL_REAL_ARRAY:
	{
		PoolRealArray a = p_source->operator PoolRealArray();
		PyObject* pyList = PyList_New(a.size());
		for (int i = 0; i < a.size(); ++i)
		{
			PyList_SET_ITEM(pyList, i, gd2py(a[i]));
		}
		return pyList;
	}
	case Variant::POOL_STRING_ARRAY:
	{
		PoolStringArray a = p_source->operator PoolStringArray();
		PyObject* pyList = PyList_New(a.size());
		for (int i = 0; i < a.size(); ++i)
		{
			PyList_SET_ITEM(pyList, i, gd2py(a[i]));
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
		return PyUnicode_AS_UNICODE(p_source);
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

int PyScript::get_pyFunc_argc(PyObject* p_func)
{
	if (!PyCallable_Check(p_func))
		return -1;

	int keep = 0;
	if (PyInstanceMethod_Check(p_func))
	{
		p_func = PyInstanceMethod_GET_FUNCTION(p_func);
		keep = 1;
	}
	else if (PyMethod_Check(p_func))
	{
		p_func = PyMethod_GET_FUNCTION(p_func);
		keep = 1;
	}

	if (PyFunction_Check(p_func))
	{
		auto co = (PyCodeObject*)PyFunction_GetCode(p_func);
		return co->co_argcount + co->co_kwonlyargcount - keep;
	}
	return -1;
}

int PyScript::get_pyFunc_defc(PyObject* p_func)
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
			return PyObject_Size(co);
		}
	}
	return 0;
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
	PyObject* mod = get_module();
	auto method = p_method.operator String();
	if (mod && PyObject_HasAttrString(mod, method.utf8().get_data()))
	{
		PyObject* func = PyObject_GetAttrString(mod, method.utf8().get_data());
		if (PyFunction_Check(func) || PyMethod_Check(func))
		{
			int argc = get_pyFunc_argc(func);
			if (p_argcount > argc)
			{
				Py_XDECREF(func);
				r_error.error = Variant::CallError::CALL_ERROR_TOO_MANY_ARGUMENTS;
				r_error.argument = argc;
				return Variant();
			}
			int defc = get_pyFunc_defc(func);
			if (p_argcount < argc - defc)
			{
				Py_XDECREF(func);
				r_error.error = Variant::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS;
				r_error.argument = argc;
				return Variant();
			}
			PyObject* args = PyTuple_New(p_argcount);
			for (int i = 0; i < p_argcount; ++i)
			{
				PyTuple_SetItem(args, i, gd2py(p_args[i]));
			}
			PyObject* pRet = PyObject_Call(func, args, NULL);
			Variant ret = py2gd(pRet);
			Py_XDECREF(pRet);
			Py_XDECREF(func);
			r_error.argument = argc;
			r_error.error = Variant::CallError::CALL_OK;
			return ret;
		}
		else if (PyType_Check(func))
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
		
		Py_XDECREF(func);
	}
	return Script::call(p_method, p_args, p_argcount, r_error);
}

void PyScript::_bind_methods()
{
	//ClassDB::bind_vararg_method(METHOD_FLAGS_DEFAULT, "new", &PyScript::_new, MethodInfo("new"));
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
			md.name = PyUnicode_AS_UNICODE(key);
			md.argc = get_pyFunc_argc(value);
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
		String na = PyUnicode_AS_UNICODE(key);

		ret.push_back(na);
	}
	Py_XDECREF(dict);
	return ret;
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
//			argcount = get_pyFunc_argc(initMethod) - 1;
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
	int argc = get_pyFunc_argc(func);
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
	PyObject* mod = m_script->get_module();
	PyObject* obj = get_py_obj();
	if (!obj || !mod)
		return false;

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
	PyObject* obj = get_py_obj();
	PyObject* mod = m_script->get_module();
	if (!obj || !mod)
	{
		return;
	}

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
		pi.name = PyUnicode_AS_UNICODE(key);
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
	PyObject* obj = get_py_obj();
	PyObject* mod = m_script->get_module();
	if (!obj || !mod)
	{
		return;
	}

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
			MethodInfo mi(PyUnicode_AS_UNICODE(key));
			int argc = PyScript::get_pyFunc_argc(value);
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
	PyObject* mod = m_script->get_module();
	PyObject* obj = get_py_obj();
	if (!mod || !obj)
		return false;

	auto method = String(p_method);
	bool ret = false;
	if (PyObject_HasAttrString(obj, method.utf8().get_data()))
	{
		PyObject* attr = PyObject_GetAttrString(obj, method.utf8().get_data());
		ret = attr && (PyFunction_Check(attr) || PyMethod_Check(attr) || PyInstanceMethod_Check(attr) || PyType_Check(attr));
		Py_XDECREF(attr);
	}
	return ret;
}

Variant PyScriptInstance::call(const StringName& p_method, const Variant** p_args, int p_argcount, Variant::CallError& r_error)
{
	PyObject* obj = get_py_obj();
	PyObject* mod = m_script->get_module();
	if (obj && mod)
	{
		String method = p_method.operator String();
		PyObject* func = NULL;
		if (PyObject_HasAttrString(obj, method.utf8().get_data()))
		{
			func = PyObject_GetAttrString(obj, method.utf8().get_data());
		}
		if (func)
		{
			if (PyFunction_Check(func) || PyMethod_Check(func) || PyInstanceMethod_Check(func))
			{
				PyObject* args;
				int argc = PyScript::get_pyFunc_argc(func);
				if (p_argcount > argc)
				{
					Py_XDECREF(func);
					r_error.error = Variant::CallError::CALL_ERROR_TOO_MANY_ARGUMENTS;
					r_error.argument = argc;
					return Variant();
				}
				
				int defc = PyScript::get_pyFunc_defc(func);
				if (p_argcount < argc - defc)
				{
					Py_XDECREF(func);
					r_error.error = Variant::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS;
					r_error.argument = argc;
					return Variant();
				}

				args = PyTuple_New(p_argcount);
				for (int i = 0; i < p_argcount; ++i)
				{
					PyTuple_SetItem(args, i, PyScript::gd2py(p_args[i]));
				}
				PyObject* pRet = NULL;
				PyObject* meth = func;
				bool freeMeth = false;
				/*if (PyFunction_Check(func))
				{
					meth = PyMethod_New(func, obj);
					freeMeth = true;
					print_error("[call] Function: " + p_method);
				}
				else */if (PyInstanceMethod_Check(func))
				{
					meth = PyMethod_New(PyInstanceMethod_GET_FUNCTION(func), obj);
					freeMeth = true;
					//print_error("[call] InstanceMethod: " + p_method);
				}
				/*else
				{
					print_error("[call] Method: " + p_method);
				}*/

				if (meth)
					pRet = PyObject_Call(meth, args, NULL);
				if (freeMeth)
				{
					Py_XDECREF(meth);
				}				

				Variant ret = PyScript::py2gd(pRet);
				Py_XDECREF(pRet);
				Py_XDECREF(func);
				r_error.error = Variant::CallError::CALL_OK;
				r_error.argument = 0;
				return ret;
			}
			else if (PyType_Check(func))
			{
				PyObject* args = PyTuple_New(p_argcount);
				for (int i = 0; i < p_argcount; ++i)
				{
					PyTuple_SetItem(args, i, PyScript::gd2py(p_args[i]));
				}
				PyObject* pRet = PyObject_Call(func, args, NULL);
				Variant ret = PyScript::py2gd(pRet);
				Py_XDECREF(pRet);
				Py_XDECREF(func);
				r_error.argument = 0;
				r_error.error = Variant::CallError::CALL_OK;
				return ret;
			}
			
			Py_XDECREF(func);
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
	return "[py." + m_script->get_module_name() + ":" + Variant(m_owner->get_instance_id()) + "]";
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

PyScriptInstance::PyScriptInstance()
{
}

PyScriptInstance::~PyScriptInstance()
{
	Py_XDECREF(m_obj);
	m_obj = NULL;
}
