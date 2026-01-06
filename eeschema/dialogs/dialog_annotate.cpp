/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2017 jean-pierre Charras jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

    bool TransferDataToWindow() override;

private:
    /// Initialize member variables.
    void OnOptionChanged( wxCommandEvent& event ) override;
    void OnClearAnnotationClick( wxCommandEvent& event ) override;
    void OnCloseClick( wxCommandEvent& event ) override;
    void OnClose( wxCloseEvent& event ) override;
    void OnAnnotateClick( wxCommandEvent& event ) override;

    ANNOTATE_SCOPE_T GetScope()
    {
        if( m_rbScope_Schematic->GetValue() )
            return ANNOTATE_ALL;
        else if( m_rbScope_Sheet->GetValue() )
            return ANNOTATE_CURRENT_SHEET;
        else
            return ANNOTATE_SELECTION;
    }

    ANNOTATE_ORDER_T GetSortOrder()
    {
        if( m_rbSortBy_Y_Position->GetValue() )
            return SORT_BY_Y_POSITION;
        else
            return SORT_BY_X_POSITION;
    }

    ANNOTATE_ALGO_T GetAnnotateAlgo()
    {
        if( m_rbSheetX100->GetValue() )
            return SHEET_NUMBER_X_100;
        else if( m_rbSheetX1000->GetValue() )
            return SHEET_NUMBER_X_1000;
        else
            return INCREMENTAL_BY_REF;
    }

    int GetStartNumber()
    {
        return (int) EDA_UNIT_UTILS::UI::ValueFromString( m_textNumberAfter->GetValue() );
    }

private:
    SCH_EDIT_FRAME* m_Parent;
};


DIALOG_ANNOTATE::DIALOG_ANNOTATE( SCH_EDIT_FRAME* parent, const wxString& message ) :
        DIALOG_ANNOTATE_BASE( parent )
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

    annotate_down_right_bitmap->SetBitmap( KiBitmapBundle( BITMAPS::annotate_down_right ) );
    annotate_right_down_bitmap->SetBitmap( KiBitmapBundle( BITMAPS::annotate_right_down ) );

    m_MessageWindow->MsgPanelSetMinSize( wxSize( -1, 160 ) );

    Layout();

    // When all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


DIALOG_ANNOTATE::~DIALOG_ANNOTATE()
{
    // We still save/restore to config (instead of letting DIALOG_SHIM do it) because we also
    // allow editing of these settings in preferences.

    EESCHEMA_SETTINGS* cfg = static_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );

    cfg->m_AnnotatePanel.options = m_rbReset_Annotations->GetValue() ? 1 : 0;

    if( m_rbScope_Schematic->IsEnabled() )
    {
        cfg->m_AnnotatePanel.scope = GetScope();
        cfg->m_AnnotatePanel.recursive = m_checkRecursive->GetValue();
    }

    cfg->m_AnnotatePanel.regroup_units = m_checkRegroupUnits->GetValue();

    int sort = GetSortOrder();
    int method = GetAnnotateAlgo();
    int startNum = GetStartNumber();

    if( SCH_EDIT_FRAME* schFrame = dynamic_cast<SCH_EDIT_FRAME*>( m_parentFrame ) )
    {
        SCHEMATIC_SETTINGS& projSettings = schFrame->Schematic().Settings();
        bool modified = false;

        if( projSettings.m_AnnotateSortOrder != sort )
        {
            projSettings.m_AnnotateSortOrder = sort;
            modified = true;
        }

        if( projSettings.m_AnnotateMethod != method )
        {
            projSettings.m_AnnotateMethod = method;
            modified = true;
        }

        if( projSettings.m_AnnotateStartNum != startNum )
        {
            projSettings.m_AnnotateStartNum = startNum;
            modified = true;
        }

        if( modified )
            schFrame->OnModify();
    }
}


bool DIALOG_ANNOTATE::TransferDataToWindow()
{
    // We still save/restore to config (instead of letting DIALOG_SHIM do it) because we also
    // allow editing of these settings in preferences.
    EESCHEMA_SETTINGS* cfg = static_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );

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

    bool resetAnnotation = cfg->m_AnnotatePanel.options >= 1;
    m_rbReset_Annotations->SetValue( resetAnnotation );
    m_rbKeep_Annotations->SetValue( !resetAnnotation );

    m_checkRegroupUnits->SetValue( cfg->m_AnnotatePanel.regroup_units );
    m_checkRegroupUnits->Enable( cfg->m_AnnotatePanel.options >= 1 );

    if( SCH_EDIT_FRAME* schFrame = dynamic_cast<SCH_EDIT_FRAME*>( m_parentFrame ) )
    {
        SCHEMATIC_SETTINGS& projSettings = schFrame->Schematic().Settings();

        switch( projSettings.m_AnnotateSortOrder )
        {
        default:
        case SORT_BY_X_POSITION: m_rbSortBy_X_Position->SetValue( true ); break;
        case SORT_BY_Y_POSITION: m_rbSortBy_Y_Position->SetValue( true ); break;
        }

        switch( projSettings.m_AnnotateMethod )
        {
        default:
        case INCREMENTAL_BY_REF:  m_rbFirstFree->SetValue( true );  break;
        case SHEET_NUMBER_X_100:  m_rbSheetX100->SetValue( true );  break;
        case SHEET_NUMBER_X_1000: m_rbSheetX1000->SetValue( true ); break;
        }

        m_textNumberAfter->SetValue( wxString::Format( wxT( "%d" ), projSettings.m_AnnotateStartNum ) );
    }

    return true;
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


void DIALOG_ANNOTATE::OnAnnotateClick( wxCommandEvent& event )
{
    SCH_COMMIT commit( m_Parent );

    m_MessageWindow->Clear();
    REPORTER& reporter = m_MessageWindow->Reporter();
    m_MessageWindow->SetLazyUpdate( true );     // Don't update after each message

    bool resetAnnotation = m_rbReset_Annotations->GetValue();
    bool regroupUnits = resetAnnotation && m_checkRegroupUnits->GetValue();

    m_Parent->AnnotateSymbols( &commit, GetScope(), GetSortOrder(), GetAnnotateAlgo(),
                               m_checkRecursive->GetValue(), GetStartNumber(), resetAnnotation,
                               regroupUnits, true, reporter );

    commit.Push( _( "Annotate" ) );

    m_MessageWindow->Flush( true ); // Now update to show all messages
}


void DIALOG_ANNOTATE::OnClearAnnotationClick( wxCommandEvent& event )
{
    m_MessageWindow->Clear();
    m_Parent->DeleteAnnotation( GetScope(), m_checkRecursive->GetValue(), m_MessageWindow->Reporter() );

    m_MessageWindow->Flush( true ); // Now update to show all messages
}


void DIALOG_ANNOTATE::OnOptionChanged( wxCommandEvent& event )
{
    m_checkRegroupUnits->Enable( m_rbReset_Annotations->GetValue() );

    m_sdbSizer1OK->Enable( true );
    m_sdbSizer1OK->SetDefault();
}


void SCH_EDIT_FRAME::OnAnnotate()
{
    DIALOG_ANNOTATE* dlg = static_cast<DIALOG_ANNOTATE*>( wxWindow::FindWindowByName( DLG_WINDOW_NAME ) );

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
