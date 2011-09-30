/**
 * @file dialog_page_settings.cpp
 */

/* The "Page Settings" dialog box created by this file (and setpage.h)
 * contains seven checkboxes which *are* shown when that dialog box is
 * invoked in Eeschema, but which are *not* shown when that dialog box is
 * invoked in Pcbnew instead.
 */

#include "fctsys.h"
#include "common.h"
#include "base_struct.h"
#include "class_drawpanel.h"
#include "class_sch_screen.h"
#include "wxstruct.h"

#include "wx/valgen.h"

#ifdef EESCHEMA
#include "general.h"
#endif

#include "dialog_page_settings.h"

#define NB_ITEMS 11
Ki_PageDescr* SheetList[NB_ITEMS + 1] =
{
    &g_Sheet_A4,   &g_Sheet_A3, &g_Sheet_A2, &g_Sheet_A1, &g_Sheet_A0,
    &g_Sheet_A,    &g_Sheet_B,  &g_Sheet_C,  &g_Sheet_D,  &g_Sheet_E,
    &g_Sheet_user, NULL
};


void EDA_DRAW_FRAME::Process_PageSettings( wxCommandEvent& event )
{
    DIALOG_PAGES_SETTINGS frame( this );
    int diag = frame.ShowModal();

    if( DrawPanel && diag )
        DrawPanel->Refresh();
}


DIALOG_PAGES_SETTINGS::DIALOG_PAGES_SETTINGS( EDA_DRAW_FRAME* parent ) :
    DIALOG_PAGES_SETTINGS_BASE( parent )
{
    m_Parent   = parent;
    m_Screen   = m_Parent->GetScreen();
    m_Modified = 0;
    m_SelectedSheet    = NULL;
    m_CurrentSelection = 0;

    initDialog();

    GetSizer()->SetSizeHints( this );
    Centre();
}


DIALOG_PAGES_SETTINGS::~DIALOG_PAGES_SETTINGS()
{
}


void DIALOG_PAGES_SETTINGS::initDialog()
{
    wxString msg;

    SetFocus();
    SearchPageSizeSelection();

    // Init display value for sheet User size
    wxString format = m_TextSheetCount->GetLabel();
    msg.Printf( format, m_Screen->m_NumberOfScreen );
    m_TextSheetCount->SetLabel( msg );
    format = m_TextSheetNumber->GetLabel();
    msg.Printf( format, m_Screen->m_ScreenNumber );
    m_TextSheetNumber->SetLabel( msg );

    switch( g_UserUnit )
    {
    case MILLIMETRES:
        UserSizeX  = (double) g_Sheet_user.m_Size.x * 25.4 / 1000;
        UserSizeY  = (double) g_Sheet_user.m_Size.y * 25.4 / 1000;
        msg.Printf( wxT( "%.2f" ), UserSizeX );
        m_TextUserSizeX->SetValue( msg );
        msg.Printf( wxT( "%.2f" ), UserSizeY );
        m_TextUserSizeY->SetValue( msg );
        break;

    case INCHES:
        UserSizeX  = (double) g_Sheet_user.m_Size.x / 1000;
        UserSizeY  = (double) g_Sheet_user.m_Size.y / 1000;
        msg.Printf( wxT( "%.3f" ), UserSizeX );
        m_TextUserSizeX->SetValue( msg );
        msg.Printf( wxT( "%.3f" ), UserSizeY );
        m_TextUserSizeY->SetValue( msg );
        break;

    case UNSCALED_UNITS:
        UserSizeX  = g_Sheet_user.m_Size.x;
        UserSizeY  = g_Sheet_user.m_Size.y;
        msg.Printf( wxT( "%f" ), UserSizeX );
        m_TextUserSizeX->SetValue( msg );
        msg.Printf( wxT( "%f" ), UserSizeY );
        m_TextUserSizeY->SetValue( msg );
        break;
    }

    // Set validators
    m_PageSizeBox->SetValidator( wxGenericValidator( &m_CurrentSelection ) );
    m_TextRevision->SetValidator( wxTextValidator( wxFILTER_NONE, &m_Screen->m_Revision ) );
    m_TextTitle->SetValidator( wxTextValidator( wxFILTER_NONE, &m_Screen->m_Title ) );
    m_TextCompany->SetValidator( wxTextValidator( wxFILTER_NONE, &m_Screen->m_Company ) );
    m_TextComment1->SetValidator( wxTextValidator( wxFILTER_NONE, &m_Screen->m_Commentaire1 ) );
    m_TextComment2->SetValidator( wxTextValidator( wxFILTER_NONE, &m_Screen->m_Commentaire2 ) );
    m_TextComment3->SetValidator( wxTextValidator( wxFILTER_NONE, &m_Screen->m_Commentaire3 ) );
    m_TextComment4->SetValidator( wxTextValidator( wxFILTER_NONE, &m_Screen->m_Commentaire4 ) );

#ifndef EESCHEMA
    m_RevisionExport->Show( false );
    m_TitleExport->Show( false );
    m_CompanyExport->Show( false );
    m_Comment1Export->Show( false );
    m_Comment2Export->Show( false );
    m_Comment3Export->Show( false );
    m_Comment4Export->Show( false );
#endif

    // Make the OK button the default.
    m_sdbSizer1OK->SetDefault();
}


