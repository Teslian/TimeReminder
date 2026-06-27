/**
 * @file    TimeReminder.cpp
 * @brief   Windows桌面定时提醒工具
 * @note    可读取系统时间，支持在指定时间段内弹窗提醒用户执行任务
 *          符合MISRA C++编码规范，使用Win32 API实现
 *          纯代码实现，无需资源文件，单文件编译
 *
 * @version 1.1
 * @date    2026-06-27
 */

/* ============================================================================ */
/*                                  头文件包含                                   */
/* ============================================================================ */
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <ctime>
#include <cstdio>

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

/* ============================================================================ */
/*                                  宏定义                                      */
/* ============================================================================ */
#define ID_TIMER_CLOCK 1001U        /**< 时钟刷新定时器ID */
#define ID_TIMER_CHECK 1002U        /**< 提醒检查定时器ID */
#define TIMER_CLOCK_INTERVAL 1000U  /**< 时钟刷新间隔(ms) */
#define TIMER_CHECK_INTERVAL 60000U /**< 提醒检查间隔(ms), 60秒 */
#define MAX_REMINDERS 50U           /**< 最大提醒数量 */
#define MAX_TEXT_LEN 256U           /**< 最大文本长度 */

/* 主窗口控件ID */
#define ID_BTN_ADD 2001U
#define ID_BTN_DELETE 2002U
#define ID_LST_REMINDERS 2003U
#define ID_STC_CLOCK 2004U
#define ID_STC_STATUS 2005U

/* 添加对话框控件ID */
#define ID_EDT_CONTENT 3001U
#define ID_DTP_START 3002U
#define ID_DTP_END 3003U
#define ID_BTN_OK 3004U
#define ID_BTN_CANCEL 3005U
#define ID_CHK_DAILY 3006U

/* ============================================================================ */
/*                                  数据结构                                     */
/* ============================================================================ */

/**
 * @brief 提醒项结构体
 */
typedef struct tagReminderItem
{
    CHAR szContent[MAX_TEXT_LEN]; /**< 提醒内容 */
    SYSTEMTIME stStartTime;       /**< 提醒开始时间 */
    SYSTEMTIME stEndTime;         /**< 提醒结束时间 */
    BOOL bEnabled;                /**< 是否启用 */
    BOOL bTriggered;              /**< 当前分钟是否已触发(防重复) */
    BOOL bDailyRepeat;            /**< 是否每天重复提醒 */
} ReminderItem;

/* ============================================================================ */
/*                                  全局变量                                     */
/* ============================================================================ */
static HINSTANCE g_hInst = NULL;                 /**< 应用实例句柄 */
static HWND g_hWndMain = NULL;                   /**< 主窗口句柄 */
static HWND g_hListBox = NULL;                   /**< 提醒列表控件 */
static HWND g_hClockLabel = NULL;                /**< 时钟标签 */
static HWND g_hStatusLabel = NULL;               /**< 状态标签 */
static std::vector<ReminderItem> g_vecReminders; /**< 提醒列表 */
static HFONT g_hFontLarge = NULL;                /**< 大字体 */
static HFONT g_hFontNormal = NULL;               /**< 普通字体 */
static HWND g_hWndAddDlg = NULL;                 /**< 添加对话框句柄 */

/* ============================================================================ */
/*                                  函数声明                                     */
/* ============================================================================ */
static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK AddDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static VOID RegisterWindowClass(HINSTANCE hInstance);
static HWND CreateMainWindow(HINSTANCE hInstance);
static VOID CreateControls(HWND hWnd);
static VOID UpdateClockDisplay(VOID);
static VOID CheckReminders(VOID);
static VOID AddReminderToList(const ReminderItem &item);
static VOID RefreshReminderList(VOID);
static VOID ShowAddReminderDialog(VOID);
static VOID DeleteSelectedReminder(VOID);
static BOOL IsTimeInRange(const SYSTEMTIME &stCurrent, const SYSTEMTIME &stStart, const SYSTEMTIME &stEnd);
static VOID FormatTimeString(const SYSTEMTIME &st, LPSTR szBuf, SIZE_T nBufLen);
static VOID FormatReminderString(const ReminderItem &item, LPSTR szBuf, SIZE_T nBufLen);
static SYSTEMTIME GetTimeOnly(const SYSTEMTIME &st);

