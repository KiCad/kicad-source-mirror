///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 17 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_3D_VIEW_OPTION_BASE_H__
#define __DIALOG_3D_VIEW_OPTION_BASE_H__

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
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/statline.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_3D_VIEW_OPTIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_3D_VIEW_OPTIONS_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxStaticBitmap* m_bitmapRealisticMode;
		wxCheckBox* m_checkBoxRealisticMode;
		wxStaticBitmap* m_bitmapCuThickness;
		wxCheckBox* m_checkBoxCuThickness;
		wxStaticBitmap* m_bitmap3Dshapes;
		wxCheckBox* m_checkBox3Dshapes;
		wxStaticBitmap* m_bitmapAreas;
		wxCheckBox* m_checkBoxAreas;
		wxStaticBitmap* m_bitmapSilkscreen;
		wxCheckBox* m_checkBoxSilkscreen;
		wxStaticBitmap* m_bitmapSolderMask;
		wxCheckBox* m_checkBoxSolderMask;
		wxStaticBitmap* m_bitmapSolderPaste;
		wxCheckBox* m_checkBoxSolderpaste;
		wxStaticBitmap* m_bitmapAdhesive;
		wxCheckBox* m_checkBoxAdhesive;
		wxStaticBitmap* m_bitmapComments;
		wxCheckBox* m_checkBoxComments;
		wxStaticBitmap* m_bitmapECO;
		wxCheckBox* m_checkBoxECO;
		wxButton* m_buttonShowAll;
		wxButton* m_buttonShowNone;
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnCheckRealisticMode( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnShowAllClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnShowNoneClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOKClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_3D_VIEW_OPTIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("3D Display Options"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_3D_VIEW_OPTIONS_BASE();
	
};

#endif //__DIALOG_3D_VIEW_OPTION_BASE_H__
