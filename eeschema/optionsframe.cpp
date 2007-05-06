/////////////////////////////////////////////////////////////////////////////
// Name:        optionsframe.cpp
// Purpose:     
// Author:      jean-pierre charras
// Modified by: 
// Created:     01/27/04 14:48:57
// RCS-ID:      
// Copyright:   suite kicad
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
#pragma implementation "optionsframe.cpp"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
#include "wx/wx.h"
#include "wx/valgen.h"
////@end includes

#include "optionsframe.h"

////@begin XPM images
////@end XPM images

/*!
 * DisplayOptionFrame type definition
 */

IMPLEMENT_CLASS( DisplayOptionFrame, wxDialog )

/*!
 * DisplayOptionFrame event table definition
 */

BEGIN_EVENT_TABLE( DisplayOptionFrame, wxDialog )

////@begin DisplayOptionFrame event table entries
////@end DisplayOptionFrame event table entries

END_EVENT_TABLE()

/*!
 * DisplayOptionFrame constructors
 */

DisplayOptionFrame::DisplayOptionFrame( )
{
}

DisplayOptionFrame::DisplayOptionFrame( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * optionsframe creator
 */

bool DisplayOptionFrame::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin DisplayOptionFrame member initialisation
////@end DisplayOptionFrame member initialisation

////@begin DisplayOptionFrame creation
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end DisplayOptionFrame creation
    return TRUE;
}

/*!
 * Control creation for optionsframe
 */

void DisplayOptionFrame::CreateControls()
{    
////@begin DisplayOptionFrame content construction

    DisplayOptionFrame* item1 = this;

    wxBoxSizer* item2 = new wxBoxSizer(wxVERTICAL);
    item1->SetSizer(item2);
    item1->SetAutoLayout(TRUE);

    wxBoxSizer* item3 = new wxBoxSizer(wxHORIZONTAL);
    item2->Add(item3, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxBoxSizer* item4 = new wxBoxSizer(wxHORIZONTAL);
    item3->Add(item4, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* item5 = new wxBoxSizer(wxVERTICAL);
    item4->Add(item5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxCheckBox* item6 = new wxCheckBox( item1, ID_CHECKBOX, _("Show grid"), wxDefaultPosition, wxDefaultSize, 0 );
    m_ShowGridButt = item6;
    item6->SetValue(FALSE);
    item5->Add(item6, 0, wxALIGN_LEFT|wxALL, 5);

    wxString item7Strings[] = {
        _("&Normal (50 mils)"),
        _("&Small (20 mils)"),
        _("&Very small (10 mils)")
    };
    wxRadioBox* item7 = new wxRadioBox( item1, ID_RADIOBOX, _("Grid Size"), wxDefaultPosition, wxDefaultSize, 3, item7Strings, 1, 0 );
    m_SelGridSize = item7;
    item5->Add(item7, 0, wxALIGN_LEFT|wxALL, 5);

    wxString item8Strings[] = {
        _("&Normal"),
        _("&inches")
    };
    wxRadioBox* item8 = new wxRadioBox( item1, ID_RADIOBOX1, _("Show pins"), wxDefaultPosition, wxDefaultSize, 2, item8Strings, 1, 0 );
    m_SelShowPins = item8;
    item5->Add(item8, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* item9 = new wxBoxSizer(wxVERTICAL);
    item4->Add(item9, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxString item10Strings[] = {
        _("&millimeters"),
        _("&inches")
    };
    wxRadioBox* item10 = new wxRadioBox( item1, ID_RADIOBOX3, _("Units"), wxDefaultPosition, wxDefaultSize, 2, item10Strings, 1, wxRA_SPECIFY_COLS );
    m_Selunits = item10;
    item9->Add(item10, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    item9->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxString item12Strings[] = {
        _("&Horiz/Vertical"),
        _("&Any")
    };
    wxRadioBox* item12 = new wxRadioBox( item1, ID_RADIOBOX2, _("Wire - Bus orient"), wxDefaultPosition, wxDefaultSize, 2, item12Strings, 1, wxRA_SPECIFY_COLS );
    item9->Add(item12, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxBoxSizer* item13 = new wxBoxSizer(wxVERTICAL);
    item4->Add(item13, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* item14 = new wxButton( item1, ID_BUTTON, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    item14->SetForegroundColour(wxColour(255, 0, 0));
    item13->Add(item14, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxButton* item15 = new wxButton( item1, ID_BUTTON1, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    item15->SetForegroundColour(wxColour(0, 0, 255));
    item13->Add(item15, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);


    // Set validators
    item6->SetValidator( wxGenericValidator(& ShowGrid ) );
////@end DisplayOptionFrame content construction
}

/*!
 * Should we show tooltips?
 */

bool DisplayOptionFrame::ShowToolTips()
{
    return TRUE;
}