/* ============================================================================ */
/*                                  函数实现                                     */
/* ============================================================================ */

/**
 * @brief  WinMain - 程序入口点
 * @param  hInstance     当前实例句柄
 * @param  hPrevInstance 前一个实例句柄
 * @param  lpCmdLine     命令行参数
 * @param  nCmdShow      窗口显示方式
 * @return 程序退出码
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    (void)hPrevInstance;
    (void)lpCmdLine;

    /* 初始化公共控件 */
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_DATE_CLASSES;
    InitCommonControlsEx(&icex);

    g_hInst = hInstance;

    /* 创建字体 */
    g_hFontLarge = CreateFontW(36, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei");

    g_hFontNormal = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei");

    /* 注册窗口类 */
    RegisterWindowClass(hInstance);

    /* 创建主窗口 */
    g_hWndMain = CreateMainWindow(hInstance);
    if (NULL == g_hWndMain)
    {
        return 1;
    }

    /* 显示窗口 */
    ShowWindow(g_hWndMain, nCmdShow);
    UpdateWindow(g_hWndMain);

    /* 启动定时器 */
    SetTimer(g_hWndMain, ID_TIMER_CLOCK, TIMER_CLOCK_INTERVAL, NULL);
    SetTimer(g_hWndMain, ID_TIMER_CHECK, TIMER_CHECK_INTERVAL, NULL);

    /* 立即更新时钟 */
    UpdateClockDisplay();

    /* 消息循环 */
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    /* 清理资源 */
    KillTimer(g_hWndMain, ID_TIMER_CLOCK);
    KillTimer(g_hWndMain, ID_TIMER_CHECK);

    if (NULL != g_hFontLarge)
    {
        DeleteObject(g_hFontLarge);
    }
    if (NULL != g_hFontNormal)
    {
        DeleteObject(g_hFontNormal);
    }

    return (int)msg.wParam;
}

/**
 * @brief  注册窗口类
 * @param  hInstance 实例句柄
 */
static VOID RegisterWindowClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;
    (VOID) memset(&wcex, 0, sizeof(WNDCLASSEX));

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(NULL, IDI_INFORMATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"TimeReminderClass";
    wcex.hIconSm = LoadIcon(NULL, IDI_INFORMATION);

    RegisterClassExW(&wcex);

    /* 注册添加对话框窗口类 */
    WNDCLASSEXW wcexDlg;
    (VOID) memset(&wcexDlg, 0, sizeof(WNDCLASSEX));
    wcexDlg.cbSize = sizeof(WNDCLASSEX);
    wcexDlg.style = CS_HREDRAW | CS_VREDRAW;
    wcexDlg.lpfnWndProc = AddDlgProc;
    wcexDlg.cbClsExtra = 0;
    wcexDlg.cbWndExtra = 0;
    wcexDlg.hInstance = hInstance;
    wcexDlg.hIcon = LoadIcon(NULL, IDI_INFORMATION);
    wcexDlg.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcexDlg.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wcexDlg.lpszMenuName = NULL;
    wcexDlg.lpszClassName = L"AddReminderDlgClass";
    wcexDlg.hIconSm = LoadIcon(NULL, IDI_INFORMATION);

    RegisterClassExW(&wcexDlg);
}

/**
 * @brief  创建主窗口
 * @param  hInstance 实例句柄
 * @return 窗口句柄
 */
static HWND CreateMainWindow(HINSTANCE hInstance)
{
    HWND hWnd = CreateWindowW(
        L"TimeReminderClass",
        L"定时提醒工具 v1.0",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        520, 480,
        NULL, NULL, hInstance, NULL);

    return hWnd;
}

/**
 * @brief  创建子控件
 * @param  hWnd 主窗口句柄
 */
