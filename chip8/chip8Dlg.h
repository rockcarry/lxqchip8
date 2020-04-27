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
    DECLARE_MESSAGE_MAP()

private:
    HDC     m_hMemDC;
    HBITMAP m_hMemBitmap;
    void   *m_pMemBitmap;
    void   *m_pChip8VMCtxt;
    BOOL    m_bExitChip8VM;
    HANDLE  m_hChip8VMThread;
    WORD    m_wChip8VMKeys;

public:
    void SetWindowClientSize(int w, int h);
    void DoRunChip8VM();
    void OpenRomFile(char *file);
};
