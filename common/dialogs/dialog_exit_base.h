///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version May  6 2016)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_EXIT_BASE_H__
#define __DIALOG_EXIT_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class DIALOG_SHIM;

#include "dialog_shim.h"
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/statline.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_EXIT_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_EXIT_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxStaticBitmap* m_bitmap;
		wxStaticText* m_TextInfo;
		wxStaticText* m_staticText2;
		wxStaticLine* m_staticline;
		wxButton* m_buttonSaveAndExit;
		wxButton* m_buttonExitNoSave;
		wxButton* m_buttonCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnSaveAndExit( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnExitNoSave( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_EXIT_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_EXIT_BASE();
	
};

#endif //__DIALOG_EXIT_BASE_H__
