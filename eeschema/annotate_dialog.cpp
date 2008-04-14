/////////////////////////////////////////////////////////////////////////////

// Name:        annotate_dialog.cpp
// Purpose:
// Author:      jean-pierre Charras
// Modified by: Wayne Stambaugh
//
// Created:     05/02/2006 12:31:28
// Modified     02/21/2008 13:47:10
// RCS-ID:
// Copyright:   License GNU
// Licence:
/////////////////////////////////////////////////////////////////////////////


#if defined (__GNUG__) && !defined (NO_GCC_PRAGMA)
#pragma implementation "annotate_dialog.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "fctsys.h"
#include "common.h"
#include "program.h"
#include "annotate_dialog.h"

extern void DeleteAnnotation( WinEDA_SchematicFrame* parent,
                              bool annotateSchematic );
extern void AnnotateComponents( WinEDA_SchematicFrame* parent,
                                bool annotateSchematic,
                                bool sortByPosition,
                                bool resetAnnotation );


/*!
 * WinEDA_AnnotateFrame type definition
 */

IMPLEMENT_DYNAMIC_CLASS( WinEDA_AnnotateFrame, wxDialog )

/*!
 * WinEDA_AnnotateFrame event table definition
 */

BEGIN_EVENT_TABLE( WinEDA_AnnotateFrame, wxDialog )
    EVT_BUTTON( ID_CLEAR_ANNOTATION, WinEDA_AnnotateFrame::OnClearAnnotation )
    EVT_BUTTON( wxID_APPLY, WinEDA_AnnotateFrame::OnApply )
    EVT_BUTTON( wxID_CANCEL, WinEDA_AnnotateFrame::OnCancel )
END_EVENT_TABLE()

/*!
 * WinEDA_AnnotateFrame constructors
 */

WinEDA_AnnotateFrame::WinEDA_AnnotateFrame()
{
    m_rbEntireSchematic = NULL;
    m_cbResetAnnotation = NULL;
    m_rbSortByPosition = NULL;
    m_btnClear = NULL;
}


WinEDA_AnnotateFrame::WinEDA_AnnotateFrame( WinEDA_SchematicFrame* parent,
                                            wxWindowID id,
                                            const wxString& caption,
                                            const wxPoint& pos,
                                            const wxSize& size,
                                            long style )
{
    m_Parent = parent;
    Create( parent, id, caption, pos, size, style );
}


/*!
 * WinEDA_AnnotateFrame creator
 */

bool WinEDA_AnnotateFrame::Create( wxWindow* parent,
                                   wxWindowID id,
                                   const wxString& caption,
                                   const wxPoint& pos,
                                   const wxSize& size,
                                   long style )
{
    SetExtraStyle( wxWS_EX_BLOCK_EVENTS );
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    if( GetSizer() )
    {
        GetSizer()->SetSizeHints( this );
    }
    Centre();

    return true;
}


/*!
 * Control creation for WinEDA_AnnotateFrame
 */

