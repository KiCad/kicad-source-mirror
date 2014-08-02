/**
 * @file dialog_show_page_borders.cpp
 * Dialog to show/hide frame reference and select paper size for printing
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 jean-pierre.charras@gipsa-lab.inpg.fr
 * Copyright (C) 2007 KiCad Developers, see change_log.txt for contributors.
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
#include <common.h>
#include <macros.h>

#include <gerbview.h>
#include <gerbview_frame.h>
#include <dialog_show_page_borders.h>


DIALOG_PAGE_SHOW_PAGE_BORDERS::DIALOG_PAGE_SHOW_PAGE_BORDERS( GERBVIEW_FRAME *parent) :
    DIALOG_PAGE_SHOW_PAGE_BORDERS_BASE( parent, wxID_ANY )
{
    m_Parent = parent;
    SetFocus();

    m_ShowPageLimits->SetSelection(0);

    if( m_Parent->GetShowBorderAndTitleBlock() )
    {
        wxString curPaperType = m_Parent->GetPageSettings().GetType();

        for( unsigned i = 1;  i<DIM( g_GerberPageSizeList );  ++i )
        {
            if( curPaperType == g_GerberPageSizeList[i] )
            {
                m_ShowPageLimits->SetSelection( i );
                break;
            }
        }
    }

    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
    Center();
    m_sdbSizer1OK->SetDefault();
}


void DIALOG_PAGE_SHOW_PAGE_BORDERS::OnCancelButtonClick( wxCommandEvent& event )
{
    EndModal( wxID_CANCEL );
}


void DIALOG_PAGE_SHOW_PAGE_BORDERS::OnOKBUttonClick( wxCommandEvent& event )
{
    int idx = m_ShowPageLimits->GetSelection();

    m_Parent->SetShowBorderAndTitleBlock( idx > 0 ? true : false );

    m_Parent->SetPageSettings( PAGE_INFO( g_GerberPageSizeList[idx] ) );

    EndModal( wxID_OK );
}

