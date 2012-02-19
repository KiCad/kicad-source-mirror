/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 KiCad Developers, see change_log.txt for contributors.
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

#include <sch_base_frame.h>
#include <viewlib_frame.h>
#include <libeditframe.h>

SCH_BASE_FRAME::SCH_BASE_FRAME( wxWindow* aParent,
                                id_drawframe aWindowType,
                                const wxString& aTitle,
                                const wxPoint& aPosition, const wxSize& aSize,
                                long aStyle ) :
    EDA_DRAW_FRAME( aParent, aWindowType, aTitle, aPosition, aSize, aStyle )
{
    m_ViewlibFrame = NULL;
    m_LibeditFrame = NULL;
}


void SCH_BASE_FRAME::OnOpenLibraryViewer( wxCommandEvent& event )
{
    if( m_ViewlibFrame )
    {
        m_ViewlibFrame->Show( true );
        return;
    }

    if( m_LibeditFrame && m_LibeditFrame->m_ViewlibFrame )
    {
        m_LibeditFrame->m_ViewlibFrame->Show( true );
        return;
    }

    m_ViewlibFrame = new LIB_VIEW_FRAME( this );
}


SCH_SCREEN* SCH_BASE_FRAME::GetScreen() const
{
    return (SCH_SCREEN*) EDA_DRAW_FRAME::GetScreen();
}


void SCH_BASE_FRAME::SetPageSettings( const PAGE_INFO& aPageSettings )
{
    GetScreen()->SetPageSettings( aPageSettings );
}


const PAGE_INFO& SCH_BASE_FRAME::GetPageSettings () const
{
    return GetScreen()->GetPageSettings();
}


const wxSize SCH_BASE_FRAME::GetPageSizeIU() const
{
    // GetSizeIU is compile time dependent:
    return GetScreen()->GetPageSettings().GetSizeIU();
}


const wxPoint& SCH_BASE_FRAME::GetOriginAxisPosition() const
{
    wxASSERT( GetScreen() );
    return GetScreen()->GetOriginAxisPosition();
}


void SCH_BASE_FRAME::SetOriginAxisPosition( const wxPoint& aPosition )
{
    wxASSERT( GetScreen() );
    GetScreen()->SetOriginAxisPosition( aPosition );
}


const TITLE_BLOCK& SCH_BASE_FRAME::GetTitleBlock() const
{
    wxASSERT( GetScreen() );
    return GetScreen()->GetTitleBlock();
}


void SCH_BASE_FRAME::SetTitleBlock( const TITLE_BLOCK& aTitleBlock )
{
    wxASSERT( GetScreen() );
    GetScreen()->SetTitleBlock( aTitleBlock );
}
