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
    ON_WM_SIZE()
    ON_WM_CLOSE()
    ON_WM_LBUTTONDBLCLK()
END_MESSAGE_MAP()


static DWORD WINAPI Chip8VMThreadProc(LPVOID pParam)
{
    Cchip8Dlg *dlg = (Cchip8Dlg*)pParam;
    dlg->DoRunChip8VM();
    return 0;
}

static void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    Cchip8Dlg *dlg = (Cchip8Dlg*)dwInstance;
    dlg->DoWaveOutProc(hwo, uMsg, dwInstance, dwParam1, dwParam2);
}

void Cchip8Dlg::DoRunChip8VM()
{
    DWORD next_tick = 0;
    int   sleep_tick= 0;
    int   i;
    while (!m_bExitChip8VM) {
        if (!next_tick) next_tick = GetTickCount();
        next_tick += 1000 / 60;

        // run chip8vm
        for (i=0; i<CHIP8VM_RUNNING_SPEED; i++) chip8vm_run(m_pChip8VMCtxt, !i);

        // render video
        chip8vm_getparam(m_pChip8VMCtxt, CHIP8VM_PARAM_CHIP8_LRES, &m_bChip8LRes);
        if (chip8vm_render(m_pChip8VMCtxt, m_pMemBitmap)) {
            InvalidateRect(NULL, 0);
        }

        // render audio
        DWORD soundtimer = 0;
        chip8vm_getparam(m_pChip8VMCtxt, CHIP8VM_PARAM_SOUND_TIMER, &soundtimer);
        if (soundtimer) {
            WaitForSingleObject(m_hWaveSem, -1);
            waveOutWrite(m_hWaveOut, &m_pWaveHdr[m_nWaveTail], sizeof(WAVEHDR));
            if (++m_nWaveTail == WAVE_BUF_NUM) m_nWaveTail = 0;
        }

        // chip8vm speed control
        sleep_tick = next_tick - GetTickCount();
        if (sleep_tick > 0) Sleep(sleep_tick );
        else if (sleep_tick < -1000) next_tick = 0;
    }
}

