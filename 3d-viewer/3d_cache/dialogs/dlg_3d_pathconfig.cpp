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
#include <pgm_base.h>
#include <html_messagebox.h>
#include "3d_cache/dialogs/dlg_3d_pathconfig.h"
#include "3d_cache/3d_filename_resolver.h"

DLG_3D_PATH_CONFIG::DLG_3D_PATH_CONFIG( wxWindow* aParent, S3D_FILENAME_RESOLVER* aResolver ) :
    DLG_3D_PATH_CONFIG_BASE( aParent ), m_resolver( aResolver )
{
    initDialog();

    GetSizer()->SetSizeHints( this );
	Centre();

    return;
}


void DLG_3D_PATH_CONFIG::initDialog()
{
    m_Aliases->EnableEditing( true );

    // Gives a min width to each column, when the user drags a column
    m_Aliases->SetColMinimalWidth( 0, 80 );
    m_Aliases->SetColMinimalWidth( 1, 300 );
    m_Aliases->SetColMinimalWidth( 2, 120 );
    m_Aliases->SetColMinimalAcceptableWidth( 80 );

    // Set column sizes to this min value
	m_Aliases->SetColSize( 0, 80 );
	m_Aliases->SetColSize( 1, 300 );
	m_Aliases->SetColSize( 2, 120 );

    m_EnvVars->SetColMinimalWidth( 0, 80 );
    m_EnvVars->SetColMinimalWidth( 1, 300 );
    m_EnvVars->SetColMinimalAcceptableWidth( 80 );
    m_EnvVars->SetColSize( 0, 80 );
    m_EnvVars->SetColSize( 1, 300 );

    if( m_resolver )
    {
        updateEnvVars();

        // prohibit these characters in the alias names: []{}()%~<>"='`;:.,&?/\|$
        m_aliasValidator.SetStyle( wxFILTER_EXCLUDE_CHAR_LIST );
        m_aliasValidator.SetCharExcludes( wxT( "{}[]()%~<>\"='`;:.,&?/\\|$" ) );

        const std::list< S3D_ALIAS >* rpaths = m_resolver->GetPaths();
        std::list< S3D_ALIAS >::const_iterator rI = rpaths->begin();
        std::list< S3D_ALIAS >::const_iterator rE = rpaths->end();
        size_t listsize = rpaths->size();
        size_t listidx = 0;

        while( rI != rE && ( (*rI).m_alias.StartsWith( "${" )
            || (*rI).m_alias.StartsWith( "$(" ) ) )
        {
            ++listidx;
            ++rI;
        }

        if( listidx < listsize )
            m_curdir = (*rI).m_pathexp;
        else
            return;

        listsize = listsize - listidx - m_Aliases->GetNumberRows();

        // note: if the list allocation fails we have bigger problems
        // and there is no point in trying to notify the user here
        if( listsize > 0 && !m_Aliases->InsertRows( 0, listsize ) )
            return;

        int nitems = 0;
        wxGridCellTextEditor* pEdAlias;

        while( rI != rE )
        {
            m_Aliases->SetCellValue( nitems, 0, rI->m_alias );

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

            m_Aliases->SetCellValue( nitems, 1, rI->m_pathvar );
            m_Aliases->SetCellValue( nitems++, 2, rI->m_description );

            // TODO: implement a wxGridCellEditor which invokes a wxDirDialog

            ++rI;
        }

        m_Aliases->AutoSize();
    }
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


void DLG_3D_PATH_CONFIG::OnConfigEnvVar( wxCommandEvent& event )
{
    Pgm().ConfigurePaths( this );
    updateEnvVars();
    event.Skip();
}


void DLG_3D_PATH_CONFIG::updateEnvVars( void )
{
    if( !m_resolver )
        return;

    std::list< wxString > epaths;

    m_resolver->GetKicadPaths( epaths );
    size_t nitems = epaths.size();
    size_t nrows = m_EnvVars->GetNumberRows();
    bool resize = nrows != nitems;  // true after adding/removing env vars

    if( nrows > nitems )
    {
        size_t ni = nrows - nitems;
        m_EnvVars->DeleteRows( 0, ni );
    }
    else if( nrows < nitems )
    {
        size_t ni = nitems - nrows;
        m_EnvVars->InsertRows( 0, ni );
    }

    int j = 0;

    for( auto i : epaths )
    {
        wxString val = ExpandEnvVarSubstitutions( i );
        m_EnvVars->SetCellValue( j, 0, i );
        m_EnvVars->SetCellValue( j, 1, val );
        m_EnvVars->SetReadOnly( j, 0, true );
        m_EnvVars->SetReadOnly( j, 1, true );
        wxGridCellAttr* ap = m_EnvVars->GetOrCreateCellAttr( j, 0 );
        ap->SetReadOnly( true );
        ap->SetBackgroundColour( *wxLIGHT_GREY );
        m_EnvVars->SetRowAttr( j, ap );
        ++j;
    }

    m_EnvVars->AutoSize();

    // Resizing the full dialog is sometimes needed for a clean display
    // i.e. when adding/removing Kicad environment variables
    if( resize )
        GetSizer()->SetSizeHints( this );

    return;
}


void DLG_3D_PATH_CONFIG::OnHelp( wxCommandEvent& event )
{
    wxString msg = _( "Enter the name and path for each 3D alias variable.<br>KiCad "
                      "environment variables and their values are shown for "
                      "reference only and cannot be edited." );
    msg << "<br><br><b>";
    msg << _( "Alias names may not contain any of the characters " );
    msg << "{}[]()%~<>\"='`;:.,&?/\\|$";
    msg << "</b>";

    HTML_MESSAGE_BOX dlg( GetParent(), _( "Environment Variable Help" ) );
    dlg.AddHTML_Text( msg );
    dlg.ShowModal();

    event.Skip();
}
