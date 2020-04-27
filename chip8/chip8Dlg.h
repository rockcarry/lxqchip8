// chip8Dlg.h : header file
//
#pragma once

// Cchip8Dlg dialog
class Cchip8Dlg : public CDialog
{
// Construction
public:
    Cchip8Dlg(CWnd* pParent = NULL);    // standard constructor

// Dialog Data
    enum { IDD = IDD_CHIP8_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL PreTranslateMessage(MSG* pMsg);


// Implementation
protected:
    HICON  m_hIcon;
    HACCEL m_hAcc;

    // Generated message map functions
    virtual BOOL OnInitDialog();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg void OnDestroy();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnOpenRomFile();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    DECLARE_MESSAGE_MAP()

private:
    HDC      m_hMemDC;
    HBITMAP  m_hMemBitmap;
    void    *m_pMemBitmap;
    void    *m_pChip8VMCtxt;
    BOOL     m_bExitChip8VM;
    HANDLE   m_hChip8VMThread;
    WORD     m_wChip8VMKeys;
    RECT     m_tChip8VMRender;
    int      m_bChip8LRes;

    #define WAVE_SAMPRATE 8000
    #define WAVE_BUF_NUM  3
    #define WAVE_BUF_LEN (WAVE_SAMPRATE / 60 * 2)
    HWAVEOUT m_hWaveOut;
    WAVEHDR *m_pWaveHdr;
    HANDLE   m_hWaveSem;
    int      m_nWaveHead;
    int      m_nWaveTail;

public:
    void SetWindowClientSize(int w, int h);
    void DoRunChip8VM();
    void DoWaveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
    void OpenRomFile(char *file);
};
