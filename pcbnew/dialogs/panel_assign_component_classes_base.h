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

#include <wx/string.h>
#include <wx/checkbox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/sizer.h>
#include <wx/scrolwin.h>
#include <wx/panel.h>
#include <wx/textctrl.h>
#include <wx/bmpbuttn.h>
#include <wx/radiobut.h>
#include <wx/statbox.h>
#include <wx/combobox.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_ASSIGN_COMPONENT_CLASSES_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_ASSIGN_COMPONENT_CLASSES_BASE : public wxPanel
{
	private:

	protected:
		wxCheckBox* m_assignSheetClasses;
		wxStaticLine* m_staticline1;
		wxStaticText* m_staticText3;
		wxButton* m_btnAddAssignment;
		wxScrolledWindow* m_assignmentsScrollWindow;

		// Virtual event handlers, override them in your derived class
		virtual void OnAddAssignmentClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_ASSIGN_COMPONENT_CLASSES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_ASSIGN_COMPONENT_CLASSES_BASE();

};

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_COMPONENT_CLASS_ASSIGNMENT_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_COMPONENT_CLASS_ASSIGNMENT_BASE : public wxPanel
{
	private:

	protected:
		wxStaticText* m_staticText101;
		wxTextCtrl* m_componentClass;
		STD_BITMAP_BUTTON* m_buttonHighlightItems;
		STD_BITMAP_BUTTON* m_buttonDeleteAssignment;
		STD_BITMAP_BUTTON* m_buttonAddCondition;
		wxRadioButton* m_radioAll;
		wxRadioButton* m_radioAny;

		// Virtual event handlers, override them in your derived class
		virtual void OnHighlightItemsClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDeleteAssignmentClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAddConditionClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_COMPONENT_CLASS_ASSIGNMENT_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxBORDER_SUNKEN|wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_COMPONENT_CLASS_ASSIGNMENT_BASE();

};

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_COMPONENT_CLASS_CONDITION_REFERENCE_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_COMPONENT_CLASS_CONDITION_REFERENCE_BASE : public wxPanel
{
	private:

	protected:
		wxStaticText* m_title;
		wxTextCtrl* m_refs;
		STD_BITMAP_BUTTON* m_buttonImportRefs;
		STD_BITMAP_BUTTON* m_buttonDeleteMatch;

		// Virtual event handlers, override them in your derived class
		virtual void OnReferenceRightDown( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnImportRefsClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDeleteConditionClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_COMPONENT_CLASS_CONDITION_REFERENCE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxBORDER_SUNKEN|wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_COMPONENT_CLASS_CONDITION_REFERENCE_BASE();

};

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_COMPONENT_CLASS_CONDITION_SIDE_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_COMPONENT_CLASS_CONDITION_SIDE_BASE : public wxPanel
{
	private:

	protected:
		wxStaticText* m_title;
		wxComboBox* m_side;
		STD_BITMAP_BUTTON* m_buttonDeleteMatch;

		// Virtual event handlers, override them in your derived class
		virtual void OnDeleteConditionClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_COMPONENT_CLASS_CONDITION_SIDE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxBORDER_SUNKEN|wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_COMPONENT_CLASS_CONDITION_SIDE_BASE();

};

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_COMPONENT_CLASS_CONDITION_ROTATION_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_COMPONENT_CLASS_CONDITION_ROTATION_BASE : public wxPanel
{
	private:

	protected:
		wxStaticText* m_title;
		wxComboBox* m_rotation;
		wxStaticText* m_rotUnit;
		STD_BITMAP_BUTTON* m_buttonDeleteMatch;

		// Virtual event handlers, override them in your derived class
		virtual void OnDeleteConditionClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_COMPONENT_CLASS_CONDITION_ROTATION_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxBORDER_SUNKEN|wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_COMPONENT_CLASS_CONDITION_ROTATION_BASE();

};

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_COMPONENT_CLASS_CONDITION_FOOTPRINT_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_COMPONENT_CLASS_CONDITION_FOOTPRINT_BASE : public wxPanel
{
	private:

	protected:
		wxStaticText* m_title;
		wxTextCtrl* m_footprint;
		STD_BITMAP_BUTTON* m_buttonShowLibrary;
		STD_BITMAP_BUTTON* m_buttonDeleteMatch;

		// Virtual event handlers, override them in your derived class
		virtual void OnShowLibraryClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDeleteConditionClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_COMPONENT_CLASS_CONDITION_FOOTPRINT_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxBORDER_SUNKEN|wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_COMPONENT_CLASS_CONDITION_FOOTPRINT_BASE();

};

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_COMPONENT_CLASS_CONDITION_FIELD_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_COMPONENT_CLASS_CONDITION_FIELD_BASE : public wxPanel
{
	private:

	protected:
		wxStaticText* m_title;
		wxComboBox* m_fieldName;
		wxStaticText* m_staticText44;
		wxTextCtrl* m_fieldValue;
		STD_BITMAP_BUTTON* m_buttonDeleteMatch;

		// Virtual event handlers, override them in your derived class
		virtual void OnDeleteConditionClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_COMPONENT_CLASS_CONDITION_FIELD_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxBORDER_SUNKEN|wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_COMPONENT_CLASS_CONDITION_FIELD_BASE();

};

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_COMPONENT_CLASS_CONDITION_CUSTOM_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_COMPONENT_CLASS_CONDITION_CUSTOM_BASE : public wxPanel
{
	private:

	protected:
		wxStaticText* m_title;
		wxTextCtrl* m_customCondition;
		STD_BITMAP_BUTTON* m_buttonDeleteMatch;

		// Virtual event handlers, override them in your derived class
		virtual void OnDeleteConditionClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_COMPONENT_CLASS_CONDITION_CUSTOM_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxBORDER_SUNKEN|wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_COMPONENT_CLASS_CONDITION_CUSTOM_BASE();

};

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_COMPONENT_CLASS_CONDITION_SHEET_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_COMPONENT_CLASS_CONDITION_SHEET_BASE : public wxPanel
{
	private:

	protected:
		wxStaticText* m_title;
		wxComboBox* m_sheetName;
		STD_BITMAP_BUTTON* m_buttonDeleteMatch;

		// Virtual event handlers, override them in your derived class
		virtual void OnDeleteConditionClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_COMPONENT_CLASS_CONDITION_SHEET_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxBORDER_SUNKEN|wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_COMPONENT_CLASS_CONDITION_SHEET_BASE();

};

