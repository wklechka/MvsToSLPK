#pragma once
#include <afxeditbrowsectrl.h>


class MFCEditBrowseCtrlEx :public CMFCEditBrowseCtrl
{
	DECLARE_DYNAMIC(MFCEditBrowseCtrlEx)

	virtual void OnBrowse();


	void EnableFileBrowseButton(LPCTSTR lpszDefExt = NULL, LPCTSTR lpszFilter = NULL, DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT);
	void EnableFolderBrowseButton(LPCTSTR lpszBrowseFolderTitle = NULL, UINT ulBrowseFolderFlags = BIF_RETURNONLYFSDIRS);

	void OnDrawBrowseButton(CDC* pDC, CRect rect, BOOL bIsButtonPressed, BOOL bIsButtonHot) override
	{
		CMemDC dc(*pDC, rect);
		OnDrawBrowseButtonPriv(&dc.GetDC(), rect, bIsButtonPressed, bIsButtonHot);
	}

	void OnNcPaintBase();
	afx_msg void OnNcPaint();

protected:
	DECLARE_MESSAGE_MAP()

	void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS FAR* lpncsp);
	void paintFrame(HDC hdc, CRect& rcClient);

	void OnDrawBrowseButtonPriv(CDC* pDC, CRect rect, BOOL bIsButtonPressed, BOOL bHighlight);
};

