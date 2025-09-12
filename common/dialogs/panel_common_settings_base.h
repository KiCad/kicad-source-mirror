///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-75-g9786507b-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class STD_BITMAP_BUTTON;

#include "widgets/resettable_panel.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/statline.h>
#include <wx/radiobut.h>
#include <wx/sizer.h>
#include <wx/choice.h>
#include <wx/gbsizer.h>
#include <wx/textctrl.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/spinctrl.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_COMMON_SETTINGS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_COMMON_SETTINGS_BASE : public RESETTABLE_PANEL
{
	private:

	protected:
		wxBoxSizer* bLeftSizer;
		wxStaticText* m_staticText20;
		wxStaticLine* m_staticline3;
		wxFlexGridSizer* m_renderingSizer;
		wxRadioButton* m_rbAccelerated;
		wxRadioButton* m_rbFallback;
		wxChoice* m_antialiasing;
		wxStaticText* m_staticText21;
		wxStaticLine* m_staticline2;
		wxTextCtrl* m_textEditorPath;
		STD_BITMAP_BUTTON* m_textEditorBtn;
		wxBoxSizer* bSizerFileManager;
		wxStaticText* m_staticTextFileManager;
		wxTextCtrl* m_textCtrlFileManager;
		wxRadioButton* m_defaultPDFViewer;
		wxRadioButton* m_otherPDFViewer;
		wxTextCtrl* m_PDFViewerPath;
		STD_BITMAP_BUTTON* m_pdfViewerBtn;
		wxStaticText* m_staticText22;
		wxStaticLine* m_staticline1;
		wxCheckBox* m_checkBoxIconsInMenus;
		wxCheckBox* m_showScrollbars;
		wxCheckBox* m_focusFollowSchPcb;
		wxCheckBox* m_hotkeyFeedback;
		wxCheckBox* m_gridStriping;
		wxCheckBox* m_disableCustomCursors;
		wxStaticText* m_stIconTheme;
		wxRadioButton* m_rbIconThemeLight;
		wxRadioButton* m_rbIconThemeDark;
		wxRadioButton* m_rbIconThemeAuto;
		wxStaticText* m_stToolbarIconSize;
		wxRadioButton* m_rbIconSizeSmall;
		wxRadioButton* m_rbIconSizeNormal;
		wxRadioButton* m_rbIconSizeLarge;
		wxCheckBox* m_scaleFonts;
		wxStaticText* m_fontScalingHelp;
		wxGridBagSizer* m_gbUserInterface;
		wxStaticText* m_staticTextCanvasScale;
		wxSpinCtrlDouble* m_canvasScaleCtrl;
		wxCheckBox* m_canvasScaleAuto;
		wxStaticText* m_highContrastLabel;
		wxTextCtrl* m_highContrastCtrl;
		wxStaticText* m_highContrastUnits;
		wxStaticText* m_staticText251;
		wxStaticLine* m_staticline7;
		wxStaticText* m_staticText23;
		wxStaticLine* m_staticline6;
		wxCheckBox* m_warpMouseOnMove;
		wxCheckBox* m_NonImmediateActions;
		wxStaticText* m_staticText24;
		wxStaticLine* m_staticline5;
		wxCheckBox* m_cbRememberOpenFiles;
		wxStaticText* m_staticTextFileHistorySize;
		wxSpinCtrl* m_fileHistorySize;
		wxStaticText* m_staticText25;
		wxStaticLine* m_staticline4;
		wxCheckBox* m_cbBackupEnabled;
		wxStaticText* m_staticText16;
		wxSpinCtrl* m_backupLimitTotalSize;
		wxStaticText* m_staticText17;

		// Virtual event handlers, override them in your derived class
		virtual void OnTextEditorClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRadioButtonPdfViewer( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPDFViewerClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCanvasScaleAuto( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_COMMON_SETTINGS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_COMMON_SETTINGS_BASE();

};

