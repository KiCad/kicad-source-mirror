/**
 * @file dialog_annotate.cpp
 * @brief Annotation dialog functions.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2012 jean-pierre Charras <jean-pierre.charras@ujf-grenoble.fr>
  * Copyright (C) 1992-2012 Kicad Developers, see change_log.txt for contributors.
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
#include <schframe.h>
#include <class_drawpanel.h>

#include <invoke_sch_dialog.h>
#include <dialog_annotate_base.h>
#include <kiface_i.h>

#define KEY_ANNOTATE_SORT_OPTION          wxT( "AnnotateSortOption" )
#define KEY_ANNOTATE_ALGO_OPTION          wxT( "AnnotateAlgoOption" )
#define KEY_ANNOTATE_KEEP_OPEN_OPTION     wxT( "AnnotateKeepOpenOption" )
#define KEY_ANNOTATE_ASK_FOR_CONFIRMATION wxT( "AnnotateRequestConfirmation" )


class wxConfigBase;


/**
 * Class DIALOG_ANNOTATE
 */
class DIALOG_ANNOTATE: public DIALOG_ANNOTATE_BASE
{
public:
    DIALOG_ANNOTATE( SCH_EDIT_FRAME* parent );


private:
    SCH_EDIT_FRAME* m_Parent;
    wxConfigBase*   m_Config;

    /// Initialises member variables
    void InitValues();
    void OnCancelClick( wxCommandEvent& event );
    void OnClearAnnotationCmpClick( wxCommandEvent& event );
    void OnApplyClick( wxCommandEvent& event );

    // User functions:
    bool GetLevel();
    bool GetResetItems();
    bool GetLockUnits();

    /**
     * Function GetSortOrder
     * @return 0 if annotation by X position,
     *         1 if annotation by Y position,
     *         2 if annotation by value
     */
    int GetSortOrder();

    /**
     * Function GetAnnotateAlgo
     * @return 0 if annotation using first not used Id value
     *         1 if annotation using first not used Id value inside sheet num * 100 to  sheet num * 100 + 99
     *         2 if annotation using first nhot used Id value inside sheet num * 1000 to  sheet num * 1000 + 999
     */
    int GetAnnotateAlgo();

    bool GetAnnotateKeepOpen()
    {
        return m_cbKeepDlgOpen->GetValue();
    }

    bool GetAnnotateAskForConfirmation()
    {
        return m_cbAskForConfirmation->GetValue();
    }
};



DIALOG_ANNOTATE::DIALOG_ANNOTATE( SCH_EDIT_FRAME* parent )
    : DIALOG_ANNOTATE_BASE( parent )
{
    m_Parent = parent;

    InitValues();
    Layout();
    GetSizer()->SetSizeHints( this );
    Centre();
}


void DIALOG_ANNOTATE::InitValues()
{
    m_Config = Kiface().KifaceSettings();

    if( m_Config )
    {
        long option;

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

        case 2:
            m_rbUseIncremental->SetValue( 1 );
            break;
        }

        m_Config->Read( KEY_ANNOTATE_ALGO_OPTION, &option, 0L );
        switch( option )
        {
        default:
        case 0:
            m_rbUseIncremental->SetValue( 1 );
            break;

        case 1:
            m_rbUseSheetNum->SetValue( 1 );
            break;

        case 2:
            m_rbStartSheetNumLarge->SetValue( 1 );
            break;
        }


        m_Config->Read( KEY_ANNOTATE_KEEP_OPEN_OPTION, &option, 0L );
        m_cbKeepDlgOpen->SetValue( option );


        m_Config->Read( KEY_ANNOTATE_ASK_FOR_CONFIRMATION, &option, 1L );
        m_cbAskForConfirmation->SetValue( option );
    }

    annotate_down_right_bitmap->SetBitmap( KiBitmap( annotate_down_right_xpm ) );
    annotate_right_down_bitmap->SetBitmap( KiBitmap( annotate_right_down_xpm ) );

    m_btnApply->SetDefault();
}