void Cchip8Dlg::DoWaveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    switch (uMsg) {
    case WOM_DONE:
        ReleaseSemaphore(m_hWaveSem, 1, NULL);
        if (++m_nWaveHead == WAVE_BUF_NUM) m_nWaveHead = 0;
        break;
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
    SetWindowClientSize(CHIP8VM_RENDER_WIDTH, CHIP8VM_RENDER_HEIGHT);

    BITMAPINFO bmpinfo = {0};
    bmpinfo.bmiHeader.biSize        =  sizeof(BITMAPINFOHEADER);
    bmpinfo.bmiHeader.biWidth       =  CHIP8VM_RENDER_WIDTH ;
    bmpinfo.bmiHeader.biHeight      = -CHIP8VM_RENDER_HEIGHT;
    bmpinfo.bmiHeader.biPlanes      =  1;
    bmpinfo.bmiHeader.biBitCount    =  32;
    bmpinfo.bmiHeader.biCompression =  BI_RGB;
    CDC *pDC = GetDC();
    m_hMemDC     = CreateCompatibleDC(pDC->m_hDC);
    m_hMemBitmap = CreateDIBSection(m_hMemDC, &bmpinfo, DIB_RGB_COLORS, (void**)&m_pMemBitmap, NULL, 0);
    SelectObject(m_hMemDC, m_hMemBitmap);
    ReleaseDC(pDC);

    // init for audio
    WAVEFORMATEX   wfx  = {0};
    wfx.cbSize          = sizeof(wfx);
    wfx.wFormatTag      = WAVE_FORMAT_PCM;
    wfx.wBitsPerSample  = 16;    // 16bit
    wfx.nSamplesPerSec  = WAVE_SAMPRATE;
    wfx.nChannels       = 1;     // stereo
    wfx.nBlockAlign     = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
    m_nWaveHead= m_nWaveTail = 0;
    m_hWaveSem = CreateSemaphore(NULL, WAVE_BUF_NUM, WAVE_BUF_NUM, NULL);
    waveOutOpen(&m_hWaveOut, WAVE_MAPPER, &wfx, (DWORD_PTR)waveOutProc, (DWORD_PTR)this, CALLBACK_FUNCTION);

    // init wavebuf
    m_pWaveHdr    = (WAVEHDR*)calloc(WAVE_BUF_NUM, sizeof(WAVEHDR) + WAVE_BUF_LEN);
    BYTE *pwavbuf = (BYTE*)(m_pWaveHdr + WAVE_BUF_NUM);
    for (int i=0; i<WAVE_BUF_NUM; i++) {
        m_pWaveHdr[i].lpData         = (LPSTR)(pwavbuf + i * WAVE_BUF_LEN);
        m_pWaveHdr[i].dwBufferLength = WAVE_BUF_LEN;
        waveOutPrepareHeader(m_hWaveOut, &m_pWaveHdr[i], sizeof(WAVEHDR));
        for (int j=0; j<WAVE_BUF_LEN/2; j++) {
            ((SHORT*)m_pWaveHdr[i].lpData)[j] = j % 10 < 5 ? 20000 : -20000;
        }
    }

    // start waveout
    waveOutRestart(m_hWaveOut);

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

    // unprepare wave header & close waveout device
    if (m_hWaveOut) {
        waveOutReset(m_hWaveOut);
        for (int i=0; i<WAVE_BUF_NUM; i++) {
            waveOutUnprepareHeader(m_hWaveOut, &m_pWaveHdr[i], sizeof(WAVEHDR));
        }
        waveOutClose(m_hWaveOut);
    }
    if (m_hWaveSem) CloseHandle(m_hWaveSem);
    if (m_pWaveHdr) free(m_pWaveHdr);

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
        CPaintDC dc(this);
        StretchBlt(dc, m_tChip8VMRender.left, m_tChip8VMRender.top, m_tChip8VMRender.right, m_tChip8VMRender.bottom,
            m_hMemDC, 0, 0, CHIP8VM_RENDER_WIDTH >> m_bChip8LRes, CHIP8VM_RENDER_HEIGHT >> m_bChip8LRes, SRCCOPY);
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
        if (pMsg->wParam >= VK_NUMPAD0 && pMsg->wParam <= VK_DIVIDE) {
            m_wChip8VMKeys |= (1 << (pMsg->wParam - VK_NUMPAD0));
        }
        chip8vm_key(m_pChip8VMCtxt, m_wChip8VMKeys);
    } else if (pMsg->message == WM_KEYUP) {
        if (pMsg->wParam >= VK_NUMPAD0 && pMsg->wParam <= VK_DIVIDE) {
            m_wChip8VMKeys &=~(1 << (pMsg->wParam - VK_NUMPAD0));
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

void Cchip8Dlg::OnSize(UINT nType, int cx, int cy)
{
    CDialog::OnSize(nType, cx, cy);
    RECT client_rect;
    GetClientRect(&client_rect     );
    GetClientRect(&m_tChip8VMRender);
    int clientw = m_tChip8VMRender.right, clienth = m_tChip8VMRender.bottom;
    if (m_tChip8VMRender.right * CHIP8VM_RENDER_HEIGHT > m_tChip8VMRender.bottom * CHIP8VM_RENDER_WIDTH) {
        m_tChip8VMRender.right  = m_tChip8VMRender.bottom * CHIP8VM_RENDER_WIDTH / CHIP8VM_RENDER_HEIGHT;
        m_tChip8VMRender.left   = (clientw - m_tChip8VMRender.right ) / 2;
    } else {
        m_tChip8VMRender.bottom = m_tChip8VMRender.right * CHIP8VM_RENDER_HEIGHT / CHIP8VM_RENDER_WIDTH;
        m_tChip8VMRender.top    = (clienth - m_tChip8VMRender.bottom) / 2;
    }
    InvalidateRect(NULL, TRUE);
}

void Cchip8Dlg::OnCancel() {}
void Cchip8Dlg::OnOK()     {}
void Cchip8Dlg::OnClose()
{
    CDialog::OnClose();
    EndDialog(IDCANCEL);
}

void Cchip8Dlg::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    CDialog::OnLButtonDblClk(nFlags, point);
    chip8vm_dump(m_pChip8VMCtxt);
}
