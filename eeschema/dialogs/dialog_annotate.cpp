/**
 * @file dialog_annotate.cpp
 * @brief Annotation dialog functions.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2017 jean-pierre Charras jp.charras at wanadoo.fr
 * Copyright (C) 1992-2017 Kicad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


#include <fctsys.h>
#include <sch_edit_frame.h>
#include <class_drawpanel.h>
#include <bitmaps.h>

#include <invoke_sch_dialog.h>
#include <dialog_annotate_base.h>
#include <kiface_i.h>
#include <wx_html_report_panel.h>

#define KEY_ANNOTATE_SORT_OPTION          wxT( "AnnotateSortOption" )
#define KEY_ANNOTATE_ALGO_OPTION          wxT( "AnnotateAlgoOption" )
#define KEY_ANNOTATE_KEEP_OPEN_OPTION     wxT( "AnnotateKeepOpenOption" )
#define KEY_ANNOTATE_SKIP_CONFIRMATION    wxT( "AnnotateSkipConfirmation" )
#define KEY_ANNOTATE_MESSAGES_FILTER      wxT( "AnnotateFilterMsg" )


class wxConfigBase;

/**
 * Class DIALOG_ANNOTATE: a dialog to set/clear reference designators,
 * of a schematic hierarchy, with different options
 */
class DIALOG_ANNOTATE: public DIALOG_ANNOTATE_BASE
{
public:
    DIALOG_ANNOTATE( SCH_EDIT_FRAME* parent, const wxString& message );
    ~DIALOG_ANNOTATE();

private:
    SCH_EDIT_FRAME* m_Parent;
    wxConfigBase*   m_Config;

    /// Initialises member variables
    void InitValues();
    void OnClearAnnotationCmpClick( wxCommandEvent& event ) override;
    void OnApplyClick( wxCommandEvent& event ) override;

    // User functions:
    bool GetLevel();
    bool GetResetItems();
    bool GetLockUnits();

    /**
     * Function GetSortOrder
     * @return 0 if annotation by X position,
     *         1 if annotation by Y position,
     */
    int GetSortOrder();

    /**
     * Function GetAnnotateAlgo
     * @return 0 if annotation using first free Id value
     *         1 for first free Id value inside sheet num * 100 to sheet num * 100 + 99
     *         2 for first free Id value inside sheet num * 1000 to sheet num * 1000 + 999
     */
    int GetAnnotateAlgo();

    int GetStartNumber();

    bool GetAnnotateKeepOpen()
    {
        return m_cbKeepDlgOpen->GetValue();
    }

    bool GetAnnotateSkipConfirmation()
    {
        return m_cbSkipConfirmation->GetValue();
    }
};


DIALOG_ANNOTATE::DIALOG_ANNOTATE( SCH_EDIT_FRAME* parent, const wxString& message )
    : DIALOG_ANNOTATE_BASE( parent )
{
    m_Parent = parent;

    if( !message.IsEmpty() )
    {
        m_userMessage->SetLabelText( message );
        m_userMessage->Show( true );

        m_rbScope->Enable( false );
        m_cbKeepDlgOpen->Show( false );
    }

    m_MessageWindow->SetLabel( _( "Annotation Messages:" ) );

    InitValues();
    Layout();

    // When all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();
}


DIALOG_ANNOTATE::~DIALOG_ANNOTATE()
{
    m_Config->Write( KEY_ANNOTATE_SORT_OPTION, GetSortOrder() );
    m_Config->Write( KEY_ANNOTATE_ALGO_OPTION, GetAnnotateAlgo() );
    m_Config->Write( KEY_ANNOTATE_KEEP_OPEN_OPTION, GetAnnotateKeepOpen() );
    m_Config->Write( KEY_ANNOTATE_SKIP_CONFIRMATION, GetAnnotateSkipConfirmation() );

    m_Config->Write( KEY_ANNOTATE_MESSAGES_FILTER,
                    (long) m_MessageWindow->GetVisibleSeverities() );
}


