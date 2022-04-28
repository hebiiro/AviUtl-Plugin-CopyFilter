#include "pch.h"
#include "CopyFilter.h"

//---------------------------------------------------------------------

// デバッグ用コールバック関数。デバッグメッセージを出力する
void ___outputLog(LPCTSTR text, LPCTSTR output)
{
	::OutputDebugString(output);
}

//---------------------------------------------------------------------

AviUtlInternal g_auin;

BOOL g_pasteFilter = FALSE; // 独自のエイリアス読み込み処理を行うためのフラグ。

struct CopySource
{
	UINT m_objectFlag = 0;

	struct FilterInfo
	{
//		UINT m_filterFlag = 0;
//		int m_filterId = 0;
		int m_filterIndex = 0;
		std::string m_fileName;
	};

	std::vector<FilterInfo> m_filterInfoArray; // コピーしたフィルタの情報の配列。

} g_copySource;

//--------------------------------------------------------------------

// フックをセットする。
void initHook()
{
	MY_TRACE(_T("initHook()\n"));

	true_SettingDialogProc = g_auin.HookSettingDialogProc(hook_SettingDialogProc);
	true_AddAlias = g_auin.GetAddAlias();

	// 拡張編集の関数をフックする。
	DetourTransactionBegin();
	DetourUpdateThread(::GetCurrentThread());

	ATTACH_HOOK_PROC(AddAlias);

	if (DetourTransactionCommit() == NO_ERROR)
	{
		MY_TRACE(_T("拡張編集のフックに成功しました\n"));
	}
	else
	{
		MY_TRACE(_T("拡張編集のフックに失敗しました\n"));
	}
}

// フックを解除する。
void termHook()
{
	MY_TRACE(_T("termHook()\n"));
}

struct CopyFilters
{
	int objectIndex = -1;
	ExEdit::Object* object = 0;
	char tempPath[MAX_PATH] = {};
	DWORD pid = 0;

	CopyFilters()
	{
		MY_TRACE(_T("CopyFilters::CopyFilters()\n"));

		g_copySource.m_objectFlag = 0;
		g_copySource.m_filterInfoArray.clear();
		g_copySource.m_filterInfoArray.reserve(ExEdit::Object::MAX_FILTER);

		// カレントオブジェクトを取得する。
		objectIndex = g_auin.GetCurrentObjectIndex();
		MY_TRACE_INT(objectIndex);
		if (objectIndex < 0) throw 0;
		object = g_auin.GetObject(objectIndex);
		MY_TRACE_HEX(object);
		if (!object) throw 0;

		// オブジェクトのフラグを取得しておく。
		g_copySource.m_objectFlag = (UINT)object->flag;
		MY_TRACE_HEX(g_copySource.m_objectFlag);

		// テンポラリフォルダのパス。
		::GetTempPathA(MAX_PATH, tempPath);
		MY_TRACE_STR(tempPath);

		// カレントプロセスの ID。
		pid = ::GetCurrentProcessId();
		MY_TRACE_INT(pid);
	}

	BOOL copy(int filterIndex)
	{
		MY_TRACE(_T("CopyFilters::copy(%d)\n"), filterIndex);

		if (filterIndex == 0)
			return FALSE; // 先頭のフィルタはコピーしない。

		if (!g_auin.IsMoveable(object, filterIndex))
			return FALSE; // 移動不可能なフィルタはコピーしない。

		// フィルタを取得する。
		ExEdit::Filter* filter = g_auin.GetFilter(object, filterIndex);
		if (!filter) return FALSE;

		// 一時ファイルのファイル名。
		char tempFileName[MAX_PATH] = {};
		::StringCbPrintfA(tempFileName, sizeof(tempFileName),
			"%s\\AviUtl_ObjectExplorer_Copy_%d_%d.exa", tempPath, pid, filterIndex);
		MY_TRACE_STR(tempFileName);

		// 一時ファイルにフィルタのエイリアスを保存する。
		if (!g_auin.SaveFilterAlias(objectIndex, filterIndex, tempFileName))
		{
			MY_TRACE(_T("SaveFilterAlias() が失敗しました\n"));

			return FALSE;
		}

		// フィルタ情報を配列に追加する。
		CopySource::FilterInfo fi;
//		fi.m_filterFlag = (UINT)filter->flag;
//		fi.m_filterId = object->filter_param[filterIndex].id;
		fi.m_filterIndex = filterIndex;
		fi.m_fileName = tempFileName;
		g_copySource.m_filterInfoArray.push_back(fi);

		return TRUE;
	}
};

