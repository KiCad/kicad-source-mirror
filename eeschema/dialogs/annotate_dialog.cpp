/**
 * @file annotate_dialog.cpp
 * @brief Annotation dialog functions.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2011 jean-pierre Charras <jean-pierre.charras@gipsa-lab.inpg.fr>
  * Copyright (C) 1992-2011 Kicad Developers, see change_log.txt for contributors.
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
#include <appl_wxstruct.h>
#include <wxEeschemaStruct.h>
#include <class_drawpanel.h>

#include <annotate_dialog.h>

#define KEY_ANNOTATE_SORT_OPTION wxT("AnnotateSortOption")
#define KEY_ANNOTATE_ALGO_OPTION wxT("AnnotateAlgoOption")


DIALOG_ANNOTATE::DIALOG_ANNOTATE( SCH_EDIT_FRAME* parent )
    : DIALOG_ANNOTATE_BASE( parent )
{
    m_Parent = parent;
    InitValues();
    Layout();
    GetSizer()->SetSizeHints(this);
    Centre();
}


/*********************************/
void DIALOG_ANNOTATE::InitValues()
/*********************************/
{
    m_Config = wxGetApp().GetSettings();
    SetFocus();	// needed to close dialog by escape key
    if( m_Config )
    {
        long option;
        m_Config->Read(KEY_ANNOTATE_SORT_OPTION, &option, 0l);
        switch( option )
        {
            default:
            case 0:
                m_rbSortBy_X_Position->SetValue(1);
                break;

            case 1:
                m_rbSortBy_Y_Position->SetValue(1);
                break;

            case 2:
                m_rbUseIncremental->SetValue(1);
                break;
            }

        m_Config->Read(KEY_ANNOTATE_ALGO_OPTION, &option, 0l);
        switch( option )
        {
            default:
            case 0:
                m_rbUseIncremental->SetValue(1);
                break;

            case 1:
                m_rbUseSheetNum->SetValue(1);
                break;

            case 2:
                m_rbStartSheetNumLarge->SetValue(1);
                break;
        }
    }

    annotate_down_right_bitmap->SetBitmap( KiBitmap( annotate_down_right_xpm ) );
    annotate_right_down_bitmap->SetBitmap( KiBitmap( annotate_right_down_xpm ) );

    m_btnApply->SetDefault();
}


/*********************************************************/
void DIALOG_ANNOTATE::OnApplyClick( wxCommandEvent& event )
/*********************************************************/
{
    int response;
    wxString message;

    if( m_Config )
    {
        m_Config->Write(KEY_ANNOTATE_SORT_OPTION, GetSortOrder());
        m_Config->Write(KEY_ANNOTATE_ALGO_OPTION, GetAnnotateAlgo());
    }

    if( GetResetItems() )
        message = _( "Clear and annotate all of the components " );
    else
        message = _( "Annotate only the unannotated components " );

    if( GetLevel() )
        message += _( "on the entire schematic?" );
    else
        message += _( "on the current sheet?" );

    message += _( "\n\nThis operation will change the current annotation and cannot be undone." );
    response = wxMessageBox( message, wxT( "" ), wxICON_EXCLAMATION | wxOK | wxCANCEL );

    if (response == wxCANCEL)
        return;

    m_Parent->AnnotateComponents( GetLevel(), (ANNOTATE_ORDER_T) GetSortOrder(),
                                  (ANNOTATE_OPTION_T) GetAnnotateAlgo(),
                                  GetResetItems() , true );
    m_Parent->GetCanvas()->Refresh();

    m_btnClear->Enable();
}


/************************************************************************/
void DIALOG_ANNOTATE::OnClearAnnotationCmpClick( wxCommandEvent& event )
/************************************************************************/
{
    int response;

    wxString message = _( "Clear the existing annotation for " );
    if( GetLevel() )
        message += _( "the entire schematic?" );
    else
        message += _( "the current sheet?" );

    message += _( "\n\nThis operation will clear the existing annotation and cannot be undone." );
    response = wxMessageBox( message, wxT( "" ), wxICON_EXCLAMATION | wxOK | wxCANCEL );

    if (response == wxCANCEL)
        return;

    m_Parent->DeleteAnnotation( GetLevel() ? false : true );
    m_Parent->GetCanvas()->Refresh();

    m_btnClear->Enable(false);
}


/************************************************************/
void DIALOG_ANNOTATE::OnCancelClick( wxCommandEvent& event )
/************************************************************/
{
    if( IsModal() )
        EndModal( wxID_CANCEL );
    else
    {
        SetReturnCode( wxID_CANCEL );
        this->Show( false );
    }
}


/**************************************/
bool DIALOG_ANNOTATE::GetLevel( void )
/**************************************/
{
    return m_rbEntireSchematic->GetValue();
}

/******************************************/
bool DIALOG_ANNOTATE::GetResetItems( void )
/******************************************/
{
    return m_rbResetAnnotation->GetValue();
}

int DIALOG_ANNOTATE::GetSortOrder( void )
/**
 * @return 0 if annotation by X position,
 *         1 if annotation by Y position,
 *         2 if annotation by value
 */
{
    if ( m_rbSortBy_X_Position->GetValue() )
        return 0;
    if ( m_rbSortBy_Y_Position->GetValue() )
        return 1;
    return 2;
}

int DIALOG_ANNOTATE::GetAnnotateAlgo( void )
/**
 * @return 0 if annotation using first not used Id value
 *         1 if annotation using first not used Id value inside sheet num * 100 to  sheet num * 100 + 99
 *         2 if annotation using first nhot used Id value inside sheet num * 1000 to  sheet num * 1000 + 999
 */
{
    if ( m_rbUseIncremental->GetValue() )
        return 0;
    if ( m_rbUseSheetNum->GetValue() )
        return 1;
    return 2;
}
