#include "pch.h"
#include "MFCEditBrowseCtrlEx.h"
#include "DPIManager.h"

static DPIManager g_dpi;

CImageList* ResizeImageList(CImageList* pImageList, int nWidth, int nHeight)
{
	if (pImageList == nullptr)
		return nullptr;

	// Get the number of images in the list
	int nImageCount = pImageList->GetImageCount();

	// Create a new image list with the specified size
	CImageList* pNewImageList = new CImageList();
	pNewImageList->Create(nWidth, nHeight, ILC_COLOR32 | ILC_MASK, nImageCount, 0);

	for (int i = 0; i < nImageCount; ++i)
	{
		// Extract the image and mask from the original list
		HICON hIcon = pImageList->ExtractIcon(i);
		pNewImageList->Add(hIcon);
		DestroyIcon(hIcon);
	}

	return pNewImageList;
}

void resizeImageListDPI(CImageList& imageList)
{
	int nWidth;
	int nHeight;
	IMAGEINFO imageInfo;
	imageList.GetImageInfo(0, &imageInfo);
	nWidth = imageInfo.rcImage.right - imageInfo.rcImage.left;
	nHeight = imageInfo.rcImage.bottom - imageInfo.rcImage.top;
	// 	int imageCount = imageList.GetImageCount();
	// 	nWidth /= imageCount;

	int newWidth = g_dpi.scaleX(nWidth);
	int newHeight = g_dpi.scaleY(nHeight);

	// resized list
	CImageList* newList = ResizeImageList(&imageList, newWidth, newHeight);

	// replace list
	imageList.DeleteImageList();
	imageList.Create(newList);

	delete newList;
}


IMPLEMENT_DYNAMIC(MFCEditBrowseCtrlEx, CMFCEditBrowseCtrl)

BEGIN_MESSAGE_MAP(MFCEditBrowseCtrlEx, CMFCEditBrowseCtrl)
	ON_WM_NCPAINT()
	ON_WM_NCCALCSIZE()
END_MESSAGE_MAP()

