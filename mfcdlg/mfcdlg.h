
// mfcdlg.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CmfcdlgApp: 
// �йش����ʵ�֣������ mfcdlg.cpp
//

class CmfcdlgApp : public CWinApp
{
public:
	CmfcdlgApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CmfcdlgApp theApp;