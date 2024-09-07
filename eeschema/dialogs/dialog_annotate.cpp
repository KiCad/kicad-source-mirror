/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2017 jean-pierre Charras jp.charras at wanadoo.fr
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <sch_edit_frame.h>
#include <bitmaps.h>
#include <dialog_annotate_base.h>
#include <eeschema_settings.h>
#include <kiface_base.h>
#include <widgets/wx_html_report_panel.h>
#include <schematic.h>
#include <sch_commit.h>

// A window name for the annotate dialog to retrieve is if not destroyed
#define DLG_WINDOW_NAME "DialogAnnotateWindowName"


/**
 * A dialog to set/clear reference designators of a schematic with different options.
 */
class DIALOG_ANNOTATE: public DIALOG_ANNOTATE_BASE
{
public:
    DIALOG_ANNOTATE( SCH_EDIT_FRAME* parent, const wxString& message );
    ~DIALOG_ANNOTATE();

private:
    /// Initialize member variables.
    void InitValues();
    void OnOptionChanged( wxCommandEvent& event ) override;
    void OnClearAnnotationClick( wxCommandEvent& event ) override;
    void OnCloseClick( wxCommandEvent& event ) override;
    void OnClose( wxCloseEvent& event ) override;
    void OnApplyClick( wxCommandEvent& event ) override;

    // User functions:
    bool GetResetItems();

    ANNOTATE_SCOPE_T GetScope();

    bool GetRecursive();

    ANNOTATE_ORDER_T GetSortOrder();

    ANNOTATE_ALGO_T GetAnnotateAlgo();

    int GetStartNumber();

    SCH_EDIT_FRAME* m_Parent;
};


DIALOG_ANNOTATE::DIALOG_ANNOTATE( SCH_EDIT_FRAME* parent, const wxString& message )
    : DIALOG_ANNOTATE_BASE( parent )
{
    SetName( DLG_WINDOW_NAME );
    m_Parent = parent;

    if( !message.IsEmpty() )
    {
        m_infoBar->RemoveAllButtons();
        m_infoBar->ShowMessage( message );

        m_rbScope_Schematic->SetValue( true );
        m_rbScope_Schematic->Enable( false );
    }

    m_MessageWindow->SetLabel( _( "Annotation Messages:" ) );
    m_MessageWindow->SetFileName( Prj().GetProjectPath() + wxT( "report.txt" ) );

    SetupStandardButtons( { { wxID_OK,     _( "Annotate" ) },
                            { wxID_CANCEL, _( "Close" )    } } );

    InitValues();
    Layout();

    // When all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


DIALOG_ANNOTATE::~DIALOG_ANNOTATE()
{
    auto cfg = static_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );

    cfg->m_AnnotatePanel.sort_order = GetSortOrder();
    cfg->m_AnnotatePanel.method = GetAnnotateAlgo();
    cfg->m_AnnotatePanel.options = m_rbOptions->GetSelection();

    if( m_rbScope_Schematic->IsEnabled() )
    {
        cfg->m_AnnotatePanel.scope = GetScope();
        cfg->m_AnnotatePanel.recursive = GetRecursive();
    }

    cfg->m_AnnotatePanel.messages_filter = m_MessageWindow->GetVisibleSeverities();

    // Get the "start annotation after" value from dialog and update project settings if changed
    int startNum = GetStartNumber();
    SCH_EDIT_FRAME* schFrame = dynamic_cast<SCH_EDIT_FRAME*>( m_parentFrame );

    if( schFrame )
    {
        SCHEMATIC_SETTINGS& projSettings = schFrame->Schematic().Settings();

        // If the user has updated the start annotation number then update the project file.
        // We manually update the project file here in case the user has changed the value
        // and just clicked the "Close" button on the annotation dialog.

        if( projSettings.m_AnnotateStartNum != startNum )
        {
            projSettings.m_AnnotateStartNum = startNum;
            schFrame->OnModify();
        }
    }
}