void WinEDA_AnnotateFrame::CreateControls()
{
    wxFont fontBold = this->GetFont();
    fontBold.SetWeight(wxFONTWEIGHT_BOLD);

    wxBoxSizer* sizerTop = new wxBoxSizer( wxVERTICAL );

    /* Sizer flags for setting up the spacing of the controls in the dialog
     * box.  These eventually should be moved to a file with a header in
     * the common directory so all of the dialogs share the same layout
     * spacing */

    /* Spacing for grouping labels in a dialog box. */
    wxSizerFlags flagsLabelSpacing( 0 );
    flagsLabelSpacing.Align( wxALIGN_TOP | wxALIGN_LEFT );
    flagsLabelSpacing.Border( wxLEFT | wxTOP, 6 );

    /* Spacing for grouping radio buttons inside the grouping sizer. */
    wxSizerFlags flagsRadioButtonSpacing( 0 );
    flagsRadioButtonSpacing.Align( wxALIGN_LEFT );
    flagsRadioButtonSpacing.Border( wxTOP | wxLEFT | wxRIGHT, 6 );

    /* Spacing for the radio button sizer inside the group sizer. */
    wxSizerFlags flagsRadioButtonSizerSpacing( 0 );
    flagsRadioButtonSizerSpacing.Align( wxALIGN_TOP | wxALIGN_LEFT );
    flagsRadioButtonSizerSpacing.Border( wxLEFT, 20 );

    /* Spacing for the vertical group sizers. */
    wxSizerFlags flagsGroupSizerSpacing( 1 );
    flagsGroupSizerSpacing.Align( wxALIGN_TOP | wxALIGN_LEFT );
    flagsGroupSizerSpacing.Border( wxTOP | wxLEFT | wxRIGHT, 12 );

    /* Spacing for dialog button sizer. */
    wxSizerFlags flagsDialogButtonSizerSpacing( 0 );
    flagsDialogButtonSizerSpacing.Border( wxALL, 12 );

    /* Spacing for the dialog buttons. */
    wxSizerFlags flagsDialogButtonSpacing( 0 );
    flagsDialogButtonSpacing.Border( wxLEFT | wxRIGHT, 3 );

    /* Annotate scope sizers, label, and radio buttons. */
    wxBoxSizer* sizerAnnotate = new wxBoxSizer( wxVERTICAL );
    wxStaticText* labelAnnotate = new wxStaticText( this, -1,
                                                    _( "Scope" ) );
    labelAnnotate->SetFont( fontBold );
    sizerAnnotate->Add( labelAnnotate, flagsLabelSpacing );
    wxBoxSizer* sizerAnnotateItems = new wxBoxSizer( wxVERTICAL );
    m_rbEntireSchematic =
        new wxRadioButton( this, ID_ENTIRE_SCHEMATIC,
                           _( "Annotate the &entire schematic" ),
                           wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
    wxRadioButton* rbCurrentPage =
        new wxRadioButton( this, ID_CURRENT_PAGE,
                           _( "Annotate the current &page only" ) );
    m_rbEntireSchematic->SetValue( true );
    m_cbResetAnnotation = new wxCheckBox( this, ID_RESET_ANNOTATION,
                                          _( "&Reset existing annotation" ) );

    sizerAnnotateItems->Add( m_rbEntireSchematic, flagsRadioButtonSpacing );
    sizerAnnotateItems->Add( rbCurrentPage, flagsRadioButtonSpacing );
    sizerAnnotateItems->Add( m_cbResetAnnotation, flagsRadioButtonSpacing );
    sizerAnnotate->Add( sizerAnnotateItems, flagsRadioButtonSizerSpacing );
    sizerTop->Add( sizerAnnotate, flagsGroupSizerSpacing );
    /* This is an ugly hack to make sure the focus is set correctly so the
     * escape key closes the dialog without requiring one of the controls
     * to be activated by the user first.  This problem only occurs on the
     * GTK version of wxWidgets */
#ifdef __WXGTK__
    m_rbEntireSchematic->SetFocus( );
#endif

    /* Annotation sort order sizers, label, and radio buttons. */
    wxBoxSizer* sizerSort = new wxBoxSizer( wxVERTICAL );
    wxStaticText* labelSort = new wxStaticText( this, wxID_ANY,
                                                _( "Order" ) );
    labelSort->SetFont( fontBold );
    sizerSort->Add( labelSort, flagsLabelSpacing );
    wxBoxSizer* sizerSortItems = new wxBoxSizer( wxVERTICAL );
    m_rbSortByPosition = new wxRadioButton( this,
                                            ID_SORT_BY_POSITION,
                                            _( "Sort components by p&osition" ),
                                            wxDefaultPosition,
                                            wxDefaultSize,
                                            wxRB_GROUP );
    wxRadioButton* rbSortByValue =
        new wxRadioButton( this, ID_SORT_BY_VALUE,
                           _( "Sort components by &value" ) );
    sizerSortItems->Add( m_rbSortByPosition, flagsRadioButtonSpacing );
    sizerSortItems->Add( rbSortByValue, flagsRadioButtonSpacing );
    sizerSort->Add( sizerSortItems, flagsRadioButtonSizerSpacing );
    sizerTop->Add( sizerSort, flagsGroupSizerSpacing );

    /* Standard dialog buttons and sizer. */
    wxBoxSizer* sizerDialogButtons = new wxBoxSizer( wxHORIZONTAL );
    wxButton* btnClose = new wxButton( this, wxID_CANCEL, _("Close") );
    /* TODO: Check if there is any existing annotation and enable/disable
     *       the clear button accordingly.  Probably should also enable/
     *       disable new components radio button if all of the components
     *       are already annotated.  Some low level work on the DrawSheetPath
     *       class will need to be done to accomadate this.
     */
    m_btnClear = new wxButton( this, ID_CLEAR_ANNOTATION, _("Clear Annotation") );
    wxButton* btnApply = new wxButton( this, wxID_APPLY, _("Annotation") );
    sizerDialogButtons->Add( btnClose, flagsDialogButtonSpacing );
    sizerDialogButtons->Add( new wxBoxSizer( wxHORIZONTAL ),
                             wxSizerFlags( 1 ).Expand( ) );
    sizerDialogButtons->Add( m_btnClear, flagsDialogButtonSpacing );
    sizerDialogButtons->Add( btnApply, flagsDialogButtonSpacing );
    sizerTop->Add( sizerDialogButtons, flagsDialogButtonSizerSpacing );
    SetSizer( sizerTop );
}


/*!
 * Should we show tooltips?
 */

bool WinEDA_AnnotateFrame::ShowToolTips()
{
    return true;
}


/*!
 * Get bitmap resources
 */

wxBitmap WinEDA_AnnotateFrame::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
    wxUnusedVar( name );
    return wxNullBitmap;
}


