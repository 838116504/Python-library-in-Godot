#include <core/class_db.h>
#include "register_types.h"
#include "pyscript.h"
#include "core/os/os.h"

Python* python = NULL;

void py_init()
{
	if (Py_IsInitialized())
		return;

	Py_SetProgramName(OS::get_singleton()->get_executable_path().ptr());
	Py_Initialize();
	if (!Py_IsInitialized())
		print_error("Python init failed.");
}

void py_deinit()
{
	if (!Py_IsInitialized())
		return;
	Py_Finalize();
}

void register_pyscript_types()
{
	py_init();
	python = memnew(Python);
	Engine::get_singleton()->add_singleton(Engine::Singleton("Python", Python::get_singleton()));
	ClassDB::register_class<PyScript>();
}

void unregister_pyscript_types()
{
	memdelete(python);
	py_deinit();
}
