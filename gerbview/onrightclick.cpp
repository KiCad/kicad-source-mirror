/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2014 Jean-Pierre Charras  jp.charras at wanadoo.fr
 * Copyright (C) 1992-2014 KiCad Developers, see change_log.txt for contributors.
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
#include <class_drawpanel.h>
#include <id.h>
#include <gerbview_id.h>

#include <gerbview.h>
#include <gerbview_frame.h>
#include <menus_helpers.h>


/* Prepare the right-click pullup menu.
 * The menu already has a list of zoom commands.
 */
bool GERBVIEW_FRAME::OnRightClick( const wxPoint& aPosition, wxMenu* aPopMenu )
{
    GERBER_DRAW_ITEM* currItem = (GERBER_DRAW_ITEM*) GetScreen()->GetCurItem();
    wxString    msg;
    bool        BlockActive = !GetScreen()->m_BlockLocate.IsIdle();
    bool        busy = currItem && currItem->GetFlags();

    // Do not initiate a start block validation on menu.
    m_canvas->SetCanStartBlock( -1 );

    // Simple location of elements where possible.
    if( !busy )
    {
        currItem = Locate( aPosition, CURSEUR_OFF_GRILLE );
        busy = currItem && currItem->GetFlags();
    }

    // If command in progress, end command.
    if( GetToolId() != ID_NO_TOOL_SELECTED )
    {
        if( busy )
            AddMenuItem( aPopMenu, ID_POPUP_CANCEL_CURRENT_COMMAND,
                         _( "Cancel" ), KiBitmap( cancel_xpm )  );
        else
            AddMenuItem( aPopMenu, ID_POPUP_CLOSE_CURRENT_TOOL,
                         _( "End Tool" ), KiBitmap( cursor_xpm ) );

        aPopMenu->AppendSeparator();
    }
    else
    {
        if( busy || BlockActive )
        {
            if( BlockActive )
            {
                AddMenuItem( aPopMenu, ID_POPUP_CANCEL_CURRENT_COMMAND,
                             _( "Cancel Block" ), KiBitmap( cancel_xpm ) );
                aPopMenu->AppendSeparator();
                AddMenuItem( aPopMenu, ID_POPUP_PLACE_BLOCK,
                             _( "Place Block" ), KiBitmap( checked_ok_xpm ) );
            }
            else
            {
                AddMenuItem( aPopMenu, ID_POPUP_CANCEL_CURRENT_COMMAND,
                             _( "Cancel" ), KiBitmap( cancel_xpm ) );
            }

            aPopMenu->AppendSeparator();
        }
    }

    if( BlockActive )
        return true;

    if( currItem )
    {
        GetScreen()->SetCurItem( currItem );
        bool add_separator = false;

        // Now, display a context menu
        // to allow highlighting items which share the same attributes
        // as the selected item (net attributes and aperture attributes)
        const GBR_NETLIST_METADATA& net_attr = currItem->GetNetAttributes();

        if( ( net_attr.m_NetAttribType & GBR_NETLIST_METADATA::GBR_NETINFO_PAD ) ||
            ( net_attr.m_NetAttribType & GBR_NETLIST_METADATA::GBR_NETINFO_CMP ) )
        {
            AddMenuItem( aPopMenu, ID_HIGHLIGHT_CMP_ITEMS,
                         wxString::Format( _( "Highlight items of component '%s'" ),
                                            GetChars( net_attr.m_Cmpref ) ),
                         KiBitmap( file_footprint_xpm ) );
            add_separator = true;
        }

        if( ( net_attr.m_NetAttribType & GBR_NETLIST_METADATA::GBR_NETINFO_NET ) )
        {
            AddMenuItem( aPopMenu, ID_HIGHLIGHT_NET_ITEMS,
                         wxString::Format( _( "Highlight items of net '%s'" ),
                                            GetChars( net_attr.m_Netname ) ),
                         KiBitmap( general_ratsnest_xpm ) );
            add_separator = true;
        }

        D_CODE* apertDescr = currItem->GetDcodeDescr();

        if( !apertDescr->m_AperFunction.IsEmpty() )
        {
            AddMenuItem( aPopMenu, ID_HIGHLIGHT_APER_ATTRIBUTE_ITEMS,
                         wxString::Format( _( "Highlight aperture type '%s'" ),
                                            GetChars( apertDescr->m_AperFunction ) ),
                         KiBitmap( flag_xpm ) );
            add_separator = true;
        }

        if( add_separator )
            aPopMenu->AppendSeparator();
    }

    AddMenuItem( aPopMenu, ID_HIGHLIGHT_REMOVE_ALL,
                 _( "Clear highlight" ),
                 KiBitmap( gerbview_clear_layers_xpm ) );

    aPopMenu->AppendSeparator();

    return true;
}
