///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf)
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

#include "dialogs/panel_notebook_base.h"
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
#include <wx/textctrl.h>
#include <wx/choice.h>
#include <wx/checklst.h>
#include <wx/dialog.h>
#include <wx/checkbox.h>
#include <wx/listctrl.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_JOBS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_JOBS_BASE : public PANEL_NOTEBOOK_BASE
{
	private:

	protected:
		WX_GRID* m_jobsGrid;
		STD_BITMAP_BUTTON* m_buttonAddJob;
		STD_BITMAP_BUTTON* m_buttonUp;
		STD_BITMAP_BUTTON* m_buttonDown;
		STD_BITMAP_BUTTON* m_buttonDelete;
		wxScrolledWindow* m_outputList;
		wxBoxSizer* m_outputListSizer;
		STD_BITMAP_BUTTON* m_buttonOutputAdd;
		wxButton* m_buttonSave;
		wxButton* m_buttonRunAllOutputs;

		// Virtual event handlers, override them in your derived class
		virtual void OnSizeGrid( wxSizeEvent& event ) { event.Skip(); }
		virtual void OnAddJobClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnJobButtonUp( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnJobButtonDown( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnJobButtonDelete( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAddOutputClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSaveButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRunAllJobsClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_JOBS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 600,400 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_JOBS_BASE();

};

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_JOB_OUTPUT_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_JOB_OUTPUT_BASE : public wxPanel
{
	private:

	protected:
		wxStaticBitmap* m_bitmapOutputType;
		wxStaticText* m_textOutputType;
		wxStaticBitmap* m_statusBitmap;
		wxBitmapButton* m_buttonOutputRun;
		wxBitmapButton* m_buttonOutputOptions;

		// Virtual event handlers, override them in your derived class
		virtual void OnLastStatusClick( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnOutputRunClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOutputOptionsClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_JOB_OUTPUT_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxBORDER_SIMPLE|wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_JOB_OUTPUT_BASE();

};

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_JOB_OUTPUT_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_JOB_OUTPUT_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_textArchiveDesc;
		wxTextCtrl* m_textCtrlDescription;
		wxStaticText* m_textArchiveFormat;
		wxChoice* m_choiceArchiveformat;
		wxStaticText* m_textOutputPath;
		wxTextCtrl* m_textCtrlOutputPath;
		STD_BITMAP_BUTTON* m_buttonOutputPath;
		wxStaticText* m_staticText10;
		wxCheckListBox* m_includeJobs;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1Save;
		wxButton* m_sdbSizer1Cancel;

		// Virtual event handlers, override them in your derived class
		virtual void onOutputPathBrowseClicked( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_JOB_OUTPUT_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE );

		~DIALOG_JOB_OUTPUT_BASE();

};

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_SPECIAL_EXECUTE_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_SPECIAL_EXECUTE_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_staticText9;
		wxPanel* m_panel9;
		wxStaticText* m_textCommand;
		wxTextCtrl* m_textCtrlCommand;
		wxCheckBox* m_cbRecordOutput;
		wxStaticText* m_textOutputPath;
		wxTextCtrl* m_textCtrlOutputPath;
		wxCheckBox* m_cbIgnoreExitCode;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1Save;
		wxButton* m_sdbSizer1Cancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnRecordOutputClicked( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_SPECIAL_EXECUTE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE );

		~DIALOG_SPECIAL_EXECUTE_BASE();

};

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_OUTPUT_RUN_RESULTS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_OUTPUT_RUN_RESULTS_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_staticTextOutputName;
		wxListCtrl* m_jobList;
		wxTextCtrl* m_textCtrlOutput;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;

		// Virtual event handlers, override them in your derived class
		virtual void OnJobListItemSelected( wxListEvent& event ) { event.Skip(); }
		virtual void OnButtonOk( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_OUTPUT_RUN_RESULTS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Job Output Run Log"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE );

		~DIALOG_OUTPUT_RUN_RESULTS_BASE();

};

