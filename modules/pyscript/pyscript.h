#ifndef PYTHON_LIB_H
#define PYTHON_LIB_H

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "core/script_language.h"

class PyScript;
class PyScriptInstance;

class Python : public Object
{
	GDCLASS(Python, Object);

private:
	static Python* singleton;

protected:
	static void _bind_methods();

public:
	Variant dir(const Variant& p_obj) const;

	static Python* get_singleton() { return singleton; };

	Python() { singleton = this; };
	~Python() {};
};

class PyScript : public Script
{
	GDCLASS(PyScript, Script);
	friend class Python;

private:
	String m_moduleName;
	PyObject* m_obj = NULL;

protected:
	bool _get(const StringName& p_name, Variant& r_ret) const;
	bool _set(const StringName& p_name, const Variant& p_value);
	//void _get_property_list(List<PropertyInfo>* p_properties) const;

	Variant call(const StringName& p_method, const Variant** p_args, int p_argcount, Variant::CallError& r_error);

	static void _bind_methods();
public:
	static Variant py2gd(PyObject* p_source);
	static PyObject* gd2py(const Variant* p_source);
	static PyObject* gd2py(const Variant& p_source);
	static int get_pyFunc_argc(PyObject* p_func);
	static int get_pyFunc_defc(PyObject* p_func);

	String get_module_name() const;
	PyObject* get_module() const;
	void set_module(PyObject* p_module);
	typedef struct MethodData {
		String name;
		int argc = 0;
	}MethodData;
	Vector<MethodData> get_methods_data() const;
	Vector<String> get_properties() const;

	/*void load_module(const String& p_module);*/

	/*Variant _new(const Variant** p_args, int p_argcount, Variant::CallError& r_error);
	ScriptInstance* _instance_create(const Variant** p_args, int p_argcount, Object* p_owner, Variant::CallError& r_error);*/

	virtual void set_path(const String& p_path, bool p_take_over = false);

	virtual bool can_instance() const { return false; };

	virtual Ref<Script> get_base_script() const { return Ref<Script>(); }; //for script inheritance

	virtual StringName get_instance_base_type() const { return StringName(); }; // this may not work in all scripts, will return empty if so
	virtual ScriptInstance* instance_create(Object* p_this);
	virtual bool instance_has(const Object* p_this) const;

	virtual bool has_source_code() const { return false; };
	virtual String get_source_code() const { return String(); };
	virtual void set_source_code(const String& p_code) {};
	virtual Error reload(bool p_keep_state = false);

	virtual bool has_method(const StringName& p_method) const;
	virtual MethodInfo get_method_info(const StringName& p_method) const;

	virtual bool is_tool() const;
	virtual bool is_valid() const;

	virtual ScriptLanguage* get_language() const { return NULL; };

	virtual bool has_script_signal(const StringName& p_signal) const { return false; };
	virtual void get_script_signal_list(List<MethodInfo>* r_signals) const {};

	virtual bool get_property_default_value(const StringName& p_property, Variant& r_value) const;

	virtual void get_script_method_list(List<MethodInfo>* p_list) const;
	virtual void get_script_property_list(List<PropertyInfo>* p_list) const;

	virtual void get_constants(Map<StringName, Variant>* p_constants);
	virtual void get_members(Set<StringName>* p_members);

	PyScript();
	~PyScript();
};

class PyScriptInstance : public ScriptInstance
{
	friend class PyScript;
	friend class Python;
private:
	Ref<PyScript> m_script;
	Object* m_owner = NULL;
	PyObject* m_obj = NULL;

protected:

public:
	virtual Object* get_owner() { return m_owner; }
	inline PyObject* get_py_obj() const { return m_obj; }

	virtual bool set(const StringName& p_name, const Variant& p_value);
	virtual bool get(const StringName& p_name, Variant& r_ret) const;
	virtual void get_property_list(List<PropertyInfo>* p_properties) const;
	virtual Variant::Type get_property_type(const StringName& p_name, bool* r_is_valid = NULL) const;

	virtual void get_method_list(List<MethodInfo>* p_list) const;
	virtual bool has_method(const StringName& p_method) const;
	virtual Variant call(const StringName& p_method, const Variant** p_args, int p_argcount, Variant::CallError& r_error);
	virtual void notification(int p_notification) {};
	virtual String to_string(bool* r_valid);

	virtual Ref<Script> get_script() const;

	virtual MultiplayerAPI::RPCMode get_rpc_mode(const StringName& p_method) const;
	virtual MultiplayerAPI::RPCMode get_rset_mode(const StringName& p_variable) const;

	virtual ScriptLanguage* get_language() { return NULL; };

	PyScriptInstance();
	~PyScriptInstance();
};

#endif