// 指定されたフィルタを一時ファイルに保存し、ファイル名を配列に追加する。
BOOL copyFilter(int filterIndex, int flag, BOOL cut)
{
	// 戻り値。
	BOOL retValue = FALSE;

	try
	{
		CopyFilters cf;

		switch (flag)
		{
		case 0:
			{
				// 指定されたフィルタのみをコピー。
				retValue |= cf.copy(filterIndex);

				break;
			}
		case -1:
			{
				// 指定されたフィルタより上をコピー。
				for (int i = 0; i <= filterIndex; i++)
					retValue |= cf.copy(i);

				break;
			}
		case 1:
			{
				// 指定されたフィルタより下をコピー。
				for (int i = filterIndex; i < ExEdit::Object::MAX_FILTER; i++)
					retValue |= cf.copy(i);

				break;
			}
		}

		if (cut)
		{
			// フィルタを削除する。

			// カレントオブジェクトを取得する。
			int objectIndex = g_auin.GetCurrentObjectIndex();
			MY_TRACE_INT(objectIndex);
			if (objectIndex < 0) return FALSE;

			// オブジェクトを取得する。
			ExEdit::Object* object = g_auin.GetObject(objectIndex);
			MY_TRACE_HEX(object);
			if (!object) return FALSE;

			if (object->index_midpt_leader != -1)
				objectIndex = object->index_midpt_leader;

			g_auin.PushUndo();
			g_auin.CreateUndo(objectIndex, 1);

			for (int i = g_copySource.m_filterInfoArray.size() - 1; i >= 0; i--)
			{
				g_auin.DeleteFilter(objectIndex, g_copySource.m_filterInfoArray[i].m_filterIndex);
			}

			g_auin.DrawSettingDialog(objectIndex);
			g_auin.HideControls();
			g_auin.ShowControls(g_auin.GetCurrentObjectIndex());
		}
	}
	catch (...)
	{
		return FALSE;
	}

	return retValue;
}

// オブジェクトのタイプを取得する。
int getType(UINT flag)
{
	if (flag & 0x00020000)
	{
		if (flag & 0x00050000)
		{
			return 1; // 音声メディアオブジェクト
		}
		else
		{
			return 2; // 音声フィルタオブジェクト
		}
	}
	else
	{
		if (flag & 0x00050000)
		{
			return 3; // 映像メディアオブジェクト＆グループ制御
		}
		else
		{
			if (flag & 0x00080000)
			{
				return 4; // カメラ制御＆時間制御
			}
			else
			{
				return 5; // 映像フィルタオブジェクト
			}
		}
	}
}

// アイテムにフィルタを貼り付ける。
BOOL pasteFilter()
{
	MY_TRACE(_T("pasteFilter()\n"));

	// カレントオブジェクトを取得する。
	int objectIndex = g_auin.GetCurrentObjectIndex();
	MY_TRACE_INT(objectIndex);
	if (objectIndex < 0) return FALSE;

	// オブジェクトを取得する。
	ExEdit::Object* object = g_auin.GetObject(objectIndex);
	MY_TRACE_HEX(object);
	if (!object) return FALSE;

	if (object->filter_param[0].id == 93) // 93 == 時間制御
		return FALSE; // 「時間制御」には貼り付けられるフィルタがないので何もしない。

	// オブジェクトのタイプを取得する。
	UINT srcType = getType(g_copySource.m_objectFlag);
	UINT dstType = getType((UINT)object->flag);
	MY_TRACE_INT(srcType);
	MY_TRACE_INT(dstType);

	if (srcType != dstType)
		return FALSE; // オブジェクトのタイプが異なる場合は何もしない。

	g_pasteFilter = TRUE;
	// この中で g_auin.AddAlias() が呼ばれるのでフックする。
	// 1036 はエイリアスを追加するコマンド。0 はエイリアスのインデックス。
	::SendMessage(g_auin.GetExeditWindow(), WM_COMMAND, 1036, 0);
	g_pasteFilter = FALSE;

	return TRUE;
}

//--------------------------------------------------------------------

