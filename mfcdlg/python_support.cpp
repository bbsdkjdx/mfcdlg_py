#include "stdafx.h"
#include "python_support.h"


PyObject  *pRet = nullptr;
PyObject  *pModule = nullptr;

char *pre_code =
"import ctypes,os\n"
"msgbox=lambda s:ctypes.windll.user32.MessageBoxW(ctypes.windll.user32.GetForegroundWindow(),s,'',0x40)\n"
"exe_fun__ = dict()\n"
"py_fun__ = dict()\n"
"stack__=[0]*50\n"
"def build_exe_fun__(fnn, fmt, adr,doc):\n"
"    dic = {'#': 'ctypes.c_void_p', \n"
"           's':'ctypes.c_char_p','S': 'ctypes.c_wchar_p',\n"
"           'l': 'ctypes.c_int32', 'u': 'ctypes.c_uint32',\n"
"           'L': 'ctypes.c_int64', 'U': 'ctypes.c_uint64',\n"
"           'f': 'ctypes.c_float', 'F': 'ctypes.c_double'}\n"
"    cmd = 'ctypes.CFUNCTYPE('\n"
"    cmd += ','.join(map(lambda x: dic[x], fmt))\n"
"    cmd += ')('+str(adr)+')'\n"
"    exe_fun__[fnn] = eval(cmd)\n"
"    exe_fun__[fnn].__doc__=doc\n"
"def on_msg__(s, p1, p2):\n"
"    if s not in py_fun__:\n"
"        msgbox('\"'+s+'\" not handled.')\n"
"        return\n"
"    try:\n"
"        py_fun__[s](p1, p2)\n"
"    except:\n"
"        pass\n";

void _init_python()//call it before use other function else.
{
	Py_Initialize();
	Py_SetProgramName(_T(""));  /* optional but recommended */ //define APP_NAME first.
	//makes sys.argv availiable.
	LPTSTR cmd = ::GetCommandLine();
	int argc = 0;
	LPWSTR* argv = ::CommandLineToArgvW(cmd, &argc);
	PySys_SetArgv(argc, argv);
	PyRun_SimpleString(pre_code);
	//set current directory.
	wchar_t szExeFilePathFileName[MAX_PATH];
	GetModuleFileName(NULL, szExeFilePathFileName, MAX_PATH);
	CString str = szExeFilePathFileName;
	str = str.Mid(0, str.ReverseFind(_T('\\')));
	SetCurrentDirectory(str.GetBuffer());

	pModule = PyImport_ImportModule("__main__");
}

void PysetObj(PyObject *p, int idx)
{
	PyObject_SetAttrString(pModule, PY_TMP_NAME, p);
	Py_DECREF(p);
	if (idx > -1)
	{
		char buf[100];
		sprintf_s(buf, "stack__[%d]="PY_TMP_NAME, idx);
		PyRun_SimpleString(buf);
	}
}

void PySetStrA(char *arg, int idx)
{
	PyObject *p = PyUnicode_FromString(arg);
	PysetObj(p, idx);
}

void PySetStrW(wchar_t *arg, int idx)//assign arg to TMP_NAME in python.
{
	PyObject *p = PyUnicode_FromUnicode(arg, wcslen(arg));
	PysetObj(p, idx);
}

void PySetInt(INT64 x, int idx)//assign arg to TMP_NAME in python.
{
	PysetObj(PyLong_FromLongLong(x), idx);
}

void PySetDouble(double d, int idx)
{
	PysetObj(PyFloat_FromDouble(d), idx);
}

PyObject *PyGetObj(int idx)
{
	if (idx > -1)
	{
		char buf[100];
		sprintf_s(buf, PY_TMP_NAME"=stack__[%d]", idx);
		PyRun_SimpleString(buf);
	}
	if (pRet)Py_DECREF(pRet);
	pRet = PyObject_GetAttrString(pModule, PY_TMP_NAME);
	return pRet;
}

wchar_t * PyGetStr(int idx/*=-1*/)
{
	return PyUnicode_AsUnicode(PyGetObj(idx));
}

INT64 PyGetInt(int idx/*=-1*/)
{
	return PyLong_AsLongLong(PyGetObj(idx));
}

double PyGetDouble(int idx/*=-1*/)
{
	return PyFloat_AsDouble(PyGetObj(idx));
}

char *exe_cmd =
"try:\n"
"    exec("PY_TMP_NAME")\n"
"    __ok=1\n"
"except Exception as exp:\n"
"    import sys\n"
"    "PY_TMP_NAME"=str(exp)\n"//+str(sys._getframe().f_locals)\n"
"    __ok=0"
;
int PyExecW(wchar_t *arg)
{
	PySetStrW(arg);
	PyRun_SimpleString(exe_cmd);
	return PyLong_AS_LONG(PyObject_GetAttrString(pModule, "__ok"));
}
int PyExecA(char *arg)
{
	PySetStrA(arg);
	PyRun_SimpleString(exe_cmd);
	return PyLong_AS_LONG(PyObject_GetAttrString(pModule, "__ok"));
}
char *eval_cmd =
"try:\n"
"    "PY_TMP_NAME"=eval("PY_TMP_NAME")\n"
"    __ok=1\n"
"except Exception as exp : \n"
"    "PY_TMP_NAME" =str(exp)\n"
"    __ok=0"
;
int PyEvalW(wchar_t *arg)
{
	PySetStrW(arg);
	PyRun_SimpleString(eval_cmd);
	return PyLong_AS_LONG(PyObject_GetAttrString(pModule, "__ok"));
}

int PyEvalA(char *arg)
{
	PySetStrA(arg);
	PyRun_SimpleString(eval_cmd);
	return PyLong_AS_LONG(PyObject_GetAttrString(pModule, "__ok"));
}

int PyEvalOrExecW(wchar_t *arg)
{
	PySetStrW(arg);
	if (PyEvalW(arg)) return true;
	PySetStrW(arg);
	return PyExecW(arg);
}

int PyRunFile(wchar_t *fn)
{
	PySetStrW(fn);
	return PyExecW(_T("exec(open(__c2p2c__).read())"));
}

//'#':'ctypes.c_void_p', 
//'s' : 'ctypes.c_char_p','S' : 'ctypes.c_wchar_p',
//'l' : 'ctypes.c_int32', 'u' : 'ctypes.c_uint32',
//'L' : 'ctypes.c_int64', 'U' : 'ctypes.c_uint64',
//'f' : 'ctypes.c_float', 'F' : 'ctypes.c_double' 
void reg_exe_fun(char *fnn, char *fmt, void *pfn,char *doc)
{
	PyObject_CallMethod(pModule, "build_exe_fun__", "ssIs", fnn, fmt, pfn,doc);
}

void PySendMsg(char *msg, unsigned int p1,unsigned int p2)
{
	PyObject_CallMethod(pModule, "on_msg__", "sII", msg, p1, p2);
}

///////////////////////auto initialize python.///////////////////////////////
class PY_INITIALIZER
{
public:
	PY_INITIALIZER()
	{
		_init_python();
	}
	~PY_INITIALIZER()
	{

	}
} g_py_initializer;
////////////////////////////////////////////////////////////////////////