static VOID CreateControls(HWND hWnd)
{
    /* 时钟标签 */
    g_hClockLabel = CreateWindowW(L"STATIC", L"00:00:00",
                                  WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
                                  20, 15, 460, 60, hWnd, (HMENU)ID_STC_CLOCK, g_hInst, NULL);
    SendMessage(g_hClockLabel, WM_SETFONT, (WPARAM)g_hFontLarge, TRUE);

    /* 分隔线 */
    CreateWindowW(L"STATIC", L"",
                  WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
                  20, 85, 460, 2, hWnd, NULL, g_hInst, NULL);

    /* 提醒列表标签 */
    HWND hListLabel = CreateWindowW(L"STATIC", L"提醒列表：",
                                    WS_CHILD | WS_VISIBLE | SS_LEFT,
                                    20, 95, 200, 20, hWnd, NULL, g_hInst, NULL);
    SendMessage(hListLabel, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

    /* 提醒列表 */
    g_hListBox = CreateWindowW(L"LISTBOX", L"",
                               WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL | WS_BORDER,
                               20, 120, 460, 230, hWnd, (HMENU)ID_LST_REMINDERS, g_hInst, NULL);
    SendMessage(g_hListBox, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

    /* 添加按钮 */
    HWND hBtnAdd = CreateWindowW(L"BUTTON", L"添加提醒",
                                 WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                 120, 365, 130, 35, hWnd, (HMENU)ID_BTN_ADD, g_hInst, NULL);
    SendMessage(hBtnAdd, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

    /* 删除按钮 */
    HWND hBtnDelete = CreateWindowW(L"BUTTON", L"删除选中",
                                    WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                    270, 365, 130, 35, hWnd, (HMENU)ID_BTN_DELETE, g_hInst, NULL);
    SendMessage(hBtnDelete, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

    /* 状态栏 */
    g_hStatusLabel = CreateWindowW(L"STATIC", L"就绪 | 提醒数: 0",
                                   WS_CHILD | WS_VISIBLE | SS_LEFT,
                                   20, 415, 460, 20, hWnd, (HMENU)ID_STC_STATUS, g_hInst, NULL);
    SendMessage(g_hStatusLabel, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
}

/**
 * @brief  更新时钟显示
 */
static VOID UpdateClockDisplay(VOID)
{
    SYSTEMTIME st;
    GetLocalTime(&st);

    CHAR szTime[64];
    (VOID) sprintf_s(szTime, sizeof(szTime), "%04d/%02d/%02d  %02d:%02d:%02d",
                     st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

    SetWindowTextA(g_hClockLabel, szTime);
}

/**
 * @brief  检查所有提醒是否需要触发
 */
static VOID CheckReminders(VOID)
{
    SYSTEMTIME stCurrent;
    GetLocalTime(&stCurrent);
    SYSTEMTIME stTimeOnly = GetTimeOnly(stCurrent);

    SIZE_T nCount = g_vecReminders.size();
    for (SIZE_T i = 0; i < nCount; i++)
    {
        ReminderItem &item = g_vecReminders[i];
        if (FALSE == item.bEnabled)
        {
            continue;
        }

        SYSTEMTIME stStart = GetTimeOnly(item.stStartTime);
        SYSTEMTIME stEnd = GetTimeOnly(item.stEndTime);

        BOOL bInRange = IsTimeInRange(stTimeOnly, stStart, stEnd);

        if (TRUE == bInRange)
        {
            /* 在提醒时间范围内且本分钟未触发过 */
            if (FALSE == item.bTriggered)
            {
                item.bTriggered = TRUE;

                CHAR szMsg[MAX_TEXT_LEN + 128];
                (VOID) sprintf_s(szMsg, sizeof(szMsg),
                                 "提醒时间到！\n\n"
                                 "提醒内容：%s\n"
                                 "提醒时段：%02d:%02d - %02d:%02d\n"
                                 "重复方式：%s\n\n"
                                 "请在规定时间内完成！",
                                 item.szContent,
                                 item.stStartTime.wHour, item.stStartTime.wMinute,
                                 item.stEndTime.wHour, item.stEndTime.wMinute,
                                 (TRUE == item.bDailyRepeat) ? "每天重复" : "仅一次");

                /* 弹窗提醒，置顶显示 */
                MessageBoxA(g_hWndMain, szMsg, "定时提醒", MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TOPMOST);

                /* 一次性提醒：触发后自动禁用 */
                if (FALSE == item.bDailyRepeat)
                {
                    item.bEnabled = FALSE;
                }

                /* 弹窗关闭后重新刷新列表 */
                RefreshReminderList();
            }
        }
        else
        {
            /* 不在范围内，重置触发标志（每天重复的提醒次日可再次触发） */
            item.bTriggered = FALSE;
        }
    }
}

/**
 * @brief  添加提醒项到列表显示
 * @param  item 提醒项
 */
static VOID AddReminderToList(const ReminderItem &item)
{
    CHAR szDisplay[MAX_TEXT_LEN + 64];
    FormatReminderString(item, szDisplay, sizeof(szDisplay));

    SendMessageA(g_hListBox, LB_ADDSTRING, 0, (LPARAM)szDisplay);
}

/**
 * @brief  刷新整个提醒列表显示
 */
static VOID RefreshReminderList(VOID)
{
    SendMessageA(g_hListBox, LB_RESETCONTENT, 0, 0);

    SIZE_T nCount = g_vecReminders.size();
    for (SIZE_T i = 0; i < nCount; i++)
    {
        AddReminderToList(g_vecReminders[i]);
    }

    /* 更新状态栏 */
    CHAR szStatus[128];
    SIZE_T nEnabled = 0;
    for (SIZE_T i = 0; i < nCount; i++)
    {
        if (TRUE == g_vecReminders[i].bEnabled)
        {
            nEnabled++;
        }
    }
    (VOID) sprintf_s(szStatus, sizeof(szStatus), "就绪 | 提醒总数: %zu | 已启用: %zu",
                     nCount, nEnabled);
    SetWindowTextA(g_hStatusLabel, szStatus);
}

/**
 * @brief  显示添加提醒对话框
 */
static VOID ShowAddReminderDialog(VOID)
{
    if (g_vecReminders.size() >= MAX_REMINDERS)
    {
        MessageBoxA(g_hWndMain, "提醒数量已达上限！", "提示", MB_OK | MB_ICONWARNING);
        return;
    }

    /* 创建模态对话框风格的窗口 */
    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        L"AddReminderDlgClass",
        L"添加新提醒",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        (GetSystemMetrics(SM_CXSCREEN) - 400) / 2,
        (GetSystemMetrics(SM_CYSCREEN) - 220) / 2,
        400, 250,
        g_hWndMain, NULL, g_hInst, NULL);

    if (NULL == hDlg)
    {
        return;
    }

    g_hWndAddDlg = hDlg;

    /* 创建控件 */
    /* 内容标签 */
    HWND hLblContent = CreateWindowW(L"STATIC", L"提醒内容：",
                                     WS_CHILD | WS_VISIBLE | SS_RIGHT,
                                     20, 24, 80, 20, hDlg, NULL, g_hInst, NULL);
    SendMessage(hLblContent, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

    /* 内容输入框 */
    HWND hEdtContent = CreateWindowW(L"EDIT", L"",
                                     WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                                     110, 20, 260, 24, hDlg, (HMENU)ID_EDT_CONTENT, g_hInst, NULL);
    SendMessage(hEdtContent, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

    /* 开始时间标签 */
    HWND hLblStart = CreateWindowW(L"STATIC", L"开始时间：",
                                   WS_CHILD | WS_VISIBLE | SS_RIGHT,
                                   20, 64, 80, 20, hDlg, NULL, g_hInst, NULL);
    SendMessage(hLblStart, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

    /* 开始时间选择器 */
    HWND hDtpStart = CreateWindowExW(0, DATETIMEPICK_CLASS, L"",
                                     WS_CHILD | WS_VISIBLE | DTS_TIMEFORMAT,
                                     110, 58, 140, 26, hDlg, (HMENU)ID_DTP_START, g_hInst, NULL);
    SendMessage(hDtpStart, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
    SendMessageW(hDtpStart, DTM_SETFORMAT, 0, (LPARAM)L"HH:mm");

    /* 结束时间标签 */
    HWND hLblEnd = CreateWindowW(L"STATIC", L"结束时间：",
                                 WS_CHILD | WS_VISIBLE | SS_RIGHT,
                                 20, 104, 80, 20, hDlg, NULL, g_hInst, NULL);
    SendMessage(hLblEnd, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

    /* 结束时间选择器 */
    HWND hDtpEnd = CreateWindowExW(0, DATETIMEPICK_CLASS, L"",
                                   WS_CHILD | WS_VISIBLE | DTS_TIMEFORMAT,
                                   110, 98, 140, 26, hDlg, (HMENU)ID_DTP_END, g_hInst, NULL);
    SendMessage(hDtpEnd, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
    SendMessageW(hDtpEnd, DTM_SETFORMAT, 0, (LPARAM)L"HH:mm");

    /* 每天重复复选框 */
    HWND hChkDaily = CreateWindowW(L"BUTTON", L"每天重复提醒",
                                   WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                   110, 135, 200, 20, hDlg, (HMENU)ID_CHK_DAILY, g_hInst, NULL);
    SendMessage(hChkDaily, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
    /* 默认勾选每天重复 */
    SendMessage(hChkDaily, BM_SETCHECK, BST_CHECKED, 0);

    /* 设置默认时间：开始16:45，结束17:00 */
    SYSTEMTIME stDefaultStart;
    (VOID) memset(&stDefaultStart, 0, sizeof(SYSTEMTIME));
    stDefaultStart.wHour = 16;
    stDefaultStart.wMinute = 45;
    SendMessage(hDtpStart, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&stDefaultStart);

    SYSTEMTIME stDefaultEnd;
    (VOID) memset(&stDefaultEnd, 0, sizeof(SYSTEMTIME));
    stDefaultEnd.wHour = 17;
    stDefaultEnd.wMinute = 0;
    SendMessage(hDtpEnd, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&stDefaultEnd);

    /* 确定按钮 */
    HWND hBtnOK = CreateWindowW(L"BUTTON", L"确定",
                                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_DEFPUSHBUTTON,
                                110, 170, 100, 32, hDlg, (HMENU)ID_BTN_OK, g_hInst, NULL);
    SendMessage(hBtnOK, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

    /* 取消按钮 */
    HWND hBtnCancel = CreateWindowW(L"BUTTON", L"取消",
                                    WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                    230, 170, 100, 32, hDlg, (HMENU)ID_BTN_CANCEL, g_hInst, NULL);
    SendMessage(hBtnCancel, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

    /* 禁用父窗口 */
    EnableWindow(g_hWndMain, FALSE);

    /* 显示对话框 */
    ShowWindow(hDlg, SW_SHOW);
    UpdateWindow(hDlg);

    /* 消息循环（模态） */
    MSG msg;
    BOOL bContinue = TRUE;
    while (TRUE == bContinue)
    {
        BOOL bRet = GetMessage(&msg, NULL, 0, 0);
        if (0 == bRet || -1 == bRet)
        {
            break;
        }

        /* 处理Tab键导航 */
        if (WM_KEYDOWN == msg.message)
        {
            if (VK_TAB == msg.wParam)
            {
                HWND hFocus = GetFocus();
                HWND hNext = GetNextDlgTabItem(hDlg, hFocus, (0 != (GetKeyState(VK_SHIFT) & 0x8000)));
                SetFocus(hNext);
                continue;
            }
            else if (VK_RETURN == msg.wParam)
            {
                /* 回车键触发确定按钮 */
                SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(ID_BTN_OK, BN_CLICKED), 0);
                continue;
            }
            else if (VK_ESCAPE == msg.wParam)
            {
                /* ESC键关闭对话框 */
                SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(ID_BTN_CANCEL, BN_CLICKED), 0);
                continue;
            }
        }

        if (!IsDialogMessage(hDlg, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        /* 检查对话框是否已关闭 */
        if (!IsWindow(hDlg))
        {
            bContinue = FALSE;
        }
    }

    /* 恢复父窗口 */
    EnableWindow(g_hWndMain, TRUE);
    SetFocus(g_hWndMain);
    g_hWndAddDlg = NULL;
}

/**
 * @brief  删除选中的提醒项
 */
static VOID DeleteSelectedReminder(VOID)
{
    LRESULT nSel = SendMessageA(g_hListBox, LB_GETCURSEL, 0, 0);
    if (LB_ERR == nSel)
    {
        MessageBoxA(g_hWndMain, "请先选中要删除的提醒项！", "提示", MB_OK | MB_ICONWARNING);
        return;
    }

    if (IDYES == MessageBoxA(g_hWndMain, "确定要删除选中的提醒项吗？", "确认删除",
                             MB_YESNO | MB_ICONQUESTION))
    {
        g_vecReminders.erase(g_vecReminders.begin() + (SIZE_T)nSel);
        RefreshReminderList();
    }
}

/**
 * @brief  判断当前时间是否在指定时间范围内（仅比较时分）
 * @param  stCurrent 当前时间
 * @param  stStart   开始时间
 * @param  stEnd     结束时间
 * @return TRUE=在范围内, FALSE=不在范围内
 */
static BOOL IsTimeInRange(const SYSTEMTIME &stCurrent, const SYSTEMTIME &stStart, const SYSTEMTIME &stEnd)
{
    /* 转换为分钟数便于比较 */
    DWORD dwCurrent = (DWORD)stCurrent.wHour * 60U + (DWORD)stCurrent.wMinute;
    DWORD dwStart = (DWORD)stStart.wHour * 60U + (DWORD)stStart.wMinute;
    DWORD dwEnd = (DWORD)stEnd.wHour * 60U + (DWORD)stEnd.wMinute;

    if (dwStart <= dwEnd)
    {
        /* 不跨天的情况：如 08:00 - 17:00 */
        return ((dwCurrent >= dwStart) && (dwCurrent <= dwEnd)) ? TRUE : FALSE;
    }
    else
    {
        /* 跨天的情况：如 22:00 - 06:00 */
        return ((dwCurrent >= dwStart) || (dwCurrent <= dwEnd)) ? TRUE : FALSE;
    }
}

/**
 * @brief  格式化时间字符串（仅时分）
 * @param  st      时间结构
 * @param  szBuf   输出缓冲区
 * @param  nBufLen 缓冲区长度
 */
static VOID FormatTimeString(const SYSTEMTIME &st, LPSTR szBuf, SIZE_T nBufLen)
{
    (VOID) sprintf_s(szBuf, nBufLen, "%02d:%02d", st.wHour, st.wMinute);
}

/**
 * @brief  格式化提醒项显示字符串
 * @param  item    提醒项
 * @param  szBuf   输出缓冲区
 * @param  nBufLen 缓冲区长度
 */
static VOID FormatReminderString(const ReminderItem &item, LPSTR szBuf, SIZE_T nBufLen)
{
    CHAR szStart[16];
    CHAR szEnd[16];
    FormatTimeString(item.stStartTime, szStart, sizeof(szStart));
    FormatTimeString(item.stEndTime, szEnd, sizeof(szEnd));

    (VOID) sprintf_s(szBuf, nBufLen, "[%s] %s  (%s - %s) %s",
                     (TRUE == item.bEnabled) ? "ON " : "OFF",
                     item.szContent, szStart, szEnd,
                     (TRUE == item.bDailyRepeat) ? "[每天]" : "[一次]");
}

/**
 * @brief  提取时间部分（仅时分秒）
 * @param  st 完整时间
 * @return 仅含时分秒的时间结构
 */
static SYSTEMTIME GetTimeOnly(const SYSTEMTIME &st)
{
    SYSTEMTIME stResult;
    (VOID) memset(&stResult, 0, sizeof(SYSTEMTIME));
    stResult.wHour = st.wHour;
    stResult.wMinute = st.wMinute;
    stResult.wSecond = st.wSecond;
    return stResult;
}

/**
 * @brief  主窗口过程
 * @param  hWnd    窗口句柄
 * @param  message 消息ID
 * @param  wParam  消息参数
 * @param  lParam  消息参数
 * @return 消息处理结果
 */
static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult = 0;

    switch (message)
    {
    case WM_CREATE:
        CreateControls(hWnd);
        break;

    case WM_TIMER:
        if ((UINT_PTR)ID_TIMER_CLOCK == wParam)
        {
            UpdateClockDisplay();
        }
        else if ((UINT_PTR)ID_TIMER_CHECK == wParam)
        {
            CheckReminders();
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_BTN_ADD:
            ShowAddReminderDialog();
            break;
        case ID_BTN_DELETE:
            DeleteSelectedReminder();
            break;
        default:
            break;
        }
        break;

    case WM_DESTROY:
        KillTimer(hWnd, ID_TIMER_CLOCK);
        KillTimer(hWnd, ID_TIMER_CHECK);
        PostQuitMessage(0);
        break;

    default:
        lResult = DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }

    return lResult;
}

/**
 * @brief  添加提醒对话框过程
 * @param  hDlg    对话框句柄
 * @param  message 消息ID
 * @param  wParam  消息参数
 * @param  lParam  消息参数
 * @return 消息处理结果
 */
static LRESULT CALLBACK AddDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult = 0;

    switch (message)
    {
    case WM_CREATE:
        /* 控件在ShowAddReminderDialog中创建 */
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_BTN_OK:
        {
            /* 获取输入内容 */
            HWND hEdtContent = GetDlgItem(hDlg, ID_EDT_CONTENT);
            CHAR szContent[MAX_TEXT_LEN];
            GetWindowTextA(hEdtContent, szContent, MAX_TEXT_LEN);

            /* 检查内容是否为空 */
            if (0 == strlen(szContent))
            {
                MessageBoxA(hDlg, "请输入提醒内容！", "提示", MB_OK | MB_ICONWARNING);
                lResult = 0;
                break;
            }

            /* 获取开始和结束时间 */
            SYSTEMTIME stStart;
            SYSTEMTIME stEnd;
            (VOID) memset(&stStart, 0, sizeof(SYSTEMTIME));
            (VOID) memset(&stEnd, 0, sizeof(SYSTEMTIME));

            HWND hDtpStart = GetDlgItem(hDlg, ID_DTP_START);
            HWND hDtpEnd = GetDlgItem(hDlg, ID_DTP_END);
            SendMessage(hDtpStart, DTM_GETSYSTEMTIME, 0, (LPARAM)&stStart);
            SendMessage(hDtpEnd, DTM_GETSYSTEMTIME, 0, (LPARAM)&stEnd);

            /* 获取每天重复选项 */
            HWND hChkDaily = GetDlgItem(hDlg, ID_CHK_DAILY);
            BOOL bDailyRepeat = (BST_CHECKED == SendMessage(hChkDaily, BM_GETCHECK, 0, 0)) ? TRUE : FALSE;

            /* 创建提醒项 */
            ReminderItem item;
            (VOID) memset(&item, 0, sizeof(ReminderItem));
            (VOID) strncpy_s(item.szContent, sizeof(item.szContent), szContent, _TRUNCATE);
            (VOID) memcpy(&item.stStartTime, &stStart, sizeof(SYSTEMTIME));
            (VOID) memcpy(&item.stEndTime, &stEnd, sizeof(SYSTEMTIME));
            item.bEnabled = TRUE;
            item.bTriggered = FALSE;
            item.bDailyRepeat = bDailyRepeat;

            /* 添加到列表 */
            g_vecReminders.push_back(item);
            AddReminderToList(item);

            /* 更新状态栏 */
            RefreshReminderList();

            /* 关闭对话框 */
            DestroyWindow(hDlg);
            lResult = 0;
            break;
        }

        case ID_BTN_CANCEL:
            DestroyWindow(hDlg);
            lResult = 0;
            break;

        default:
            break;
        }
        break;

    case WM_CLOSE:
        DestroyWindow(hDlg);
        lResult = 0;
        break;

    default:
        lResult = DefWindowProc(hDlg, message, wParam, lParam);
        break;
    }

    return lResult;
}