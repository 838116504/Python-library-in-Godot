# 戈多裏用Python庫
提供gdscript裏用Python庫功能的戈多模块。

# 安裝
我的系統是Windows10。 測試用的戈多版本是3.3-stable。 Python版本是3.9.5。

把**python39.lib**, **modules**文件夾和**thirdparty**文件夾放在你戈多源碼的根目錄。

然后編譯, [戈多官方編譯文檔](https://docs.godotengine.org/en/stable/development/compiling/index.html)

需要把**bin/python39.dll**放在戈多EXE文件相同目錄。

# 用其他版本的Python
我試過直接把CPython的源码丟去Godot里一起編譯，问题太多我放棄了。

我用其他方法，從python.org那里下載安裝Python，拿它現成的dll, lib和头文件來用。

头文件在Python安裝目錄的include目錄裏，整個文件夾複製去戈多源碼的**thirdparty/cpython**目錄。

lib文件在Python安裝目錄的libs目錄裏，只需要複製**pythonXY.lib**那個到戈多源碼的根目錄。

dll文件是Python安裝目錄的**pythonXX.dll**，把它複製到戈多EXE文件目錄下。

然后編譯, [戈多官方編譯文檔](https://docs.godotengine.org/en/stable/development/compiling/index.html)

# 已知Bug
Python對象的設置操作不能觸發某些設置方法的樣子(Python的機制我不懂)，比如下面的代碼
```
obj.a2 = 99
```
它只會設置obj._ _dict_ _[a2]成99的樣子。

# 用法
讀取Python庫

```
var pyScript = PyScript.new()
pyScript.set_path("openpyxl.workbook")	# openpyxl是我自己安裝的包
if pyScript.get_name() != "":
	print("load openpyxl.workbook module success")
```

Note: set_path方法不會改变其他PyScript的路徑來確保路徑唯一。

調用函數、方法或类(調用class相當於調用構建函數)

```
var obj = pyScript.classA()
var ret = obj.func1("arg1")
var ret2 = obj.call_with_kwarg("func1", "arg1", {"arg2":"arg2"})
```

Note: call_with_kwarg不是obj的方法，它是帶kwargs參數的調用，第一個參數是方法名，最後是kwargs。

設置和獲取屬性

```
obj.a = "test"
print(obj.a)
```

獲取iter的下一個

```
var next = Python.next(iter)
```

Python對象轉換成字符串

```
var string = Python.str(pyobject)
```

# 原理
戈多里PyScript來表示Python模块和类，它保存了cpython里相應指針，然后成为調用方法、獲取/設置屬性的中間人。

PyScript.set_path方法調用cpython的API去根據模块名獲取指針，模块名的機制和python里的應該相同。

Python的對象用PyScriptInstance表示，和PyScript同理。

戈多值類型和Python的值類型內部自動轉換成相應的。