/*!
 * wxEVT_CLOSE_WINDOW event handler for ID_DIALOG
 */

void DIALOG_PAGES_SETTINGS::OnCloseWindow( wxCloseEvent& event )
{
    EndModal( m_Modified );
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void DIALOG_PAGES_SETTINGS::OnOkClick( wxCommandEvent& event )
{
    SavePageSettings( event );
    m_Modified = 1;
    Close( true );
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void DIALOG_PAGES_SETTINGS::OnCancelClick( wxCommandEvent& event )
{
    Close( true );
}


void DIALOG_PAGES_SETTINGS::SavePageSettings( wxCommandEvent& event )
{
    double      dtmp;
    wxString    msg;

    m_Screen->m_Revision       = m_TextRevision->GetValue();
    m_Screen->m_Company        = m_TextCompany->GetValue();
    m_Screen->m_Title          = m_TextTitle->GetValue();
    m_Screen->m_Commentaire1   = m_TextComment1->GetValue();
    m_Screen->m_Commentaire2   = m_TextComment2->GetValue();
    m_Screen->m_Commentaire3   = m_TextComment3->GetValue();
    m_Screen->m_Commentaire4   = m_TextComment4->GetValue();

    msg = m_TextUserSizeX->GetValue();
    msg.ToDouble( &dtmp );
    UserSizeX = dtmp;
    msg = m_TextUserSizeY->GetValue();
    msg.ToDouble( &dtmp );
    UserSizeY = dtmp;

    int ii = m_PageSizeBox->GetSelection();

    if( ii < 0 )
        ii = 0;

    m_SelectedSheet = SheetList[ii];
    m_Screen->m_CurrentSheetDesc = m_SelectedSheet;

    switch( g_UserUnit )
    {
    case MILLIMETRES:
        g_Sheet_user.m_Size.x  = (int) ( UserSizeX * 1000 / 25.4 );
        g_Sheet_user.m_Size.y  = (int) ( UserSizeY * 1000 / 25.4 );
        break;

    case INCHES:
        g_Sheet_user.m_Size.x  = (int) ( UserSizeX * 1000 );
        g_Sheet_user.m_Size.y  = (int) ( UserSizeY * 1000 );
        break;

    case UNSCALED_UNITS:
        g_Sheet_user.m_Size.x  = (int) ( UserSizeX );
        g_Sheet_user.m_Size.y  = (int) ( UserSizeY );
        break;
    }

    if( g_Sheet_user.m_Size.x < 6000 )
        g_Sheet_user.m_Size.x = 6000;

    if( g_Sheet_user.m_Size.x > 44000 )
        g_Sheet_user.m_Size.x = 44000;

    if( g_Sheet_user.m_Size.y < 4000 )
        g_Sheet_user.m_Size.y = 4000;

    if( g_Sheet_user.m_Size.y > 44000 )
        g_Sheet_user.m_Size.y = 44000;

#ifdef EESCHEMA
    /* Exports settings to other sheets if requested: */
    SCH_SCREEN* screen;

    /* Build the screen list */
    SCH_SCREENS ScreenList;

    /* Update the datas */
    for( screen = ScreenList.GetFirst(); screen != NULL; screen = ScreenList.GetNext() )
    {
        if( screen == m_Screen )
            continue;

        if( m_RevisionExport->IsChecked() )
            screen->m_Revision = m_Screen->m_Revision;

        if( m_TitleExport->IsChecked() )
            screen->m_Title = m_Screen->m_Title;

        if( m_CompanyExport->IsChecked() )
            screen->m_Company = m_Screen->m_Company;

        if( m_Comment1Export->IsChecked() )
            screen->m_Commentaire1 = m_Screen->m_Commentaire1;

        if( m_Comment2Export->IsChecked() )
            screen->m_Commentaire2 = m_Screen->m_Commentaire2;

        if( m_Comment3Export->IsChecked() )
            screen->m_Commentaire3 = m_Screen->m_Commentaire3;

        if( m_Comment4Export->IsChecked() )
            screen->m_Commentaire4 = m_Screen->m_Commentaire4;
    }

#endif

    m_Screen->SetModify();
    m_Parent->DrawPanel->Refresh();
}


/* Search the correct index to activate the radiobox list size selection
 * according to the current page size
 */
void DIALOG_PAGES_SETTINGS::SearchPageSizeSelection()
{
    Ki_PageDescr*   sheet;
    int             ii;

    m_CurrentSelection = NB_ITEMS - 1;

    for( ii = 0; ii < NB_ITEMS; ii++ )
    {
        sheet = SheetList[ii];

        if( m_Parent->GetScreen()->m_CurrentSheetDesc == sheet )
            m_CurrentSelection = ii;
    }
}
