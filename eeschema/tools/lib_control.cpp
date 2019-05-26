/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <sch_painter.h>
#include <tool/tool_manager.h>
#include <tools/ee_actions.h>
#include <lib_edit_frame.h>
#include <tools/lib_control.h>


TOOL_ACTION EE_ACTIONS::showElectricalTypes( "eeschema.SymbolLibraryControl.showElectricalTypes",
        AS_GLOBAL, 0,
        _( "Show Pin Electrical Types" ), "",
        pin_show_etype_xpm );


TOOL_ACTION EE_ACTIONS::showComponentTree( "eeschema.SymbolLibraryControl.showComponentTree",
        AS_GLOBAL, 0,
        _( "Show Symbol Tree" ), "",
        search_tree_xpm );


int LIB_CONTROL::Save( const TOOL_EVENT& aEvent )
{
    m_frame->OnSave();
    return 0;
}


int LIB_CONTROL::SaveAs( const TOOL_EVENT& aEvent )
{
    m_frame->OnSaveAs( true );
    return 0;
}


int LIB_CONTROL::SaveAll( const TOOL_EVENT& aEvent )
{
    m_frame->OnSaveAll();
    return 0;
}


int LIB_CONTROL::ShowLibraryBrowser( const TOOL_EVENT& aEvent )
{
    wxCommandEvent dummy;
    m_frame->OnOpenLibraryViewer( dummy );

    return 0;
}


int LIB_CONTROL::ShowComponentTree( const TOOL_EVENT& aEvent )
{
    if( m_isLibEdit )
    {
        LIB_EDIT_FRAME* editFrame = static_cast<LIB_EDIT_FRAME*>( m_frame );

        wxCommandEvent dummy;
        editFrame->OnToggleSearchTree( dummy );
    }

    return 0;
}


int LIB_CONTROL::ShowElectricalTypes( const TOOL_EVENT& aEvent )
{
    m_frame->SetShowElectricalType( !m_frame->GetShowElectricalType() );

    // Update canvas
    m_frame->GetRenderSettings()->m_ShowPinsElectricalType = m_frame->GetShowElectricalType();
    m_frame->GetCanvas()->GetView()->UpdateAllItems( KIGFX::REPAINT );
    m_frame->GetCanvas()->Refresh();

    return 0;
}


void LIB_CONTROL::setTransitions()
{
    Go( &LIB_CONTROL::Save,                  ACTIONS::save.MakeEvent() );
    Go( &LIB_CONTROL::SaveAs,                ACTIONS::saveAs.MakeEvent() );
    Go( &LIB_CONTROL::SaveAll,               ACTIONS::saveAll.MakeEvent() );

    Go( &LIB_CONTROL::ShowLibraryBrowser,    EE_ACTIONS::showLibraryBrowser.MakeEvent() );
    Go( &LIB_CONTROL::ShowElectricalTypes,   EE_ACTIONS::showElectricalTypes.MakeEvent() );
    Go( &LIB_CONTROL::ShowComponentTree,     EE_ACTIONS::showComponentTree.MakeEvent() );
}