/*!
 * Get icon resources
 */

wxIcon WinEDA_AnnotateFrame::GetIconResource( const wxString& name )
{
    // Icon retrieval
    wxUnusedVar( name );
    return wxNullIcon;
}

void WinEDA_AnnotateFrame::OnClearAnnotation( wxCommandEvent& event )
{
    int response;

    wxString message = _( "Clear the existing annotation for " );
    if( GetLevel() )
        message += _( "the entire schematic?" );
    else
        message += _( "the current sheet?" );

    message += _( "\n\nThis operation will clear the existing annotation " \
                  "and cannot be undone." );
    response = wxMessageBox( message, wxT( "" ),
                             wxICON_EXCLAMATION | wxOK | wxCANCEL );
    if (response == wxCANCEL)
        return;
    DeleteAnnotation( m_Parent, GetLevel() );
    m_btnClear->Enable(false);
}

void WinEDA_AnnotateFrame::OnApply( wxCommandEvent& event )
{
    int response;
    wxString message;

    if( GetResetItems() )
        message = _( "Clear and annotate all of the components " );
    else
        message = _( "Annotate only the unannotated components " );
    if( GetLevel() )
        message += _( "on the entire schematic?" );
    else
        message += _( "on the current sheet?" );

    message += _( "\n\nThis operation will change the current annotation and " \
                  "cannot be undone." );
    response = wxMessageBox( message, wxT( "" ),
                             wxICON_EXCLAMATION | wxOK | wxCANCEL );
    if (response == wxCANCEL)
        return;
    AnnotateComponents( m_Parent, GetLevel(), GetSortOrder(),
                        GetResetItems() );
    m_btnClear->Enable();
}

void WinEDA_AnnotateFrame::OnCancel( wxCommandEvent& event )
{
    if( IsModal() )
        EndModal( wxID_CANCEL );
    else
    {
        SetReturnCode( wxID_CANCEL );
        this->Show( false );
    }
}

bool WinEDA_AnnotateFrame::GetLevel( void )
{
    wxASSERT_MSG( ((m_rbEntireSchematic != NULL) &&
                   m_rbEntireSchematic->IsKindOf( CLASSINFO( wxRadioButton ) )),
                  wxT( "m_rbEntireSchematic pointer was NULL." ) );

    return m_rbEntireSchematic->GetValue();
}

bool WinEDA_AnnotateFrame::GetResetItems( void )
{
    wxASSERT_MSG( (m_cbResetAnnotation != NULL) &&
                  m_cbResetAnnotation->IsKindOf( CLASSINFO( wxCheckBox ) ),
                  wxT( "m_cbResetAnnotation pointer was NULL." ) );

    return m_cbResetAnnotation->IsChecked();
}

bool WinEDA_AnnotateFrame::GetSortOrder( void )
/**
 * @return true if annotation by position, false if annotation by value
 */
{
    wxASSERT_MSG( (m_rbSortByPosition != NULL) &&
                  m_rbSortByPosition->IsKindOf( CLASSINFO( wxRadioButton ) ),
                  wxT( "m_rbSortByPosition pointer was NULL." ) );

    return m_rbSortByPosition->GetValue();
}
