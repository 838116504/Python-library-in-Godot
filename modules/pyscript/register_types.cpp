#include <core/class_db.h>
#include "register_types.h"
#include "pyscript.h"
#include "core/os/os.h"

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
	ClassDB::register_class<PyScript>();
}

void unregister_pyscript_types()
{
	py_deinit();
}
