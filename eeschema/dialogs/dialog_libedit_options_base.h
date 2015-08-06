///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar 13 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_LIBEDIT_OPTIONS_BASE_H__
#define __DIALOG_LIBEDIT_OPTIONS_BASE_H__

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
#include <wx/choice.h>
#include <wx/spinctrl.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_LIBEDIT_OPTIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_LIBEDIT_OPTIONS_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxStaticText* m_staticText3;
		wxChoice* m_choiceGridSize;
		wxStaticText* m_staticGridUnits;
		wxStaticText* m_staticText5;
		wxSpinCtrl* m_spinLineWidth;
		wxStaticText* m_staticLineWidthUnits;
		wxStaticText* m_staticText52;
		wxSpinCtrl* m_spinPinLength;
		wxStaticText* m_staticPinLengthUnits;
		wxStaticText* m_staticText7;
		wxSpinCtrl* m_spinPinNumSize;
		wxStaticText* m_staticTextSizeUnits;
		wxStaticText* m_staticText9;
		wxSpinCtrl* m_spinPinNameSize;
		wxStaticText* m_staticRepeatXUnits;
		wxStaticText* m_staticText11;
		wxSpinCtrl* m_spinRepeatHorizontal;
		wxStaticText* m_staticText12;
		wxStaticText* m_staticText13;
		wxSpinCtrl* m_spinRepeatVertical;
		wxStaticText* m_staticText14;
		wxStaticText* m_staticText15;
		wxChoice* m_choicePinDisplacement;
		wxStaticText* m_staticText16;
		wxStaticText* m_staticText17;
		wxSpinCtrl* m_spinRepeatLabel;
		wxStaticText* m_stMaxUndoItems;
		wxSpinCtrl* m_spinMaxUndoItems;
		wxStaticText* m_stMaxUndoItemsUnit;
		wxStaticLine* m_staticline3;
		wxCheckBox* m_checkShowGrid;
		wxStaticLine* m_staticline2;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;
	
	public:
		
		DIALOG_LIBEDIT_OPTIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Library Editor Options"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 487,433 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_LIBEDIT_OPTIONS_BASE();
	
};

#endif //__DIALOG_LIBEDIT_OPTIONS_BASE_H__
