/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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


#include <wx/msgdlg.h>
#include "3d_cache/dialogs/dlg_3d_pathconfig.h"
#include "3d_cache/3d_filename_resolver.h"

DLG_3D_PATH_CONFIG::DLG_3D_PATH_CONFIG( wxWindow* aParent, S3D_FILENAME_RESOLVER* aResolver ) :
    DLG_3D_PATH_CONFIG_BASE( aParent ), m_resolver( aResolver )
{
    m_Aliases->EnableEditing( true );
    m_Aliases->SetColMinimalWidth( 0, 80 );
    m_Aliases->SetColMinimalWidth( 1, 300 );
    m_Aliases->SetColMinimalWidth( 2, 120 );
    m_Aliases->SetColMinimalAcceptableWidth( 80 );

    if( m_resolver )
    {
        // prohibit these characters in teh alias names: []{}()%~<>"='`;:.,&?/\|$
        m_aliasValidator.SetStyle( wxFILTER_EXCLUDE_CHAR_LIST );
        m_aliasValidator.SetCharExcludes( wxT( "{}[]()%~<>\"='`;:.,&?/\\|$" ) );

        const std::list< S3D_ALIAS >* rpaths = m_resolver->GetPaths();
        size_t listsize = rpaths->size();

        if( listsize > 0 )
            m_curdir = rpaths->front().m_pathexp;

        if( listsize < 3 )
            return;

        listsize = listsize - 2 - m_Aliases->GetNumberRows();

        // note: if the list allocation fails we have bigger problems
        // and there is no point in trying to notify the user here
        if( listsize > 0 && !m_Aliases->InsertRows( 0, listsize ) )
            return;

        std::list< S3D_ALIAS >::const_iterator sL = rpaths->begin();
        std::list< S3D_ALIAS >::const_iterator eL = rpaths->end();
        int nitems = 0;

        // skip the current project dir and KISYS3DMOD
        ++sL;
        ++sL;
        wxGridCellTextEditor* pEdAlias;

        while( sL != eL )
        {
            m_Aliases->SetCellValue( nitems, 0, sL->m_alias );

            if( 0 == nitems )
            {
                m_Aliases->SetCellEditor( nitems, 0, new wxGridCellTextEditor );
                pEdAlias = (wxGridCellTextEditor*) m_Aliases->GetCellEditor( nitems, 0 );
                pEdAlias->SetValidator( m_aliasValidator );
                pEdAlias->DecRef();
            }
            else
            {
                pEdAlias->IncRef();
                m_Aliases->SetCellEditor( nitems, 0, pEdAlias );
            }

            m_Aliases->SetCellValue( nitems, 1, sL->m_pathvar );
            m_Aliases->SetCellValue( nitems++, 2, sL->m_description );

            // TODO: implement a wxGridCellEditor which invokes a wxDirDialog

            ++sL;
        }

        m_Aliases->AutoSize();
    }

    Fit();
    SetMinSize( GetSize() );

    return;
}


bool DLG_3D_PATH_CONFIG::TransferDataFromWindow()
{
    if( NULL == m_resolver )
    {
        wxMessageBox( _( "[BUG] No valid resolver; data will not be updated" ),
            _( "Update 3D search path list" ) );

        return false;
    }

    std::vector<S3D_ALIAS> alist;
    S3D_ALIAS alias;

    int ni = m_Aliases->GetNumberRows();

    if( ni <= 0 )
    {
        // note: UI usability: we should ask a user if they're sure they
        // want to clear the entire path list
        m_resolver->UpdatePathList( alist );
        return true;
    }

    for( int i = 0; i < ni; ++i )
    {
        alias.m_alias = m_Aliases->GetCellValue( i, 0 );
        alias.m_pathvar = m_Aliases->GetCellValue( i, 1 );
        alias.m_description = m_Aliases->GetCellValue( i, 2 );

        if( !alias.m_alias.empty() && !alias.m_pathvar.empty() )
            alist.push_back( alias );

    }

    return m_resolver->UpdatePathList( alist );
}


void DLG_3D_PATH_CONFIG::OnAddAlias( wxCommandEvent& event )
{
    int ni = m_Aliases->GetNumberRows();

    if( m_Aliases->InsertRows( ni, 1 ) )
    {
        wxGridCellTextEditor* pEdAlias;
        pEdAlias = (wxGridCellTextEditor*) m_Aliases->GetCellEditor( 0, 0 );
        m_Aliases->SetCellEditor( ni, 0, pEdAlias );
        m_Aliases->SelectRow( ni, false );
        m_Aliases->AutoSize();
        Fit();

        // TODO: set the editors on any newly created rows
    }

    event.Skip();
    return;
}


