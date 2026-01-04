///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class STD_BITMAP_BUTTON;
class WX_GRID;

#include "widgets/panel_notebook_base.h"
#include "dialog_shim.h"
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/font.h>
#include <wx/grid.h>
#include <wx/gdicmn.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/scrolwin.h>
#include <wx/panel.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>
#include <wx/listctrl.h>
#include <wx/textctrl.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_JOBSET_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_JOBSET_BASE : public PANEL_NOTEBOOK_BASE
{
	private:

	protected:
		WX_GRID* m_jobsGrid;
		STD_BITMAP_BUTTON* m_buttonAddJob;
		STD_BITMAP_BUTTON* m_buttonUp;
		STD_BITMAP_BUTTON* m_buttonDown;
		STD_BITMAP_BUTTON* m_buttonDelete;
		wxScrolledWindow* m_destinationList;
		wxBoxSizer* m_destinationListSizer;
		STD_BITMAP_BUTTON* m_buttonAddDestination;
		wxButton* m_buttonSave;
		wxButton* m_buttonGenerateAllDestinations;

		// Virtual event handlers, override them in your derived class
		virtual void OnGridCellChange( wxGridEvent& event ) { event.Skip(); }
		virtual void OnSizeGrid( wxSizeEvent& event ) { event.Skip(); }
		virtual void OnAddJobClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnJobButtonUp( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnJobButtonDown( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnJobButtonDelete( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAddDestinationClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSaveButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnGenerateAllDestinationsClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_JOBSET_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_JOBSET_BASE();

};

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_DESTINATION_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_DESTINATION_BASE : public wxPanel
{
	private:

	protected:
		wxStaticBitmap* m_bitmapOutputType;
		wxStaticText* m_textOutputType;
		wxStaticBitmap* m_statusBitmap;
		wxStaticText* m_pathInfo;
		STD_BITMAP_BUTTON* m_buttonProperties;
		STD_BITMAP_BUTTON* m_buttonDelete;
		wxButton* m_buttonGenerate;
		STD_BITMAP_BUTTON* m_buttonOpenOutput;

		// Virtual event handlers, override them in your derived class
		virtual void OnRightDown( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnLastStatusClick( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnProperties( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDelete( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnGenerate( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOpenOutput( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_DESTINATION_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxBORDER_SUNKEN|wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_DESTINATION_BASE();

};

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_JOBSET_RUN_LOG_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_JOBSET_RUN_LOG_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_staticTextOutputName;
		wxListCtrl* m_jobList;
		wxTextCtrl* m_textCtrlOutput;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;

		// Virtual event handlers, override them in your derived class
		virtual void OnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnJobListItemSelected( wxListEvent& event ) { event.Skip(); }
		virtual void OnButtonOk( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_JOBSET_RUN_LOG_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Jobset Run Log"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_JOBSET_RUN_LOG_BASE();

};