void DIALOG_ANNOTATE::OnApplyClick( wxCommandEvent& event )
{
    int         response;
    wxString    message;

    if( m_Config )
    {
        m_Config->Write( KEY_ANNOTATE_SORT_OPTION, GetSortOrder() );
        m_Config->Write( KEY_ANNOTATE_ALGO_OPTION, GetAnnotateAlgo() );
        m_Config->Write( KEY_ANNOTATE_KEEP_OPEN_OPTION, GetAnnotateKeepOpen() );
        m_Config->Write( KEY_ANNOTATE_ASK_FOR_CONFIRMATION, GetAnnotateAskForConfirmation() );
    }

    // Display a message info if we always ask for confirmation
    // or if a reset of the previous annotation is asked.
    bool promptUser = GetAnnotateAskForConfirmation();

    if( GetResetItems() )
    {
        if( GetLevel() )
            message += _( "Clear and annotate all of the components on the entire schematic?" );
        else
            message += _( "Clear and annotate all of the components on the current sheet?" );
        promptUser = true;
    }
    else
    {
        if( GetLevel() )
            message += _( "Annotate only the unannotated components on the entire schematic?" );
        else
            message += _( "Annotate only the unannotated components on the current sheet?" );
    }

    message += _( "\n\nThis operation will change the current annotation and cannot be undone." );

    if( promptUser )
    {
        // TODO(hzeller): ideally, this would be a wxMessageDialog that contains
        // a checkbox asking the 'ask for confirmation' flag for better
        // discoverability (and it should only show in the 'benign' case, so if
        // !GetResetItems())
        response = wxMessageBox( message, wxT( "" ), wxICON_EXCLAMATION | wxOK | wxCANCEL );

        if( response == wxCANCEL )
            return;
    }

    m_Parent->AnnotateComponents( GetLevel(), (ANNOTATE_ORDER_T) GetSortOrder(),
                                  (ANNOTATE_OPTION_T) GetAnnotateAlgo(),
                                  GetResetItems() , true, GetLockUnits() );
    m_Parent->GetCanvas()->Refresh();

    m_btnClear->Enable();

    if( !GetAnnotateKeepOpen() )
    {
        if( IsModal() )
            EndModal( wxID_OK );
        else
        {
            SetReturnCode( wxID_OK );
            this->Show( false );
        }
    }
}


void DIALOG_ANNOTATE::OnClearAnnotationCmpClick( wxCommandEvent& event )
{
    int         response;
    wxString    message;

    if( GetLevel() )
        message = _( "Clear the existing annotation for the entire schematic?" );
    else
        message = _( "Clear the existing annotation for the current sheet?" );

    message += _( "\n\nThis operation will clear the existing annotation and cannot be undone." );
    response = wxMessageBox( message, wxT( "" ), wxICON_EXCLAMATION | wxOK | wxCANCEL );

    if( response == wxCANCEL )
        return;

    m_Parent->DeleteAnnotation( GetLevel() ? false : true );
    m_Parent->GetCanvas()->Refresh();

    m_btnClear->Enable( false );
}


void DIALOG_ANNOTATE::OnCancelClick( wxCommandEvent& event )
{
    if( IsModal() )
        EndModal( wxID_CANCEL );
    else
    {
        SetReturnCode( wxID_CANCEL );
        this->Show( false );
    }
}


bool DIALOG_ANNOTATE::GetLevel()
{
    return m_rbEntireSchematic->GetValue();
}


bool DIALOG_ANNOTATE::GetResetItems()
{
    return m_rbResetAnnotation->GetValue() || m_rbResetButLock->GetValue();
}

bool DIALOG_ANNOTATE::GetLockUnits()
{
    return m_rbResetButLock->GetValue();
}

int DIALOG_ANNOTATE::GetSortOrder()
{
    if( m_rbSortBy_X_Position->GetValue() )
        return 0;

    if( m_rbSortBy_Y_Position->GetValue() )
        return 1;

    return 2;
}


int DIALOG_ANNOTATE::GetAnnotateAlgo()
{
    if( m_rbUseIncremental->GetValue() )
        return 0;

    if( m_rbUseSheetNum->GetValue() )
        return 1;

    return 2;
}


int InvokeDialogAnnotate( SCH_EDIT_FRAME* aCaller )
{
    DIALOG_ANNOTATE dlg( aCaller );

    return dlg.ShowModal();
}