void DIALOG_ANNOTATE::InitValues()
{
    EESCHEMA_SETTINGS* cfg = static_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );
    int option;

    if( m_rbScope_Schematic->IsEnabled() )
    {
        switch( cfg->m_AnnotatePanel.scope )
        {
        default:
        case ANNOTATE_ALL:           m_rbScope_Schematic->SetValue( true ); break;
        case ANNOTATE_CURRENT_SHEET: m_rbScope_Sheet->SetValue( true );     break;
        case ANNOTATE_SELECTION:     m_rbScope_Selection->SetValue( true ); break;
        }

        m_checkRecursive->SetValue( cfg->m_AnnotatePanel.recursive );
    }


    m_rbOptions->SetSelection( cfg->m_AnnotatePanel.options );

    option = cfg->m_AnnotatePanel.sort_order;

    switch( option )
    {
    default:
    case SORT_BY_X_POSITION: m_rbSortBy_X_Position->SetValue( true ); break;
    case SORT_BY_Y_POSITION: m_rbSortBy_Y_Position->SetValue( true ); break;
    }

    option = cfg->m_AnnotatePanel.method;

    switch( option )
    {
    default:
    case INCREMENTAL_BY_REF:  m_rbFirstFree->SetValue( true );  break;
    case SHEET_NUMBER_X_100:  m_rbSheetX100->SetValue( true );  break;
    case SHEET_NUMBER_X_1000: m_rbSheetX1000->SetValue( true ); break;
    }

    int annotateStartNum = 0; // Default "start after" value for annotation

    // See if we can get a "start after" value from the project settings
    SCH_EDIT_FRAME* schFrame = dynamic_cast<SCH_EDIT_FRAME*>( m_parentFrame );

    if( schFrame )
    {
        SCHEMATIC_SETTINGS& projSettings = schFrame->Schematic().Settings();
        annotateStartNum = projSettings.m_AnnotateStartNum;
    }

    m_textNumberAfter->SetValue( wxString::Format( wxT( "%d" ), annotateStartNum ) );

    annotate_down_right_bitmap->SetBitmap( KiBitmapBundle( BITMAPS::annotate_down_right ) );
    annotate_right_down_bitmap->SetBitmap( KiBitmapBundle( BITMAPS::annotate_right_down ) );

    m_MessageWindow->SetVisibleSeverities( cfg->m_AnnotatePanel.messages_filter );

    m_MessageWindow->MsgPanelSetMinSize( wxSize( -1, 160 ) );
}


// This is a modeless dialog so we have to handle these ourselves.
void DIALOG_ANNOTATE::OnCloseClick( wxCommandEvent& event )
{
    Close();
}


void DIALOG_ANNOTATE::OnClose( wxCloseEvent& event )
{
    Destroy();
}


void DIALOG_ANNOTATE::OnApplyClick( wxCommandEvent& event )
{
    SCH_COMMIT commit( m_Parent );

    m_MessageWindow->Clear();
    REPORTER& reporter = m_MessageWindow->Reporter();
    m_MessageWindow->SetLazyUpdate( true );     // Don't update after each message

    m_Parent->AnnotateSymbols( &commit, GetScope(), GetSortOrder(), GetAnnotateAlgo(),
                               GetRecursive(), GetStartNumber(), GetResetItems(), true, reporter );

    commit.Push( _( "Annotate" ) );

    m_MessageWindow->Flush( true );             // Now update to show all messages

    m_sdbSizer1Cancel->SetDefault();

    // Don't close dialog if there are things the user needs to address
    if( reporter.HasMessageOfSeverity( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING ) )
        return;

    if( m_infoBar->IsShown() )
    {
        // Close the dialog by calling the default handler for a wxID_OK event
        event.SetId( wxID_OK );
        event.Skip();
    }
}


void DIALOG_ANNOTATE::OnClearAnnotationClick( wxCommandEvent& event )
{
    m_Parent->DeleteAnnotation( GetScope(), GetRecursive() );
}


void DIALOG_ANNOTATE::OnOptionChanged( wxCommandEvent& event )
{
    m_sdbSizer1OK->Enable( true );
    m_sdbSizer1OK->SetDefault();
}


bool DIALOG_ANNOTATE::GetResetItems()
{
    return m_rbOptions->GetSelection() >= 1;
}


ANNOTATE_SCOPE_T DIALOG_ANNOTATE::GetScope()
{
    if( m_rbScope_Schematic->GetValue() )
        return ANNOTATE_ALL;
    else if( m_rbScope_Sheet->GetValue() )
        return ANNOTATE_CURRENT_SHEET;
    else
        return ANNOTATE_SELECTION;
}


bool DIALOG_ANNOTATE::GetRecursive()
{
    return m_checkRecursive->GetValue();
}


ANNOTATE_ORDER_T DIALOG_ANNOTATE::GetSortOrder()
{
    if( m_rbSortBy_Y_Position->GetValue() )
        return SORT_BY_Y_POSITION;
    else
        return SORT_BY_X_POSITION;
}


ANNOTATE_ALGO_T DIALOG_ANNOTATE::GetAnnotateAlgo()
{
    if( m_rbSheetX100->GetValue() )
        return SHEET_NUMBER_X_100;
    else if( m_rbSheetX1000->GetValue() )
        return SHEET_NUMBER_X_1000;
    else
        return INCREMENTAL_BY_REF;
}


int DIALOG_ANNOTATE::GetStartNumber()
{
    return EDA_UNIT_UTILS::UI::ValueFromString( m_textNumberAfter->GetValue() );
}


void SCH_EDIT_FRAME::OnAnnotate( wxCommandEvent& event )
{
    DIALOG_ANNOTATE* dlg = static_cast<DIALOG_ANNOTATE*> ( wxWindow::FindWindowByName( DLG_WINDOW_NAME ) );

    if( !dlg )
    {
        dlg = new DIALOG_ANNOTATE( this, wxEmptyString );
        dlg->Show( true );
    }
    else    // The dialog is already opened, perhaps not visible
    {
        dlg->Show( true );
    }
}


int SCH_EDIT_FRAME::ModalAnnotate( const wxString& aMessage )
{
    DIALOG_ANNOTATE dlg( this, aMessage );

    return dlg.ShowModal();
}
