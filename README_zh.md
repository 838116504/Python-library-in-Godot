# ����Y��Python��
�ṩgdscript�Y��Python�칦�ܵĸ��ģ�顣

# ���b
�ҵ�ϵ�y��Windows7�� �yԇ�õĸ��汾��3.2.3-stable�� Python�汾��3.7.9��

��**python37.lib**, **modules**�ļ��A��**thirdparty**�ļ��A��������Դ�a�ĸ�Ŀ䛡�

Ȼ���g, [���ٷ����g�ęn](https://docs.godotengine.org/en/stable/development/compiling/index.html)

��Ҫ��**bin/python37.dll**���ڸ��EXE�ļ���ͬĿ䛡�

# �������汾��Python
��ԇ�^ֱ�Ӱ�CPython��Դ��GȥGodot��һ���g������̫���ҷŗ��ˡ�

����������������python.org�������d���bPython�������F�ɵ�dll, lib��ͷ�ļ����á�

ͷ�ļ���Python���bĿ䛵�includeĿ��Y�������ļ��A�}�uȥ���Դ�a��**thirdparty/cpython/**Ŀ䛡�

lib�ļ���Python���bĿ䛵�libsĿ��Y��ֻ��Ҫ�}�u**pythonXY.lib**�ǂ������Դ�a�ĸ�Ŀ䛡�

dll�ļ���Python���bĿ䛵�**pythonXX.dll**�������}�u�����EXE�ļ�Ŀ��¡�

Ȼ���g, [���ٷ����g�ęn](https://docs.godotengine.org/en/stable/development/compiling/index.html)

# ��֪Bug
Python������O�ò��������|�lĳЩ�O�÷����Ę���(Python�ęC���Ҳ���)����������Ĵ��a
```
obj.a2 = 99
```
��ֻ���O��obj.__dict__[a2]��99�Ę��ӡ�

# �÷�
�xȡPython��

```
var pyScript = PyScript.new()
pyScript.set_path("openpyxl.workbook")	# openpyxl�����Լ����b�İ�
if pyScript.get_name() != "":
	print("load openpyxl.workbook module success")
```

Note: set_path���������ı�����PyScript��·����_��·��Ψһ��

�{�ú�������������(�{��class�ஔ��{�Ø�������)

```
var obj = pyScript.classA()
var ret = obj.func1(arg)
```

�O�úͫ@ȡ����

```
obj.a = "test"
print(obj.a)
```

# ԭ��
�����PyScript���ʾPythonģ����࣬��������cpython������ָᘣ�Ȼ���Ϊ�{�÷������@ȡ/�O�Ì��Ե����g�ˡ�

PyScript.set_path�����{��cpython��APIȥ����ģ�����@ȡָᘣ�ģ�����ęC�ƺ�python��đ�ԓ��ͬ��

Python�Č�����PyScriptInstance��ʾ����PyScriptͬ��

���ֵ��ͺ�Python��ֵ��̓Ȳ��Ԅ��D�Q�������ġ