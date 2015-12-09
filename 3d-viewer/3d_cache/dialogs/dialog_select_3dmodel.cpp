/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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

#include <iostream>
#include "3d_cache.h"
#include "dialog_select_3dmodel.h"
#include "panel_prev_model.h"

static S3D_CACHE* mm = NULL;
static wxFileDialog* fm = NULL;

static wxWindow* mkPreviewPanel( wxWindow* aParent )
{
    PANEL_PREV_3D* pp = new PANEL_PREV_3D( aParent, true );
    pp->SetModelManager( mm );
    pp->SetFileSelectorDlg( fm );

    return (wxWindow*)pp;
}

wxBEGIN_EVENT_TABLE( DLG_SEL_3DMODEL, wxFileDialog )
        EVT_BUTTON( wxID_OK, DLG_SEL_3DMODEL::OnOK )
        EVT_BUTTON( wxID_CANCEL, DLG_SEL_3DMODEL::OnExit )
wxEND_EVENT_TABLE()


DLG_SEL_3DMODEL::DLG_SEL_3DMODEL( wxWindow* aParent, S3D_CACHE* aManager,
    const wxString& aDefaultDir, int aFilterIndex )
    : wxFileDialog( aParent, _( "Select a 3D Model" ), aDefaultDir )
{
    m_manager = aManager;
    mm = aManager;
    fm = this;

    long ws = GetWindowStyleFlag();
    ws |= wxFD_FILE_MUST_EXIST;
    SetWindowStyleFlag( ws );
    SetExtraControlCreator( mkPreviewPanel );

    if( NULL != m_manager )
    {
        std::list< wxString > const* fl = m_manager->GetFileFilters();
        std::list< wxString >::const_iterator sL = fl->begin();
        std::list< wxString >::const_iterator eL = fl->end();
        wxString filter;

        while( sL != eL )
        {
            filter.Append( *sL );

            ++sL;

            if( sL != eL )
                filter.Append( wxT( "|" ) );

        }

        if( !filter.empty() )
            SetWildcard( filter );

        if( aFilterIndex >= 0 && aFilterIndex < (int)fl->size() )
            SetFilterIndex( aFilterIndex );
    }

    return;
}


void DLG_SEL_3DMODEL::OnExit( wxCommandEvent& event )
{
    if( IsModal() )
        EndModal( wxID_EXIT );
    else
        Close( true );

    return;
}


void DLG_SEL_3DMODEL::OnOK( wxCommandEvent& event )
{
    if( IsModal() )
        EndModal( wxID_OK );
    else
        Close( true );

    return;
}


void DLG_SEL_3DMODEL::GetModelData( S3D_INFO* aModel )
{
    PANEL_PREV_3D* pp =  (PANEL_PREV_3D*)GetExtraControl();

    if( pp )
        pp->GetModelData( aModel );

    return;
}
