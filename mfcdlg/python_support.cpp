#include "stdafx.h"
#include "python_support.h"


PyObject  *pRet = nullptr;
PyObject  *pModule = nullptr;



char *pre_code =
"import ctypes,os\n"

//stack to transform parameters between exe and python.
"stack__=[0]*50\n"

//show message box before foreground window.
"def msgbox(s,title=''):\n"
" ctypes.windll.user32.MessageBoxW(ctypes.windll.user32.GetForegroundWindow(),str(s),title,0x40)\n"

//define exe funciton management class,exe is the object,type it can see all functions.
"class CEXEFUN(object):\n"
"    def __repr__(self):\n"
"        dic=self.__dict__\n"
"        return '\\n**************************************************\\n'.join((x+':\\n'+dic[x].__doc__ for x in dic))\n"
"exe=CEXEFUN()\n"
"def build_exe_fun__(fnn, fmt, adr,doc):\n"
"    dic = {'#': 'ctypes.c_void_p', \n"
"           's':'ctypes.c_char_p','S': 'ctypes.c_wchar_p',\n"
"           'l': 'ctypes.c_int32', 'u': 'ctypes.c_uint32',\n"
"           'L': 'ctypes.c_int64', 'U': 'ctypes.c_uint64',\n"
"           'f': 'ctypes.c_float', 'F': 'ctypes.c_double'}\n"
"    cmd = 'ctypes.CFUNCTYPE('\n"
"    cmd += ','.join(map(lambda x: dic[x], fmt))\n"
"    cmd += ')('+str(adr)+')'\n"
"    exe.__dict__[fnn] = eval(cmd)\n"
"    exe.__dict__[fnn].__doc__=doc\n"
;
void _init_python()//call it before use other function else.
{
	Py_Initialize(); 
	PyEval_InitThreads();
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
	PyEval_SaveThread();
}

void PysetObj(PyObject *p, int idx)
{
	CGIL gil;
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
	CGIL gil;
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
	CGIL gil;
	PySetStrW(arg);
	PyRun_SimpleString(exe_cmd);
	return PyLong_AS_LONG(PyObject_GetAttrString(pModule, "__ok"));
}
int PyExecA(char *arg)
{
	CGIL gil;
	PySetStrA(arg);
	PyRun_SimpleString(exe_cmd);
	int ret= PyLong_AS_LONG(PyObject_GetAttrString(pModule, "__ok"));
	return ret;
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
	CGIL gil;
	PySetStrW(arg);
	PyRun_SimpleString(eval_cmd);
	return PyLong_AS_LONG(PyObject_GetAttrString(pModule, "__ok"));
}

int PyEvalA(char *arg)
{
	CGIL gil;
	PySetStrA(arg);
	PyRun_SimpleString(eval_cmd);
	return PyLong_AS_LONG(PyObject_GetAttrString(pModule, "__ok"));
}

char *evals_cmd =
"try:\n"
"    "PY_TMP_NAME"=str(eval("PY_TMP_NAME"))\n"
"    __ok=1\n"
"except Exception as exp : \n"
"    "PY_TMP_NAME" =str(exp)\n"
"    __ok=0"
;
wchar_t *PyEvalOrExecW(wchar_t *arg)
{
	CGIL gil;
	PySetStrW(arg);
	PyRun_SimpleString(evals_cmd);
	if (PyLong_AS_LONG(PyObject_GetAttrString(pModule, "__ok")))
	{
		return PyGetStr();
	}
	if (PyExecW(arg))
	{
		return _T("");
	}
	return PyGetStr();
}

int PyRunFile(wchar_t *fn)
{
	CGIL gil;
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
	CGIL gil;
	PyObject_CallMethod(pModule, "build_exe_fun__", "ssIs", fnn, fmt, pfn,doc);
}

void InteractInConsole()
{
	AllocConsole();
	SetConsoleTitleA("press Ctrl+C to quit.");

	HWND hwn = ::GetConsoleWindow();
	if (hwn)
	{
		HMENU mn = ::GetSystemMenu(hwn, FALSE);
		if (mn)
		{
			DeleteMenu(mn, SC_CLOSE, MF_BYCOMMAND);
		}
	}
	SetConsoleCtrlHandler(0, true);
	TCHAR Buffer[1000]; //开缓存
	DWORD dwCount = 0;//已输入数
	HANDLE hdlRead = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE hdlWrite = GetStdHandle(STD_OUTPUT_HANDLE);

	SetConsoleTextAttribute((HANDLE)hdlWrite,  FOREGROUND_GREEN | FOREGROUND_INTENSITY);

	while (1)
	{
		WriteConsole(hdlWrite, _T(">>"), 2, NULL, NULL);
		ReadConsole(hdlRead, Buffer, 1000, &dwCount, NULL);
		if (!dwCount)
		{
			break;
		}
		Buffer[dwCount - 2] = 0;//delete '\n\r'
		TCHAR *ptc = PyEvalOrExecW(Buffer);
		if (ptc[0]==0)
		{
			continue;
		}
		int len = wcslen(ptc);
		WriteConsole(hdlWrite, ptc,len, NULL, NULL);
		if (len)WriteConsole(hdlWrite, _T("\n"), 1, NULL, NULL);
	}
	FreeConsole();
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