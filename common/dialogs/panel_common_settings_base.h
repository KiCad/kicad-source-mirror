///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include "widgets/stepped_slider.h"
#include "widgets/resettable_panel.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/choice.h>
#include <wx/gbsizer.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/textctrl.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/radiobut.h>
#include <wx/slider.h>
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
		wxChoice* m_antialiasing;
		wxStaticText* m_antialiasingFallbackLabel;
		wxChoice* m_antialiasingFallback;
		wxTextCtrl* m_textEditorPath;
		wxBitmapButton* m_textEditorBtn;
		wxRadioButton* m_defaultPDFViewer;
		wxRadioButton* m_otherPDFViewer;
		wxTextCtrl* m_PDFViewerPath;
		wxBitmapButton* m_pdfViewerBtn;
		wxStaticText* m_stIconTheme;
		wxRadioButton* m_rbIconThemeLight;
		wxRadioButton* m_rbIconThemeDark;
		wxRadioButton* m_rbIconThemeAuto;
		wxStaticText* m_staticTexticonscale;
		STEPPED_SLIDER* m_iconScaleSlider;
		wxCheckBox* m_iconScaleAuto;
		wxStaticText* m_staticTextCanvasScale;
		wxSpinCtrlDouble* m_canvasScaleCtrl;
		wxCheckBox* m_canvasScaleAuto;
		wxCheckBox* m_checkBoxIconsInMenus;
		wxCheckBox* m_scaleFonts;
		wxStaticText* m_fontScalingHelp;
		wxCheckBox* m_warpMouseOnMove;
		wxCheckBox* m_NonImmediateActions;
		wxCheckBox* m_cbRememberOpenFiles;
		wxStaticText* m_staticTextautosave;
		wxSpinCtrl* m_SaveTime;
		wxStaticText* m_staticTextFileHistorySize;
		wxSpinCtrl* m_fileHistorySize;
		wxStaticText* m_staticTextClear3DCache;
		wxSpinCtrl* m_Clear3DCacheFilesOlder;
		wxStaticText* m_staticTextDays;
		wxCheckBox* m_cbBackupEnabled;
		wxCheckBox* m_cbBackupAutosave;
		wxStaticText* m_staticText9;
		wxSpinCtrl* m_backupLimitTotalFiles;
		wxStaticText* m_staticText10;
		wxSpinCtrl* m_backupLimitDailyFiles;
		wxStaticText* m_staticText11;
		wxSpinCtrl* m_backupMinInterval;
		wxStaticText* m_staticText15;
		wxStaticText* m_staticText16;
		wxSpinCtrl* m_backupLimitTotalSize;
		wxStaticText* m_staticText17;

		// Virtual event handlers, overide them in your derived class
		virtual void OnTextEditorClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void onUpdateUIPdfPath( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnPDFViewerClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnScaleSlider( wxScrollEvent& event ) { event.Skip(); }
		virtual void OnIconScaleAuto( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCanvasScaleAuto( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_COMMON_SETTINGS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
		~PANEL_COMMON_SETTINGS_BASE();

};

