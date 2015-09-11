///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_GRAPHIC_ITEMS_OPTIONS_BASE_H__
#define __DIALOG_GRAPHIC_ITEMS_OPTIONS_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class DIALOG_SHIM;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_GRAPHIC_ITEMS_OPTIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_GRAPHIC_ITEMS_OPTIONS_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxStaticText* m_GraphicSegmWidthTitle;
		wxTextCtrl* m_OptPcbSegmWidth;
		wxStaticText* m_BoardEdgesWidthTitle;
		wxTextCtrl* m_OptPcbEdgesWidth;
		wxStaticText* m_CopperTextWidthTitle;
		wxTextCtrl* m_OptPcbTextWidth;
		wxStaticText* m_TextSizeVTitle;
		wxTextCtrl* m_OptPcbTextVSize;
		wxStaticText* m_TextSizeHTitle;
		wxTextCtrl* m_OptPcbTextHSize;
		wxStaticText* m_EdgeModWidthTitle;
		wxTextCtrl* m_OptModuleEdgesWidth;
		wxStaticText* m_TextModWidthTitle;
		wxTextCtrl* m_OptModuleTextWidth;
		wxStaticText* m_TextModSizeVTitle;
		wxTextCtrl* m_OptModuleTextVSize;
		wxStaticText* m_TextModSizeHTitle;
		wxTextCtrl* m_OptModuleTextHSize;
		wxStaticText* m_DefaultPenSizeTitle;
		wxTextCtrl* m_DefaultPenSizeCtrl;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_GRAPHIC_ITEMS_OPTIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Text and Drawings"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_GRAPHIC_ITEMS_OPTIONS_BASE();
	
};

#endif //__DIALOG_GRAPHIC_ITEMS_OPTIONS_BASE_H__
