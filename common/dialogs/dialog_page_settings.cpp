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
#include <wx/tokenzr.h>

#ifdef EESCHEMA
#include "general.h"
#endif

#include "dialog_page_settings.h"


void EDA_DRAW_FRAME::Process_PageSettings( wxCommandEvent& event )
{
    DIALOG_PAGES_SETTINGS frame( this );
    int diag = frame.ShowModal();

    if( m_canvas && diag )
        m_canvas->Refresh();
}


DIALOG_PAGES_SETTINGS::DIALOG_PAGES_SETTINGS( EDA_DRAW_FRAME* parent ) :
    DIALOG_PAGES_SETTINGS_BASE( parent ),
    m_user_size( wxT( "User" ) )
{
    m_Parent   = parent;
    m_Screen   = m_Parent->GetScreen();
    m_modified = false;

    initDialog();

    GetSizer()->SetSizeHints( this );
    Centre();
}


DIALOG_PAGES_SETTINGS::~DIALOG_PAGES_SETTINGS()
{
}


void DIALOG_PAGES_SETTINGS::initDialog()
{
    wxString    msg;
    double      userSizeX;
    double      userSizeY;

    SetFocus();

    // Init display value for sheet User size
    wxString format = m_TextSheetCount->GetLabel();
    msg.Printf( format, m_Screen->m_NumberOfScreen );
    m_TextSheetCount->SetLabel( msg );

    format = m_TextSheetNumber->GetLabel();
    msg.Printf( format, m_Screen->m_ScreenNumber );
    m_TextSheetNumber->SetLabel( msg );

    m_page = m_Parent->GetPageSettings();

    setCurrentPageSizeSelection();

    switch( g_UserUnit )
    {
    case MILLIMETRES:
        userSizeX  = m_user_size.GetWidthMils()  * 25.4e-3;
        userSizeY  = m_user_size.GetHeightMils() * 25.4e-3;

        msg.Printf( wxT( "%.2f" ), userSizeX );
        m_TextUserSizeX->SetValue( msg );

        msg.Printf( wxT( "%.2f" ), userSizeY );
        m_TextUserSizeY->SetValue( msg );
        break;

    default:
    case INCHES:
        userSizeX  = m_user_size.GetWidthMils() / 1000.0;
        userSizeY  = m_user_size.GetHeightMils() / 1000.0;

        msg.Printf( wxT( "%.3f" ), userSizeX );
        m_TextUserSizeX->SetValue( msg );

        msg.Printf( wxT( "%.3f" ), userSizeY );
        m_TextUserSizeY->SetValue( msg );
        break;

/*  // you want it in mils, why?
    case UNSCALED_UNITS:
        userSizeX  = m_user_size.GetWidthMils();
        userSizeY  = m_user_size.GetHeightMils();
        msg.Printf( wxT( "%f" ), m_userSizeX );
        m_TextUserSizeX->SetValue( msg );
        msg.Printf( wxT( "%f" ), m_userSizeY );
        m_TextUserSizeY->SetValue( msg );
        break;
*/
    }

    // Set validators
//    m_PageSizeBox->SetValidator( wxGenericValidator( &m_CurrentSelection ) );
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
    EndModal( m_modified );
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void DIALOG_PAGES_SETTINGS::OnOkClick( wxCommandEvent& event )
{
    SavePageSettings( event );
    m_modified = true;
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
    wxString    msg;
    double      userSizeX;
    double      userSizeY;

    m_Screen->m_Revision       = m_TextRevision->GetValue();
    m_Screen->m_Company        = m_TextCompany->GetValue();
    m_Screen->m_Title          = m_TextTitle->GetValue();
    m_Screen->m_Commentaire1   = m_TextComment1->GetValue();
    m_Screen->m_Commentaire2   = m_TextComment2->GetValue();
    m_Screen->m_Commentaire3   = m_TextComment3->GetValue();
    m_Screen->m_Commentaire4   = m_TextComment4->GetValue();

    msg = m_TextUserSizeX->GetValue();
    msg.ToDouble( &userSizeX );

    msg = m_TextUserSizeY->GetValue();
    msg.ToDouble( &userSizeY );

    int radioSelection = m_PageSizeBox->GetSelection();
    if( radioSelection < 0 )
        radioSelection = 0;

    // wxFormBuilder must use "A4", "A3", etc for choices, in all languages/translations
    wxString paperType = m_PageSizeBox->GetString( radioSelection );

    m_page.SetType( paperType );

    m_Parent->SetPageSettings( m_page );

    switch( g_UserUnit )
    {
    case MILLIMETRES:
        PAGE_INFO::SetUserWidthMils( int( userSizeX * 1000.0 / 25.4 ) );
        PAGE_INFO::SetUserHeightMils( int( userSizeY * 1000.0 / 25.4 ) );
        break;

    default:
    case INCHES:
        PAGE_INFO::SetUserWidthMils( int( 1000 * userSizeX ) );
        PAGE_INFO::SetUserHeightMils( int( 1000 * userSizeY ) );
        break;

/*      // set in 1/1000ths of an inch, but why?
    case UNSCALED_UNITS:
        PAGE_INFO::SetUserWidthMils( (int) userSizeX );
        PAGE_INFO::SetUserHeightMils( (int) userSizeY );
        break;
*/
    }

#ifdef EESCHEMA
    // Exports settings to other sheets if requested:
    SCH_SCREEN* screen;

    // Build the screen list
    SCH_SCREENS ScreenList;

    // Update the datas
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
    m_Parent->GetCanvas()->Refresh();
}


void DIALOG_PAGES_SETTINGS::setCurrentPageSizeSelection()
{
    wxString    curPaperType = m_page.GetType();

    // use wxFormBuilder to store the sheet type in the wxRadioButton's label
    // i.e. "A4", "A3", etc, anywhere within the text of the label.

    D(printf("m_PageSizeBox->GetCount() = %d\n", (int) m_PageSizeBox->GetCount() );)

    // search all the child wxRadioButtons for a label containing our paper type
    for( unsigned i = 0;  i < m_PageSizeBox->GetCount();  ++i )
    {
        // parse each label looking for curPaperType within it
        wxStringTokenizer st( m_PageSizeBox->GetString( i ) );

        while( st.HasMoreTokens() )
        {
            if( st.GetNextToken() == curPaperType )
            {
                m_PageSizeBox->SetSelection( i );
                return;
            }
        }
    }

    // m_PageSizeBox->SetSelection( 1 );        // wxFormBuilder does this, control there
}

