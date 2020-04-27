// chip8Dlg.cpp : implementation file
//

#include "stdafx.h"
#include "chip8.h"
#include "chip8Dlg.h"
#include "chip8vm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define TIMER_OPEN_FILE  1

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
    CAboutDlg();

// Dialog Data
    enum { IDD = IDD_ABOUTBOX };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// Cchip8Dlg dialog
Cchip8Dlg::Cchip8Dlg(CWnd* pParent /*=NULL*/)
    : CDialog(Cchip8Dlg::IDD, pParent)
    , m_pChip8VMCtxt(NULL)
    , m_wChip8VMKeys(0)
    , m_bExitChip8VM(FALSE)
    , m_hChip8VMThread(NULL)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void Cchip8Dlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(Cchip8Dlg, CDialog)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_WM_CTLCOLOR()
    //}}AFX_MSG_MAP
    ON_WM_DESTROY()
    ON_WM_TIMER()
    ON_COMMAND(ID_OPEN_ROM_FILE, &Cchip8Dlg::OnOpenRomFile)
END_MESSAGE_MAP()


static DWORD WINAPI Chip8VMThreadProc(LPVOID pParam)
{
    Cchip8Dlg *dlg = (Cchip8Dlg*)pParam;
    dlg->DoRunChip8VM();
    return 0;
}

void Cchip8Dlg::DoRunChip8VM()
{
    DWORD next_tick = 0;
    int   sleep_tick= 0;
    int   i;
    while (!m_bExitChip8VM) {
        if (!next_tick) next_tick = GetTickCount();
        next_tick += 1000 / 60;
        for (i=0; i<8; i++) chip8vm_run(m_pChip8VMCtxt, !i);
        chip8vm_render(m_pChip8VMCtxt, m_pMemBitmap);
        InvalidateRect(NULL, 0);
        sleep_tick = next_tick - GetTickCount();
        if (sleep_tick > 0) Sleep(sleep_tick );
        else if (sleep_tick < -1000) next_tick = 0;
    }
}

void Cchip8Dlg::SetWindowClientSize(int w, int h)
{
    RECT rect1, rect2;
    GetWindowRect(&rect1);
    GetClientRect(&rect2);
    w+= (rect1.right  - rect1.left) - rect2.right;
    h+= (rect1.bottom - rect1.top ) - rect2.bottom;
    int x = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;
    x = x > 0 ? x : 0;
    y = y > 0 ? y : 0;
    MoveWindow(x, y, w, h, TRUE);
}

void Cchip8Dlg::OpenRomFile(char *file)
{
    char romfile[MAX_PATH] = "";
    if (!file) {
        CFileDialog dlg(TRUE);
        if (dlg.DoModal() == IDOK) {
            _tcsncpy(romfile, dlg.GetPathName(), sizeof(romfile));
        } else {
            OnOK();
        }
    } else {
        _tcsncpy(romfile, file, sizeof(romfile));
    }
    int pos = CString(romfile).ReverseFind('\\');
    if (pos != -1) pos++;
    SetWindowText(CString("lxqchip8 - ") + CString(romfile).Mid(pos));

    if (m_pChip8VMCtxt) {
        chip8vm_stop(m_pChip8VMCtxt);
        m_bExitChip8VM = TRUE;
        if (m_hChip8VMThread) {
            WaitForSingleObject(m_hChip8VMThread, -1);
            CloseHandle(m_hChip8VMThread);
        }
        chip8vm_exit(m_pChip8VMCtxt);
    }

    m_pChip8VMCtxt   = chip8vm_init(romfile);
    m_wChip8VMKeys   = 0;
    m_bExitChip8VM   = FALSE;
    m_hChip8VMThread = CreateThread(NULL, 0, Chip8VMThreadProc, this, 0, NULL);
}

// Cchip8Dlg message handlers

BOOL Cchip8Dlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // Add "About..." menu item to system menu.

    // IDM_ABOUTBOX must be in the system command range.
    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);

    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu != NULL) {
        CString strAboutMenu;
        strAboutMenu.LoadString(IDS_ABOUTBOX);
        if (!strAboutMenu.IsEmpty()) {
            pSysMenu->AppendMenu(MF_SEPARATOR);
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
        }
    }

    // Set the icon for this dialog.  The framework does this automatically
    //  when the application's main window is not a dialog
    SetIcon(m_hIcon, TRUE);         // Set big icon
    SetIcon(m_hIcon, FALSE);        // Set small icon

    // load accelerators
    m_hAcc = LoadAccelerators(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_ACCELERATOR1)); 

    // TODO: Add extra initialization here
    SetWindowClientSize(64 * 5, 32 * 5);

    BITMAPINFO bmpinfo = {0};
    bmpinfo.bmiHeader.biSize        =  sizeof(BITMAPINFOHEADER);
    bmpinfo.bmiHeader.biWidth       =  64 * 5;
    bmpinfo.bmiHeader.biHeight      = -32 * 5;
    bmpinfo.bmiHeader.biPlanes      =  1;
    bmpinfo.bmiHeader.biBitCount    =  32;
    bmpinfo.bmiHeader.biCompression =  BI_RGB;
    CDC *pDC = GetDC();
    m_hMemDC     = CreateCompatibleDC(pDC->m_hDC);
    m_hMemBitmap = CreateDIBSection(m_hMemDC, &bmpinfo, DIB_RGB_COLORS, (void**)&m_pMemBitmap, NULL, 0);
    SelectObject(m_hMemDC, m_hMemBitmap);
    ReleaseDC(pDC);

    if (__argc >= 2) {
        OpenRomFile(__argv[1]);
    } else {
        SetTimer(TIMER_OPEN_FILE, 100, NULL);
    }
    return TRUE;  // return TRUE  unless you set the focus to a control
}

