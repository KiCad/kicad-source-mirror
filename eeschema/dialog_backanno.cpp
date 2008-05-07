/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_Backanno.cpp
// Purpose:
// Author:      Frank Bennett
// Modified by:
// Created:     Thu May  1 11:23:06 MDT 2008 
// RCS-ID:
// Copyright:   License GNU
// Licence:
/////////////////////////////////////////////////////////////////////////////


////@begin includes
////@end includes

#include "dialog_backanno.h"

////@begin XPM images
////@end XPM images

/*!
 * WinEDA_BackannoFrame type definition
 */

IMPLEMENT_DYNAMIC_CLASS( WinEDA_BackannoFrame, wxDialog )

/*!
 * WinEDA_BackannoFrame event table definition
 */

BEGIN_EVENT_TABLE( WinEDA_BackannoFrame, wxDialog )

////@begin WinEDA_BackannoFrame event table entries

////@end WinEDA_BackannoFrame event table entries

END_EVENT_TABLE()

/*!
 * WinEDA_BackannoFrame constructors
 */

WinEDA_BackannoFrame::WinEDA_BackannoFrame( )
{
}

WinEDA_BackannoFrame::WinEDA_BackannoFrame( WinEDA_SchematicFrame* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    m_Parent = parent;
    Create(parent, id, caption, pos, size, style);
}

/*!
 * WinEDA_BackannoFrame creator
 */

bool WinEDA_BackannoFrame::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin WinEDA_BackannoFrame member initialisation
    m_NewTextCtrl = NULL;
////@end WinEDA_BackannoFrame member initialisation

////@begin WinEDA_BackannoFrame creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end WinEDA_BackannoFrame creation

    m_NewTextCtrl->SetFocus();

    /*  does not work here, might work if moved elsewhere,
    //    see void DrcDialog::OnInitDialog( wxInitDialogEvent& event )
    // deselect the existing text, seems SetFocus() wants to emulate
    // Microsoft and select all text, which is not desireable here.
    m_NewTextCtrl->SetSelection(0,0);
    */

    return true;
}

/*!
 * Should we show tooltips?
 */

bool WinEDA_BackannoFrame::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap WinEDA_BackannoFrame::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin WinEDA_BackannoFrame bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end WinEDA_BackannoFrame bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon WinEDA_BackannoFrame::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin WinEDA_BackannoFrame icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end WinEDA_BackannoFrame icon retrieval
}