void DLG_3D_PATH_CONFIG::OnDelAlias( wxCommandEvent& event )
{
    wxArrayInt sel = m_Aliases->GetSelectedRows();

    if( sel.empty() )
    {
        wxMessageBox( _( "No entry selected" ), _( "Delete alias entry" ) );
        return;
    }

    if( sel.size() > 1 )
    {
        wxMessageBox( _( "Multiple entries selected; please\nselect only one entry" ),
            _( "Delete alias entry" ) );
        return;
    }

    if( m_Aliases->GetNumberRows() > 1 )
    {
        int ni = sel.front();
        m_Aliases->DeleteRows( ni, 1 );

        if( ni >= m_Aliases->GetNumberRows() )
            ni = m_Aliases->GetNumberRows() - 1;

        m_Aliases->SelectRow( ni, false );
        m_Aliases->AutoSize();
        Fit();
    }
    else
    {
        m_Aliases->SetCellValue( 0, 0, wxEmptyString );
        m_Aliases->SetCellValue( 0, 1, wxEmptyString );
        m_Aliases->SetCellValue( 0, 2, wxEmptyString );
    }

    event.Skip();
}


void DLG_3D_PATH_CONFIG::OnAliasMoveUp( wxCommandEvent& event )
{
    wxArrayInt sel = m_Aliases->GetSelectedRows();

    if( sel.empty() )
    {
        wxMessageBox( _( "No entry selected" ), _( "Move alias up" ) );
        return;
    }

    if( sel.size() > 1 )
    {
        wxMessageBox( _( "Multiple entries selected; please\nselect only one entry" ),
                      _( "Move alias up" ) );
        return;
    }

    int ci = sel.front();

    if( ci > 0 )
    {
        S3D_ALIAS al0;
        al0.m_alias = m_Aliases->GetCellValue( ci, 0 );
        al0.m_pathvar = m_Aliases->GetCellValue( ci, 1 );
        al0.m_description = m_Aliases->GetCellValue( ci, 2 );

        int ni = ci - 1;
        m_Aliases->SetCellValue( ci, 0, m_Aliases->GetCellValue( ni, 0 ) );
        m_Aliases->SetCellValue( ci, 1, m_Aliases->GetCellValue( ni, 1 ) );
        m_Aliases->SetCellValue( ci, 2, m_Aliases->GetCellValue( ni, 2 ) );

        m_Aliases->SetCellValue( ni, 0, al0.m_alias );
        m_Aliases->SetCellValue( ni, 1, al0.m_pathvar );
        m_Aliases->SetCellValue( ni, 2, al0.m_description );
        m_Aliases->SelectRow( ni, false );
    }

    event.Skip();
}


void DLG_3D_PATH_CONFIG::OnAliasMoveDown( wxCommandEvent& event )
{
    wxArrayInt sel = m_Aliases->GetSelectedRows();

    if( sel.empty() )
    {
        wxMessageBox( _( "No entry selected" ), _( "Move alias down" ) );
        return;
    }

    if( sel.size() > 1 )
    {
        wxMessageBox( _( "Multiple entries selected; please\nselect only one entry" ),
                      _( "Move alias down" ) );
        return;
    }

    int ni = m_Aliases->GetNumberRows() - 1;
    int ci = sel.front();

    if( ci < ni )
    {
        S3D_ALIAS al0;
        al0.m_alias = m_Aliases->GetCellValue( ci, 0 );
        al0.m_pathvar = m_Aliases->GetCellValue( ci, 1 );
        al0.m_description = m_Aliases->GetCellValue( ci, 2 );

        ni = ci + 1;
        m_Aliases->SetCellValue( ci, 0, m_Aliases->GetCellValue( ni, 0 ) );
        m_Aliases->SetCellValue( ci, 1, m_Aliases->GetCellValue( ni, 1 ) );
        m_Aliases->SetCellValue( ci, 2, m_Aliases->GetCellValue( ni, 2 ) );

        m_Aliases->SetCellValue( ni, 0, al0.m_alias );
        m_Aliases->SetCellValue( ni, 1, al0.m_pathvar );
        m_Aliases->SetCellValue( ni, 2, al0.m_description );
        m_Aliases->SelectRow( ni, false );
    }

    event.Skip();
}