void MFCEditBrowseCtrlEx::OnBrowse()
{
	ASSERT_VALID(this);
	ENSURE(GetSafeHwnd() != NULL);

	switch (m_Mode)
	{
	case BrowseMode_Folder:
		if (afxShellManager != NULL)
		{
			CString strFolder;
			GetWindowText(strFolder);

			// Use the much better folder picker

			CFolderPickerDialog dlg(NULL,m_ulBrowseFolderFlags);
			INT_PTR result = dlg.DoModal();
			if (result == IDOK) {
				SetWindowText(dlg.GetPathName());
				SetModify(TRUE);
				OnAfterUpdate();
			}

// 			CString strResult;
// 			if (afxShellManager->BrowseForFolder(strResult, this, strFolder, m_strBrowseFolderTitle.IsEmpty() ? NULL : (LPCTSTR)m_strBrowseFolderTitle, m_ulBrowseFolderFlags) &&
// 				(strResult != strFolder))
// 			{
// 				SetWindowText(strResult);
// 				SetModify(TRUE);
// 				OnAfterUpdate();
// 			}
		}
		else
		{
			ASSERT(FALSE);
		}
		break;

	case BrowseMode_File:
	{
		CString strFile;
		GetWindowText(strFile);

		if (!strFile.IsEmpty())
		{
			TCHAR fname[_MAX_FNAME];

			_tsplitpath_s(strFile, NULL, 0, NULL, 0, fname, _MAX_FNAME, NULL, 0);

			CString strFileName = fname;
			strFileName.TrimLeft();
			strFileName.TrimRight();

			if (strFileName.IsEmpty())
			{
				strFile.Empty();
			}

			const CString strInvalidChars = _T("*?<>|");
			if (strFile.FindOneOf(strInvalidChars) >= 0)
			{
				if (!OnIllegalFileName(strFile))
				{
					SetFocus();
					return;
				}
			}
		}

		CFileDialog dlg(TRUE, !m_strDefFileExt.IsEmpty() ? (LPCTSTR)m_strDefFileExt : (LPCTSTR)NULL, strFile, m_dwFileDialogFlags, !m_strFileFilter.IsEmpty() ? (LPCTSTR)m_strFileFilter : (LPCTSTR)NULL, NULL);
		if (dlg.DoModal() == IDOK && strFile != dlg.GetPathName())
		{
			SetWindowText(dlg.GetPathName());
			SetModify(TRUE);
			OnAfterUpdate();
		}

		if (GetParent() != NULL)
		{
			GetParent()->RedrawWindow(NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
		}
	}
	break;
	}

	SetFocus();
}



void MFCEditBrowseCtrlEx::OnNcPaintBase()
{
	CEdit::OnNcPaint();

	if (m_Mode == BrowseMode_None)
	{
		return;
	}

	CWindowDC dc(this);

	CRect rectWindow;
	GetWindowRect(rectWindow);

	m_rectBtn = rectWindow;
	m_rectBtn.left = m_rectBtn.right - g_dpi.scaleX(m_nBrowseButtonWidth);

	CRect rectClient;
	GetClientRect(rectClient);
	ClientToScreen(&rectClient);

	m_rectBtn.OffsetRect(rectClient.right + g_dpi.scaleX(m_nBrowseButtonWidth) - rectWindow.right, 0);
	m_rectBtn.top += rectClient.top - rectWindow.top;
	m_rectBtn.bottom -= rectWindow.bottom - rectClient.bottom;

	CRect rect = m_rectBtn;
	rect.OffsetRect(-rectWindow.left, -rectWindow.top);

	CRgn rgnClip;
	rgnClip.CreateRectRgnIndirect(&rect);

	dc.SelectClipRgn(&rgnClip);

	OnDrawBrowseButton(&dc, rect, m_bIsButtonPressed, m_bIsButtonHighlighted);

	dc.SelectClipRgn(NULL);

	ScreenToClient(&m_rectBtn);
}

void MFCEditBrowseCtrlEx::OnNcPaint()
{
	OnNcPaintBase();
}



void MFCEditBrowseCtrlEx::OnDrawBrowseButtonPriv(CDC* pDC, CRect rect, BOOL bIsButtonPressed, BOOL bHighlight)
{
	ASSERT(m_Mode != BrowseMode_None);
	ASSERT_VALID(pDC);

	CMFCVisualManager::AFX_BUTTON_STATE state = CMFCVisualManager::ButtonsIsRegular;

	if (bIsButtonPressed)
	{
		state = CMFCVisualManager::ButtonsIsPressed;
	}
	else if (bHighlight)
	{
		state = CMFCVisualManager::ButtonsIsHighlighted;
	}

	COLORREF clrText = GetGlobalData()->clrBtnText;
	
	if (!CMFCVisualManager::GetInstance()->OnDrawBrowseButton(pDC, rect, this, state, clrText))
	{
		return;
	}


	int iImage = 0;

	if (m_ImageBrowse.GetSafeHandle() != NULL)
	{
		if (m_bDefaultImage)
		{
			switch (m_Mode)
			{
			case BrowseMode_Folder:
				iImage = 0;
				break;

			case BrowseMode_File:
				iImage = 1;
				break;
			}
		}

		CPoint ptImage;
		ptImage.x = rect.CenterPoint().x - m_sizeImage.cx / 2;
		ptImage.y = rect.CenterPoint().y - m_sizeImage.cy / 2;

		if (bIsButtonPressed && CMFCVisualManager::GetInstance()->IsOffsetPressedButton())
		{
			ptImage.x++;
			ptImage.y++;
		}

		ImageList_Draw(m_ImageBrowse.m_hImageList, iImage, pDC->m_hDC, ptImage.x, ptImage.y, ILD_NORMAL);
		//m_ImageBrowse.Draw(pDC, iImage, ptImage, ILD_NORMAL);
	}
	else
	{
		COLORREF clrTextOld = pDC->SetTextColor(clrText);
		int nTextMode = pDC->SetBkMode(TRANSPARENT);
		CFont* pFont = (CFont*)pDC->SelectStockObject(DEFAULT_GUI_FONT);

		CRect rectText = rect;
		rectText.DeflateRect(1, 2);
		rectText.OffsetRect(0, -2);

		if (bIsButtonPressed)
		{
			rectText.OffsetRect(1, 1);
		}

		pDC->DrawText(_T("..."), rectText, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

		pDC->SetTextColor(clrTextOld);
		pDC->SetBkMode(nTextMode);
		pDC->SelectObject(pFont);
	}
}

void MFCEditBrowseCtrlEx::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS FAR* lpncsp)
{
	CEdit::OnNcCalcSize(bCalcValidRects, lpncsp);

	if (m_Mode != BrowseMode_None)
	{
		lpncsp->rgrc[0].right -= g_dpi.scaleX(m_nBrowseButtonWidth);
	}
}

void MFCEditBrowseCtrlEx::EnableFileBrowseButton(LPCTSTR lpszDefExt/* = NULL*/, LPCTSTR lpszFilter/* = NULL*/, DWORD dwFlags/* = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT*/)
{
	CMFCEditBrowseCtrl::EnableFileBrowseButton(lpszDefExt, lpszFilter, dwFlags);
	resizeImageListDPI(m_ImageBrowse);
	m_sizeImage = CSize(g_dpi.scaleX(16), g_dpi.scaleX(16));
}

void MFCEditBrowseCtrlEx::EnableFolderBrowseButton(LPCTSTR lpszBrowseFolderTitle/* = NULL */, UINT ulBrowseFolderFlags/* = BIF_RETURNONLYFSDIRS */)
{
	CMFCEditBrowseCtrl::EnableFolderBrowseButton(lpszBrowseFolderTitle, ulBrowseFolderFlags);
	resizeImageListDPI(m_ImageBrowse);
	m_sizeImage = CSize(g_dpi.scaleX(16), g_dpi.scaleX(16));
}