void Cchip8Dlg::OnDestroy()
{
    CDialog::OnDestroy();

    // TODO: Add your message handler code here
    chip8vm_stop(m_pChip8VMCtxt);
    m_bExitChip8VM = TRUE;
    if (m_hChip8VMThread) {
        WaitForSingleObject(m_hChip8VMThread, -1);
        CloseHandle(m_hChip8VMThread);
    }
    chip8vm_exit(m_pChip8VMCtxt);

    DeleteDC(m_hMemDC);
    DeleteObject(m_hMemBitmap);
}

void Cchip8Dlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX) {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    } else {
        CDialog::OnSysCommand(nID, lParam);
    }
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void Cchip8Dlg::OnPaint()
{
    if (IsIconic()) {
        CPaintDC dc(this); // device context for painting

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // Center icon in client rectangle
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // Draw the icon
        dc.DrawIcon(x, y, m_hIcon);
    } else {
        CPaintDC dc(this); CRect rect; GetClientRect(&rect);
        StretchBlt(dc, 0, 0, rect.right, rect.bottom, m_hMemDC, 0, 0, 64 * 5, 32 * 5, SRCCOPY);
    }
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR Cchip8Dlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

HBRUSH Cchip8Dlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

    // TODO: Change any attributes of the DC here
    // TODO: Return a different brush if the default is not desired
    if (pWnd == this) return (HBRUSH)GetStockObject(BLACK_BRUSH);
    else return hbr;
}

BOOL Cchip8Dlg::PreTranslateMessage(MSG *pMsg)
{
    if (TranslateAccelerator(GetSafeHwnd(), m_hAcc, pMsg)) return TRUE;
    if (pMsg->message == WM_KEYDOWN) {
        if (pMsg->wParam >= '0' && pMsg->wParam <= '9') {
            m_wChip8VMKeys |= (1 << (pMsg->wParam - '0'));
        } else if (pMsg->wParam >= VK_NUMPAD0 && pMsg->wParam <= VK_NUMPAD9) {
            m_wChip8VMKeys |= (1 << (pMsg->wParam - VK_NUMPAD0));
        } else if (pMsg->wParam >= 'a' && pMsg->wParam <= 'f') {
            m_wChip8VMKeys |= (1 << (0xa + pMsg->wParam - 'a'));
        } else if (pMsg->wParam >= 'A' && pMsg->wParam <= 'F') {
            m_wChip8VMKeys |= (1 << (0xa + pMsg->wParam - 'A'));
        }
        chip8vm_key(m_pChip8VMCtxt, m_wChip8VMKeys);
    } else if (pMsg->message == WM_KEYUP) {
        if (pMsg->wParam >= '0' && pMsg->wParam <= '9') {
            m_wChip8VMKeys &=~(1 << (pMsg->wParam - '0'));
        } else if (pMsg->wParam >= VK_NUMPAD0 && pMsg->wParam <= VK_NUMPAD9) {
            m_wChip8VMKeys &=~(1 << (pMsg->wParam - VK_NUMPAD0));
        } else if (pMsg->wParam >= 'a' && pMsg->wParam <= 'f') {
            m_wChip8VMKeys &=~(1 << (0xa + pMsg->wParam - 'a'));
        } else if (pMsg->wParam >= 'A' && pMsg->wParam <= 'F') {
            m_wChip8VMKeys &=~(1 << (0xa + pMsg->wParam - 'A'));
        }
        chip8vm_key(m_pChip8VMCtxt, m_wChip8VMKeys);
    }
    return CDialog::PreTranslateMessage(pMsg);
}

void Cchip8Dlg::OnTimer(UINT_PTR nIDEvent)
{
    // TODO: Add your message handler code here and/or call default
    switch (nIDEvent) {
    case TIMER_OPEN_FILE:
        KillTimer(nIDEvent);
        OpenRomFile(NULL);
        break;
    }
    CDialog::OnTimer(nIDEvent);
}

void Cchip8Dlg::OnOpenRomFile()
{
    OpenRomFile(NULL);
}
