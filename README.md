[中文readme](README_zh.md)

# Python library in Godot
 Godot module for use Python libray on gdscript.

# Installing
My system is Windows7. The Godot's verson that test use is 3.2.3-stable. Python version is 3.7.9.

Put **python37.lib**, **modules** folder and **thirdparty** folder to your Godot source root directory.

Then compile, [Godot compile docs](https://docs.godotengine.org/en/stable/development/compiling/index.html)

It need put the **bin/python37.dll** to the directory where the Godot executable file is located.

# Use orther python version
I tried to make the cpython source together, I feel too many things need set. I gave up.

I used another method, get the dll, lib and h file from python.org, so need dowload this first.

Install the python and go to the folder you install. Copy the **include** folder to **thirdparty/cpython/**. Copy the **libs/pythonXY.lib** to Godot source root directory. Copy the **pythonXY.dll** to the the directory where the Godot executable file is located.

Then compile, [Godot compile docs](https://docs.godotengine.org/en/stable/development/compiling/index.html)

# Bug known
Python object set operate can not trigger something(I don't know the python mechanism), such as
```
obj.a2 = 99
```
It only set the obj.__dict__[a2] to 99.

# Usage
Load python libray.

```
var pyScript = PyScript.new()
pyScript.set_path("openpyxl.workbook")	# openpyxl is  a package that I installed
if pyScript.get_name() != "":
	print("load openpyxl.workbook module success")
```

Note: The set_path not changed other PyScript's path that same to guaranteed the path unique.

Call function, method or class(call class = new object)

```
var obj = pyScript.classA()
var ret = obj.func1(arg)
```

Set and get property

```
obj.a = "test"
print(obj.a)
```

# Principle
Python module and class I create PyScript to hold it and interpretation between gdodt and cpython.

PyScript.set_path method to call cpython API to load python library.

Python object I create PyScriptInstance to hold it and interpretation between gdodt and cpython.

Python int, float, string, list, etc value type will auto conversion to godot value type.