void DIALOG_ANNOTATE::InitValues()
{
    m_Config = Kiface().KifaceSettings();
    long option;

    // These are always reset to attempt to keep the user out of trouble...
    m_rbScope->SetSelection( 0 );
    m_rbOptions->SetSelection( 0 );

    m_Config->Read( KEY_ANNOTATE_SORT_OPTION, &option, 0L );
    switch( option )
    {
    default:
    case 0:
        m_rbSortBy_X_Position->SetValue( 1 );
        break;

    case 1:
        m_rbSortBy_Y_Position->SetValue( 1 );
        break;
    }

    m_Config->Read( KEY_ANNOTATE_ALGO_OPTION, &option, 0L );
    switch( option )
    {
    default:
    case 0:
        m_rbFirstFree->SetValue( 1 );
        break;

    case 1:
        m_rbSheetX100->SetValue( 100 );
        break;

    case 2:
        m_rbSheetX1000->SetValue( 1000 );
        break;
    }
    m_textNumberAfter->SetValue( wxT( "0" ) );

    m_Config->Read( KEY_ANNOTATE_KEEP_OPEN_OPTION, &option, 0L );
    m_cbKeepDlgOpen->SetValue( option );

    m_Config->Read( KEY_ANNOTATE_SKIP_CONFIRMATION, &option, 0L );
    m_cbSkipConfirmation->SetValue( option );

    annotate_down_right_bitmap->SetBitmap( KiBitmap( annotate_down_right_xpm ) );
    annotate_right_down_bitmap->SetBitmap( KiBitmap( annotate_right_down_xpm ) );

    int severities = m_Config->Read( KEY_ANNOTATE_MESSAGES_FILTER, -1l );
    m_MessageWindow->SetVisibleSeverities( severities );

    m_MessageWindow->MsgPanelSetMinSize( wxSize( -1, 160 ) );

    m_btnApply->SetDefault();
}


void DIALOG_ANNOTATE::OnApplyClick( wxCommandEvent& event )
{
    int         response;
    wxString    message;

    // Ask for confirmation of destructive actions unless the user asked us not to.
    if( GetResetItems() && !GetAnnotateSkipConfirmation() )
    {
        if( GetLevel() )
            message += _( "Clear and annotate all of the symbols on the entire schematic?" );
        else
            message += _( "Clear and annotate all of the symbols on the current sheet?" );

        message += _( "\n\nThis operation will change the current annotation and cannot be undone." );

        response = wxMessageBox( message, wxT( "" ), wxICON_EXCLAMATION | wxOK | wxCANCEL );

        if( response == wxCANCEL )
            return;
    }

    m_MessageWindow->Clear();
    REPORTER& reporter = m_MessageWindow->Reporter();
    m_MessageWindow->SetLazyUpdate( true );     // Don't update after each message

    m_Parent->AnnotateComponents( GetLevel(), (ANNOTATE_ORDER_T) GetSortOrder(),
                                  (ANNOTATE_OPTION_T) GetAnnotateAlgo(), GetStartNumber(),
                                  GetResetItems() , true, GetLockUnits(), reporter );

    m_MessageWindow->Flush( true );                   // Now update to show all messages

    m_Parent->GetCanvas()->Refresh();

    m_btnClear->Enable();

    // Don't close dialog if there are things the user needs to address
    if( reporter.HasMessage() )
        return;

    if( m_userMessage->IsShown() || !GetAnnotateKeepOpen() )
    {
        // Close the dialog by calling the default handler for a wxID_OK event
        event.SetId( wxID_OK );
        event.Skip();
    }
}


void DIALOG_ANNOTATE::OnClearAnnotationCmpClick( wxCommandEvent& event )
{
    int         response;
    wxString    message;

    if( !GetAnnotateSkipConfirmation() )
    {
        if( GetLevel() )
            message = _( "Clear the existing annotation for the entire schematic?" );
        else
            message = _( "Clear the existing annotation for the current sheet?" );

        message += _( "\n\nThis operation will clear the existing annotation and cannot be undone." );
        response = wxMessageBox( message, wxT( "" ), wxICON_EXCLAMATION | wxOK | wxCANCEL );

        if( response == wxCANCEL )
            return;
    }

    m_Parent->DeleteAnnotation( GetLevel() ? false : true );
    m_btnClear->Enable( false );
}


bool DIALOG_ANNOTATE::GetLevel()
{
    return m_rbScope->GetSelection() == 0;
}


bool DIALOG_ANNOTATE::GetResetItems()
{
    return m_rbOptions->GetSelection() >= 1;
}


bool DIALOG_ANNOTATE::GetLockUnits()
{
    return m_rbOptions->GetSelection() == 2;
}


int DIALOG_ANNOTATE::GetSortOrder()
{
    if( m_rbSortBy_Y_Position->GetValue() )
        return 1;
    else
        return 0;
}


int DIALOG_ANNOTATE::GetAnnotateAlgo()
{
    if( m_rbSheetX100->GetValue() )
        return 1;
    else if( m_rbSheetX1000->GetValue() )
        return 2;
    else
        return 0;
}


int DIALOG_ANNOTATE::GetStartNumber()
{
    return ValueFromString( EDA_UNITS_T::UNSCALED_UNITS, m_textNumberAfter->GetValue() );
}


void SCH_EDIT_FRAME::OnAnnotate( wxCommandEvent& event )
{
    if( !m_annotateDialog )
    {
        m_annotateDialog = new DIALOG_ANNOTATE( this, wxEmptyString );
        m_annotateDialog->Show( true );
    }
    else    // The dialog is just not visible
    {
        m_annotateDialog->Show( true );
    }
}


int SCH_EDIT_FRAME::ModalAnnotate( const wxString& aMessage )
{
    DIALOG_ANNOTATE dlg( this, aMessage );

    return dlg.ShowModal();
}
