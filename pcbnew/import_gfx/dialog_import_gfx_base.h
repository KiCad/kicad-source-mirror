///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jul 11 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_IMPORT_GFX_BASE_H__
#define __DIALOG_IMPORT_GFX_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class PCB_LAYER_BOX_SELECTOR;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/choice.h>
#include <wx/radiobox.h>
#include <wx/valtext.h>
#include <wx/bmpcbox.h>
#include <wx/statline.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

#define wxID_ORIGIN_SELECT 1000

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_IMPORT_GFX_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_IMPORT_GFX_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxStaticText* m_staticText37;
		wxTextCtrl* m_textCtrlFileName;
		wxButton* m_buttonBrowse;
		wxStaticText* m_staticText3;
		wxChoice* m_PCBGridUnits;
		wxRadioBox* m_rbOffsetOption;
		wxStaticText* m_staticText4;
		wxTextCtrl* m_PCBXCoord;
		wxStaticText* m_staticText5;
		wxTextCtrl* m_PCBYCoord;
		wxStaticText* m_staticTextScale;
		wxTextCtrl* m_tcScale;
		wxStaticText* m_staticTextBrdlayer;
		PCB_LAYER_BOX_SELECTOR* m_SelLayerBox;
		wxStaticLine* m_staticline8;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnBrowseFiles( wxCommandEvent& event ) { event.Skip(); }
		virtual void OriginOptionOnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onChangeHeight( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOKClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_IMPORT_GFX_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Import vector graphics file"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_IMPORT_GFX_BASE();
	
};

#endif //__DIALOG_IMPORT_GFX_BASE_H__
