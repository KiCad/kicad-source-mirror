///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_EDIT_COMPONENT_IN_LIB_BASE_H__
#define __DIALOG_EDIT_COMPONENT_IN_LIB_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class DIALOG_SHIM;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/checkbox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/spinctrl.h>
#include <wx/panel.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/listbox.h>
#include <wx/notebook.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

#define ID_LIBEDIT_NOTEBOOK 1000
#define ID_COPY_DOC_TO_ALIAS 1001
#define ID_BROWSE_DOC_FILES 1002
#define ID_ADD_ALIAS 1003
#define ID_DELETE_ONE_ALIAS 1004
#define ID_DELETE_ALL_ALIAS 1005
#define ID_ADD_FOOTPRINT_FILTER 1006
#define ID_DELETE_ONE_FOOTPRINT_FILTER 1007
#define ID_DELETE_ALL_FOOTPRINT_FILTER 1008

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_EDIT_COMPONENT_IN_LIBRARY_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_EDIT_COMPONENT_IN_LIBRARY_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxNotebook* m_NoteBook;
		wxPanel* m_PanelBasic;
		wxCheckBox* m_AsConvertButt;
		wxCheckBox* m_ShowPinNumButt;
		wxCheckBox* m_ShowPinNameButt;
		wxCheckBox* m_PinsNameInsideButt;
		wxStaticLine* m_staticline3;
		wxStaticText* m_staticTextNbUnits;
		wxSpinCtrl* m_SelNumberOfUnits;
		wxStaticText* m_staticTextskew;
		wxSpinCtrl* m_SetSkew;
		wxStaticLine* m_staticline1;
		wxCheckBox* m_OptionPower;
		wxCheckBox* m_OptionPartsLocked;
		wxPanel* m_PanelDoc;
		wxStaticText* m_staticTextDescription;
		wxTextCtrl* m_DocCtrl;
		wxStaticText* m_staticTextKeywords;
		wxTextCtrl* m_KeywordsCtrl;
		wxStaticText* m_staticTextDocFileName;
		wxTextCtrl* m_DocfileCtrl;
		wxButton* m_ButtonCopyDoc;
		wxButton* m_buttonBrowseDocFiles;
		wxPanel* m_PanelAlias;
		wxStaticText* m_staticTextAlias;
		wxListBox* m_PartAliasListCtrl;
		wxButton* m_ButtonAddeAlias;
		wxButton* m_ButtonDeleteOneAlias;
		wxButton* m_ButtonDeleteAllAlias;
		wxPanel* m_PanelFootprintFilter;
		wxStaticText* m_staticTextFootprints;
		wxListBox* m_FootprintFilterListBox;
		wxButton* m_buttonAddFpF;
		wxButton* m_buttonEditOneFootprintFilter;
		wxButton* m_ButtonDeleteOneFootprintFilter;
		wxButton* m_ButtonDeleteAllFootprintFilter;
		wxStdDialogButtonSizer* m_stdSizerButton;
		wxButton* m_stdSizerButtonOK;
		wxButton* m_stdSizerButtonCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void CopyDocFromRootToAlias( wxCommandEvent& event ) { event.Skip(); }
		virtual void BrowseAndSelectDocFile( wxCommandEvent& event ) { event.Skip(); }
		virtual void AddAliasOfPart( wxCommandEvent& event ) { event.Skip(); }
		virtual void DeleteAliasOfPart( wxCommandEvent& event ) { event.Skip(); }
		virtual void DeleteAllAliasOfPart( wxCommandEvent& event ) { event.Skip(); }
		virtual void AddFootprintFilter( wxCommandEvent& event ) { event.Skip(); }
		virtual void EditOneFootprintFilter( wxCommandEvent& event ) { event.Skip(); }
		virtual void DeleteOneFootprintFilter( wxCommandEvent& event ) { event.Skip(); }
		virtual void DeleteAllFootprintFilter( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_EDIT_COMPONENT_IN_LIBRARY_BASE( wxWindow* parent, wxWindowID id = ID_LIBEDIT_NOTEBOOK, const wxString& title = _("Library Component Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_EDIT_COMPONENT_IN_LIBRARY_BASE();
	
};

#endif //__DIALOG_EDIT_COMPONENT_IN_LIB_BASE_H__