IMPLEMENT_HOOK_PROC_NULL(LRESULT, WINAPI, SettingDialogProc, (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam))
{
	switch (message)
	{
	case WM_CREATE:
		{
			MY_TRACE(_T("WM_CREATE\n"));

			// 設定ダイアログのコンテキストメニューを拡張する。
			for (int i = 0; i < g_auin.GetSettingDialogMenuCount(); i++)
			{
				HMENU menu = g_auin.GetSettingDialogMenu(i);
				HMENU subMenu = ::GetSubMenu(menu, 0);
				::AppendMenu(subMenu, MF_SEPARATOR, 0, 0);
				::AppendMenu(subMenu, MF_STRING, ID_CUT_FILTER, _T("このフィルタを切り取り"));
				::AppendMenu(subMenu, MF_STRING, ID_CUT_FILTER_ABOVE, _T("このフィルタ以上を切り取り"));
				::AppendMenu(subMenu, MF_STRING, ID_CUT_FILTER_BELOW, _T("このフィルタ以下を切り取り"));
				::AppendMenu(subMenu, MF_STRING, ID_COPY_FILTER, _T("このフィルタをコピー"));
				::AppendMenu(subMenu, MF_STRING, ID_COPY_FILTER_ABOVE, _T("このフィルタ以上をコピー"));
				::AppendMenu(subMenu, MF_STRING, ID_COPY_FILTER_BELOW, _T("このフィルタ以下をコピー"));
				::AppendMenu(subMenu, MF_STRING, ID_PASTE_FILTER, _T("フィルタを貼り付け"));
			}

			break;
		}
	case WM_COMMAND:
		{
			switch (wParam)
			{
			case ID_CUT_FILTER:
				{
					MY_TRACE(_T("ID_CUT_FILTER\n"));

					int filterIndex = g_auin.GetCurrentFilterIndex();
					if (filterIndex >= 0)
						copyFilter(filterIndex, 0, TRUE);
					break;
				}
			case ID_CUT_FILTER_ABOVE:
				{
					MY_TRACE(_T("ID_CUT_FILTER_ABOVE\n"));

					int filterIndex = g_auin.GetCurrentFilterIndex();
					if (filterIndex >= 0)
						copyFilter(filterIndex, -1, TRUE);
					break;
				}
			case ID_CUT_FILTER_BELOW:
				{
					MY_TRACE(_T("ID_CUT_FILTER_BELOW\n"));

					int filterIndex = g_auin.GetCurrentFilterIndex();
					if (filterIndex >= 0)
						copyFilter(filterIndex, 1, TRUE);
					break;
				}
			case ID_COPY_FILTER:
				{
					MY_TRACE(_T("ID_COPY_FILTER\n"));

					int filterIndex = g_auin.GetCurrentFilterIndex();
					if (filterIndex >= 0)
						copyFilter(filterIndex, 0, FALSE);
					break;
				}
			case ID_COPY_FILTER_ABOVE:
				{
					MY_TRACE(_T("ID_COPY_FILTER_ABOVE\n"));

					int filterIndex = g_auin.GetCurrentFilterIndex();
					if (filterIndex >= 0)
						copyFilter(filterIndex, -1, FALSE);
					break;
				}
			case ID_COPY_FILTER_BELOW:
				{
					MY_TRACE(_T("ID_COPY_FILTER_BELOW\n"));

					int filterIndex = g_auin.GetCurrentFilterIndex();
					if (filterIndex >= 0)
						copyFilter(filterIndex, 1, FALSE);
					break;
				}
			case ID_PASTE_FILTER:
				{
					MY_TRACE(_T("ID_PASTE_FILTER\n"));

					pasteFilter();

					break;
				}
			}

			break;
		}
	}

	return true_SettingDialogProc(hwnd, message, wParam, lParam);
}

IMPLEMENT_HOOK_PROC_NULL(BOOL, CDECL, AddAlias, (LPCSTR fileName, BOOL flag1, BOOL flag2, int objectIndex))
{
	MY_TRACE(_T("AddAlias(%s, %d, %d, %d)\n"), fileName, flag1, flag2, objectIndex);

	if (g_pasteFilter)
	{
		// フラグが立っている場合はペースト処理を行う。

		// オブジェクトを取得する。
		ExEdit::Object* object = g_auin.GetObject(objectIndex);
		MY_TRACE_HEX(object);
		if (!object) return FALSE;

		// カレントフィルタを取得する。
		int filterIndex = g_auin.GetCurrentFilterIndex();
		MY_TRACE_INT(filterIndex);
		if (filterIndex < 0) return FALSE;

		int insertPos = filterIndex; // フィルタを挿入する位置。
		BOOL retValue = FALSE; // 戻り値。

		for (auto filterInfo : g_copySource.m_filterInfoArray)
		{
			MY_TRACE_STR(filterInfo.m_fileName.c_str());

			// フィルタを末尾に追加する。
			int result = true_AddAlias(filterInfo.m_fileName.c_str(), flag1, flag2, objectIndex);

			int c = g_auin.GetMoveableFilterCount(object);

			// 末尾に追加されたフィルタを挿入位置まで移動する。
			for (int i = c - 1; i > insertPos + 1; i--)
			{
				ExEdit::Filter* filter = g_auin.GetFilter(object, i);

				g_auin.SwapFilter(objectIndex, i, -1);
			}

			insertPos++;
			retValue = TRUE;
		}

		return retValue;
	}

	return true_AddAlias(fileName, flag1, flag2, objectIndex);
}

//--------------------------------------------------------------------
