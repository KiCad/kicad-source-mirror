/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <kiway.h>
#include <eeschema_id.h>
#include <sch_draw_panel.h>
#include <sch_edit_frame.h>
#include <sim/sim_plot_frame.h>
#include <sch_component.h>
#include <sch_sheet.h>
#include <sch_bitmap.h>
#include <netlist_object.h>
#include <sch_view.h>

void SCH_EDIT_FRAME::OnLeftClick( wxDC* aDC, const wxPoint& aPosition )
{
    SCH_ITEM* item = GetScreen()->GetCurItem();

    if( GetToolId() == ID_NO_TOOL_SELECTED )
    {
        m_canvas->SetAutoPanRequest( false );
        SetRepeatItem( NULL );

        // item_flags != 0 means a current item in edit
        if( item && item->GetEditFlags() )
        {
            switch( item->Type() )
            {
            case SCH_LABEL_T:
            case SCH_GLOBAL_LABEL_T:
            case SCH_HIER_LABEL_T:
            case SCH_TEXT_T:
            case SCH_SHEET_PIN_T:
            case SCH_SHEET_T:
            case SCH_BUS_WIRE_ENTRY_T:
            case SCH_BUS_BUS_ENTRY_T:
            case SCH_JUNCTION_T:
            case SCH_COMPONENT_T:
            case SCH_FIELD_T:
            case SCH_BITMAP_T:
            case SCH_NO_CONNECT_T:
                AddItemToScreen( item );
                GetCanvas()->GetView()->ClearPreview();
                GetCanvas()->GetView()->ClearHiddenFlags();
                return;

            default:
                wxFAIL_MSG( wxT( "SCH_EDIT_FRAME::OnLeftClick error.  Item type <" ) +
                            item->GetClass() + wxT( "> is already being edited." ) );
                item->ClearFlags();
                break;
            }
        }
        else
        {
            item = LocateAndShowItem( aPosition );
        }
    }

    if( !item ) // If clicked on a empty area, clear any highligthed symbol
        GetCanvas()->GetView()->HighlightItem( nullptr, nullptr );

    switch( GetToolId() )
    {
    case ID_SCHEMATIC_DELETE_ITEM_BUTT:
        DeleteItemAtCrossHair();
        break;

#ifdef KICAD_SPICE
    case ID_SIM_PROBE:
        {
            constexpr KICAD_T wiresAndComponents[] = { SCH_LINE_T,
                                                       SCH_COMPONENT_T,
                                                       SCH_SHEET_PIN_T,
                                                       EOT };
            item = LocateAndShowItem( aPosition, wiresAndComponents );

            if( !item )
                break;

            std::unique_ptr<NETLIST_OBJECT_LIST> netlist( BuildNetListBase() );

            for( NETLIST_OBJECT* obj : *netlist )
            {
                if( obj->m_Comp == item )
                {
                    SIM_PLOT_FRAME* simFrame = (SIM_PLOT_FRAME*) Kiway().Player( FRAME_SIMULATOR, false );

                    if( simFrame )
                        simFrame->AddVoltagePlot( obj->GetNetName() );

                    break;
                }
            }
        }
        break;

    case ID_SIM_TUNE:
        {
            constexpr KICAD_T fieldsAndComponents[] = { SCH_COMPONENT_T, SCH_FIELD_T, EOT };
            item = LocateAndShowItem( aPosition, fieldsAndComponents );

            if( !item )
                return;

            if( item->Type() != SCH_COMPONENT_T )
            {
                item = static_cast<SCH_ITEM*>( item->GetParent() );

                if( item->Type() != SCH_COMPONENT_T )
                    return;
            }

            SIM_PLOT_FRAME* simFrame = (SIM_PLOT_FRAME*) Kiway().Player( FRAME_SIMULATOR, false );

            if( simFrame )
                simFrame->AddTuner( static_cast<SCH_COMPONENT*>( item ) );
        }
        break;
#endif /* KICAD_SPICE */

    default:
        break;
    }
}


/**
 * Function OnLeftDClick
 * called on a double click event from the drawpanel mouse handler
 *  if an editable item is found (text, component)
 *      Call the suitable dialog editor.
 *  Id a create command is in progress:
 *      validate and finish the command
 */
void SCH_EDIT_FRAME::OnLeftDClick( wxDC* aDC, const wxPoint& aPosition )

{
    EDA_ITEM* item = GetScreen()->GetCurItem();

    switch( GetToolId() )
    {
    case ID_NO_TOOL_SELECTED:
        if( item == NULL || item->GetEditFlags() == 0 )
            item = LocateAndShowItem( aPosition, SCH_COLLECTOR::DoubleClickItems );

        if( item == NULL || item->GetEditFlags() != 0 )
            break;

        switch( item->Type() )
        {
        case SCH_SHEET_T:
            g_CurrentSheet->push_back( (SCH_SHEET*) item );
            DisplayCurrentSheet();
            break;

        case SCH_COMPONENT_T:
            EditComponent( (SCH_COMPONENT*) item );
            GetCanvas()->MoveCursorToCrossHair();

            if( item->GetEditFlags() == 0 )
                GetScreen()->SetCurItem( nullptr );

            GetCanvas()->Refresh();
            break;

        case SCH_TEXT_T:
        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIER_LABEL_T:
            EditSchematicText( (SCH_TEXT*) item );
            break;

        case SCH_BITMAP_T:
            // The bitmap is cached in Opengl: clear the cache, because
            // the cache data is perhaps invalid
            if( EditImage( (SCH_BITMAP*) item ) )
                GetCanvas()->GetView()->RecacheAllItems();

            break;

        case SCH_FIELD_T:
            EditComponentFieldText( (SCH_FIELD*) item );
            GetCanvas()->MoveCursorToCrossHair();
            break;

        case SCH_MARKER_T:
            ( (SCH_MARKER*) item )->DisplayMarkerInfo( this );
            break;

        default:
            break;
        }

        break;
    }
}
