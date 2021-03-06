///////////////////////////////////////////////////
// Classes.cpp - Definitions for the CViewClasses, CContainClasses
//               and CDockClasses classes


#include "stdafx.h"
#include "Classes.h"
#include "ContainerApp.h"
#include "resource.h"


///////////////////////////////////////////////
// CViewClasses functions
CViewClasses::CViewClasses()
{
}

CViewClasses::~CViewClasses()
{
	if (IsWindow()) DeleteAllItems();
}

HTREEITEM CViewClasses::AddItem(HTREEITEM hParent, LPCTSTR szText, int iImage)
{
	TVITEM tvi;
	ZeroMemory(&tvi, sizeof(TVITEM));
	tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tvi.iImage = iImage;
	tvi.iSelectedImage = iImage;
	tvi.pszText = (LPTSTR)szText;

	TVINSERTSTRUCT tvis;
	ZeroMemory(&tvis, sizeof(TVINSERTSTRUCT));
	tvis.hParent = hParent;
	tvis.item = tvi;

	return InsertItem(tvis);
}

void CViewClasses::OnAttach()
{
	//set the image lists
	m_imlNormal.Create(16, 15, ILC_COLOR32 | ILC_MASK, 1, 0);
	CBitmap bm(IDB_CLASSVIEW);
	m_imlNormal.Add( bm, RGB(255, 0, 0) );
	SetImageList(m_imlNormal, LVSIL_NORMAL);

	// Adjust style to show lines and [+] button
	DWORD dwStyle = (DWORD)GetWindowLongPtr(GWL_STYLE);
	dwStyle |= TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT;
	SetWindowLongPtr(GWL_STYLE, dwStyle);

	DeleteAllItems();
}

void CViewClasses::OnDestroy()
{
	SetImageList(NULL, LVSIL_SMALL);
}

LRESULT CViewClasses::OnMouseActivate(UINT uMsg, WPARAM wParam, LPARAM lParam)
// Respond to a mouse click on the window
{
	// Set window focus. The docker will now report this as active.
	SetFocus();
	return FinalWindowProc(uMsg, wParam, lParam);
}

void CViewClasses::PreCreate(CREATESTRUCT& cs)
{
	cs.style = TVS_NOTOOLTIPS|WS_CHILD;
	cs.lpszClass = WC_TREEVIEW;
}

LRESULT CViewClasses::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_MOUSEACTIVATE:		return OnMouseActivate(uMsg, wParam, lParam);
	}

	return WndProcDefault(uMsg, wParam, lParam);
}



///////////////////////////////////////////////
// CContainClasses functions
CContainClasses::CContainClasses() 
{
	SetTabText(_T("ClassView"));
	SetTabIcon(IDI_CLASSVIEW);
	SetDockCaption (_T("Class View - Docking container"));
	SetView(m_ViewClasses);
}

void CContainClasses::AddCombo()
{
	int nComboWidth = 120; 
	CToolBar& TB = GetToolBar();
	if (TB.CommandToIndex(IDM_FILE_SAVE) < 0) return;
	 
	// Adjust button width and convert to separator   
	TB.SetButtonStyle(IDM_FILE_SAVE, TBSTYLE_SEP);
	TB.SetButtonWidth(IDM_FILE_SAVE, nComboWidth);
	 
	// Determine the size and position of the ComboBox 
	int nIndex = TB.CommandToIndex(IDM_FILE_SAVE); 
	CRect rect = TB.GetItemRect(nIndex); 
	 
	// Create the ComboboxEx window 
	m_ComboBoxEx.Create(TB);
	m_ComboBoxEx.SetWindowPos(NULL, rect, SWP_NOACTIVATE);

	// Adjust the toolbar height to accomodate the ComboBoxEx control
	CRect rc = m_ComboBoxEx.GetWindowRect();
	TB.SetButtonSize( rc.Height(), rc.Height() );
	
	// Add the ComboBox's items
	m_ComboBoxEx.AddItems();
}

BOOL CContainClasses::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	// OnCommand responds to menu and and toolbar input
	UINT nID = LOWORD(wParam);
	switch(nID)
	{
	case IDM_FILE_NEW:		return OnFileNew();
	case IDM_HELP_ABOUT:	return OnHelpAbout();
	}

	return FALSE;
}

BOOL CContainClasses::OnFileNew()
{
	TRACE("File New\n");
	MessageBox(_T("File New"), _T("Button Pressed"), MB_OK);
	return TRUE;
}

BOOL CContainClasses::OnHelpAbout()
{
	// Send a message to the frame requesting the help dialog
	GetContainerApp().GetMainFrame().SendMessage(WM_HELP);
	return TRUE;
}

void CContainClasses::SetupToolBar()
{
	// Set the Bitmap resource for the toolbar
	SetToolBarImages(RGB(192,192,192), IDW_MAIN, 0, 0);
	
	// Set the Resource IDs for the toolbar buttons
	AddToolBarButton( IDM_FILE_NEW         );
	AddToolBarButton( IDM_FILE_OPEN, FALSE );
	
	AddToolBarButton( 0 );	// Separator
	AddToolBarButton( IDM_FILE_SAVE, FALSE );
	
	AddToolBarButton( 0 );	// Separator
	AddToolBarButton( IDM_EDIT_CUT         );
	AddToolBarButton( IDM_EDIT_COPY        );
	AddToolBarButton( IDM_EDIT_PASTE       );
	AddToolBarButton( 0 );	// Separator
	AddToolBarButton( IDM_HELP_ABOUT       );

	// Add the ComboBarEx control to the toolbar
	AddCombo();
}


/////////////////////////////////////////////////
//  Definitions for the CDockClasses class
CDockClasses::CDockClasses() 
{ 
	SetView(m_Classes); 

	// Set the width of the splitter bar
	SetBarWidth(8);
}

