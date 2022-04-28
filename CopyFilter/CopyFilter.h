#pragma once

//---------------------------------------------------------------------
// Define and Const

const UINT ID_CREATE_CLONE			= 12020;
const UINT ID_CREATE_SAME_ABOVE		= 12021;
const UINT ID_CREATE_SAME_BELOW		= 12022;

const UINT ID_CUT_FILTER			= 12023;
const UINT ID_CUT_FILTER_ABOVE		= 12024;
const UINT ID_CUT_FILTER_BELOW		= 12025;
const UINT ID_COPY_FILTER			= 12026;
const UINT ID_COPY_FILTER_ABOVE		= 12027;
const UINT ID_COPY_FILTER_BELOW		= 12028;
const UINT ID_PASTE_FILTER			= 12029;

//---------------------------------------------------------------------
// Api Hook

DECLARE_HOOK_PROC(LRESULT, WINAPI, SettingDialogProc, (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam));
DECLARE_HOOK_PROC(BOOL, CDECL, AddAlias, (LPCSTR fileName, BOOL flag1, BOOL flag2, int objectIndex));

//---------------------------------------------------------------------

void initHook();
void termHook();

//---------------------------------------------------------------------

extern AviUtlInternal g_auin;

//---------------------------------------------------------------------
