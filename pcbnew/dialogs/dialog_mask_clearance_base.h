///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar  9 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_MASK_CLEARANCE_BASE_H__
#define __DIALOG_MASK_CLEARANCE_BASE_H__

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
#include <wx/statline.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_PADS_MASK_CLEARANCE_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_PADS_MASK_CLEARANCE_BASE : public DIALOG_SHIM
{
	DECLARE_EVENT_TABLE()
	private:
		
		// Private event handlers
		void _wxFB_OnButtonCancelClick( wxCommandEvent& event ){ OnButtonCancelClick( event ); }
		void _wxFB_OnButtonOkClick( wxCommandEvent& event ){ OnButtonOkClick( event ); }
		
	
	protected:
		wxStaticText* m_staticTextInfo;
		wxStaticLine* m_staticline1;
		wxStaticText* m_MaskClearanceTitle;
		wxTextCtrl* m_SolderMaskMarginCtrl;
		wxStaticText* m_SolderMaskMarginUnits;
		wxStaticText* m_staticTextMinWidth;
		wxTextCtrl* m_SolderMaskMinWidthCtrl;
		wxStaticText* m_solderMaskMinWidthUnit;
		wxStaticLine* m_staticline3;
		wxStaticLine* m_staticline4;
		wxStaticLine* m_staticline5;
		wxStaticText* m_staticTextSolderPaste;
		wxTextCtrl* m_SolderPasteMarginCtrl;
		wxStaticText* m_SolderPasteMarginUnits;
		wxStaticText* m_staticTextRatio;
		wxTextCtrl* m_SolderPasteMarginRatioCtrl;
		wxStaticText* m_SolderPasteRatioMarginUnits;
		wxStaticLine* m_staticline11;
		wxStdDialogButtonSizer* m_sdbButtonsSizer;
		wxButton* m_sdbButtonsSizerOK;
		wxButton* m_sdbButtonsSizerCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnButtonCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonOkClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_PADS_MASK_CLEARANCE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Pads Mask Clearance"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 361,304 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_PADS_MASK_CLEARANCE_BASE();
	
};

#endif //__DIALOG_MASK_CLEARANCE_BASE_H__
