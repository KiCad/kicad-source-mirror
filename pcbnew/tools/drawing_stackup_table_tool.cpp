/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include "drawing_tool.h"
#include <kiplatform/ui.h>
#include "pcb_actions.h"
#include <pcb_edit_frame.h>
#include <view/view.h>
#include <tool/tool_manager.h>
#include <board_commit.h>
#include <scoped_set_reset.h>
#include <gal/painter.h>
#include <tools/zone_filler_tool.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pcb_shape.h>
#include <pcb_group.h>
#include <pcb_text.h>
#include <view/view_controls.h>
#include <string_utils.h>
#include <wx/utils.h>


using SCOPED_DRAW_MODE = SCOPED_SET_RESET<DRAWING_TOOL::MODE>;


static std::vector<BOARD_ITEM*> initTextTable( std::vector<std::vector<PCB_TEXT*>> aContent,
                                               VECTOR2I origin, PCB_LAYER_ID aLayer,
                                               VECTOR2I* aTableSize, bool aDrawFrame = true )
{
    int i;
    int j;

    int nbCols = aContent.size();
    int nbRows = 0;

    for( const std::vector<PCB_TEXT*>& col : aContent )
        nbRows = std::max( nbRows, static_cast<int>( col.size() ) );

    // Limit the number of cells
    nbCols = std::min( nbCols, 99 );
    nbRows = std::min( nbRows, 99 );

    int rowHeight[99];
    int colWidth[99];

    std::vector<BOARD_ITEM*> table;

    // xmargin and ymargin are margins between the text and the table lines.
    //
    //  +--------------------------------+
    //  |            ^                   |
    //  |            | ymargin           |
    //  |            v                   |
    //  |<------->TEXT_TEXT_TEXT<------->|
    //  | xmargin    ^           xmargin |
    //  |            | ymargin           |
    //  |            v                   |
    //  +--------------------------------+
    //

    int xmargin = pcbIUScale.mmToIU( 0.75 );
    int ymargin = pcbIUScale.mmToIU( 0.75 );

    // Init table
    for( i = 0; i < nbRows; i++ )
        rowHeight[i] = 0;

    for( i = 0; i < nbCols; i++ )
        colWidth[i] = 0;

    // First, we determine what the height/Width should be for every cell
    i = 0;

    for( const std::vector<PCB_TEXT*>& col : aContent )
    {
        j = 0;

        if( i >= nbCols )
            break;

        for( const PCB_TEXT* cell : col )
        {
            if( j >= nbRows )
                break;

            int height   = cell->GetBoundingBox().GetHeight() + 2 * ymargin;
            int width    = cell->GetBoundingBox().GetWidth() + 2 * xmargin;
            rowHeight[j] = rowHeight[j] > height ? rowHeight[j] : height;
            colWidth[i]  = colWidth[i] > width ? colWidth[i] : width;
            j++;
        }

        i++;
    }

    // get table size
    int height = std::accumulate( rowHeight, rowHeight + nbRows, 0 );
    int width  = std::accumulate( colWidth, colWidth + nbCols, 0 );

    aTableSize->x = width;
    aTableSize->y = height;
    // Draw the frame

    if( aDrawFrame )
    {
        int          y = origin.y;
        PCB_SHAPE*   line;

        for( i = 0; i < nbRows; i++ )
        {
            line = new PCB_SHAPE;
            line->SetLayer( aLayer );
            line->SetStart( VECTOR2I( origin.x, y ) );
            line->SetEnd( VECTOR2I( origin.x + width, y ) );
            y += rowHeight[i];
            table.push_back( line );
        }

        line = new PCB_SHAPE;
        line->SetLayer( aLayer );
        line->SetStart( VECTOR2I( origin.x, y ) );
        line->SetEnd( VECTOR2I( origin.x + width, y ) );
        table.push_back( line );
        int x = origin.x;

        for( i = 0; i < nbCols; i++ )
        {
            line = new PCB_SHAPE;
            line->SetLayer( aLayer );
            line->SetStart( VECTOR2I( x, origin.y ) );
            line->SetEnd( VECTOR2I( x, origin.y + height ) );
            x += colWidth[i];
            table.push_back( line );
        }

        line = new PCB_SHAPE;
        line->SetLayer( aLayer );
        line->SetStart( VECTOR2I( x, origin.y ) );
        line->SetEnd( VECTOR2I( x, origin.y + height ) );
        table.push_back( line );
    }

    //Now add the text
    i = 0;
    VECTOR2I pos( origin.x + xmargin, origin.y + ymargin );

    for( std::vector<PCB_TEXT*>& col : aContent )
    {
        j = 0;

        if( i >= nbCols )
            break;

        pos.y = origin.y + ymargin;

        for( PCB_TEXT* cell : col )
        {
            if( j >= nbRows )
                break;

            cell->SetTextPos( pos );
            cell->SetLayer( aLayer );
            pos.y = pos.y + rowHeight[j];
            table.push_back( cell );
            j++;
        }

        pos.x = pos.x + colWidth[i];
        i++;
    }

    return table;
}


std::vector<BOARD_ITEM*> DRAWING_TOOL::DrawSpecificationStackup( const VECTOR2I& aOrigin,
                                                                 PCB_LAYER_ID aLayer,
                                                                 bool aDrawNow,
                                                                 VECTOR2I* tableSize )
{
    BOARD_COMMIT commit( m_frame );
    FOOTPRINT*   footprint = static_cast<FOOTPRINT*>( m_frame->GetModel() );

    std::vector<std::vector<PCB_TEXT*>> texts;

    // Style : Header
    std::unique_ptr<PCB_TEXT> headStyle = std::make_unique<PCB_TEXT>( footprint );
    headStyle->SetLayer( Eco1_User );
    headStyle->SetTextSize( VECTOR2I( pcbIUScale.mmToIU( 1.5 ), pcbIUScale.mmToIU( 1.5 ) ) );
    headStyle->SetTextThickness( pcbIUScale.mmToIU( 0.3 ) );
    headStyle->SetItalic( false );
    headStyle->SetTextPos( VECTOR2I( 0, 0 ) );
    headStyle->SetText( _( "Layer" ) );
    headStyle->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
    headStyle->SetVertJustify( GR_TEXT_V_ALIGN_TOP );

    // Style : data
    std::unique_ptr<PCB_TEXT> dataStyle = std::make_unique<PCB_TEXT>( footprint );
    dataStyle->SetLayer( Eco1_User );
    dataStyle->SetTextSize( VECTOR2I( pcbIUScale.mmToIU( 1.5 ), pcbIUScale.mmToIU( 1.5 ) ) );
    dataStyle->SetTextThickness( pcbIUScale.mmToIU( 0.1 ) );
    dataStyle->SetItalic( false );
    dataStyle->SetTextPos( VECTOR2I( 0, 0 ) );
    dataStyle->SetText( _( "Layer" ) );
    dataStyle->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
    dataStyle->SetVertJustify( GR_TEXT_V_ALIGN_TOP );

    //Get Layer names
    BOARD_DESIGN_SETTINGS& dsnSettings = m_frame->GetDesignSettings();
    BOARD_STACKUP&         stackup = dsnSettings.GetStackupDescriptor();
    stackup.SynchronizeWithBoard( &dsnSettings );

    std::vector<BOARD_STACKUP_ITEM*> layers = stackup.GetList();

    std::vector<PCB_TEXT*> colLayer;
    std::vector<PCB_TEXT*> colType;
    std::vector<PCB_TEXT*> colMaterial;
    std::vector<PCB_TEXT*> colThickness;
    std::vector<PCB_TEXT*> colColor;
    std::vector<PCB_TEXT*> colEpsilon;
    std::vector<PCB_TEXT*> colTanD;
    PCB_TEXT*              t;

    t = static_cast<PCB_TEXT*>( headStyle->Duplicate() );
    t->SetText( _( "Layer Name" ) );
    colLayer.push_back( t );

    t = static_cast<PCB_TEXT*>( headStyle->Duplicate() );
    t->SetText( _( "Type" ) );
    colType.push_back( t );

    t = static_cast<PCB_TEXT*>( headStyle->Duplicate() );
    t->SetText( _( "Material" ) );
    colMaterial.push_back( t );

    t = static_cast<PCB_TEXT*>( headStyle->Duplicate() );

    switch( m_frame->GetUserUnits() )
    {
    case EDA_UNITS::MM:   t->SetText( _( "Thickness (mm)" ) );        break;
    case EDA_UNITS::INCH: t->SetText( _( "Thickness (inches)" ) );    break;
    case EDA_UNITS::MILS: t->SetText( _( "Thickness (mils)" ) );      break;
    default:              wxFAIL_MSG( wxT( "Unhandled unit type" ) );
    }

    colThickness.push_back( t );

    t = static_cast<PCB_TEXT*>( headStyle->Duplicate() );
    t->SetText( _( "Color" ) );
    colColor.push_back( t );

    t = static_cast<PCB_TEXT*>( headStyle->Duplicate() );
    t->SetText( _( "Epsilon R" ) );
    colEpsilon.push_back( t );

    t = static_cast<PCB_TEXT*>( headStyle->Duplicate() );
    t->SetText( _( "Loss Tangent" ) );
    colTanD.push_back( t );

    for( int i = 0; i < stackup.GetCount(); i++ )
    {
        BOARD_STACKUP_ITEM* stackup_item = layers.at( i );

        for( int sublayer_id = 0; sublayer_id < stackup_item->GetSublayersCount(); sublayer_id++ )
        {
            t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );

            // Layer names are empty until we close at least once the board setup dialog.
            // If the user did not open the dialog, then get the names from the board.
            // But dielectric layer names will be missing.
            // In this case, for dielectric, a dummy name will be used
            if( stackup_item->GetLayerName().IsEmpty() )
            {
                wxString ly_name;

                if( IsValidLayer( stackup_item->GetBrdLayerId() ) )
                    ly_name = m_frame->GetBoard()->GetLayerName( stackup_item->GetBrdLayerId() );

                if( ly_name.IsEmpty() && stackup_item->GetType() == BS_ITEM_TYPE_DIELECTRIC )
                    ly_name = _( "Dielectric" );

                t->SetText( ly_name );
            }
            else
            {
                t->SetText( stackup_item->GetLayerName() );
            }

            colLayer.push_back( t );

            t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
            t->SetText( stackup_item->GetTypeName() );
            colType.push_back( t );

            t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
            t->SetText( stackup_item->GetMaterial( sublayer_id ) );
            colMaterial.push_back( t );

            t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
            t->SetText( m_frame->StringFromValue( stackup_item->GetThickness( sublayer_id ), true ) );
            colThickness.push_back( t );

            t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
            t->SetText( stackup_item->GetColor( sublayer_id ) );
            colColor.push_back( t );

            t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
            t->SetText( EDA_UNIT_UTILS::UI::StringFromValue( unityScale, EDA_UNITS::UNSCALED,
                                                             stackup_item->GetEpsilonR( sublayer_id ), false ) );
            colEpsilon.push_back( t );

            t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
            t->SetText( EDA_UNIT_UTILS::UI::StringFromValue( unityScale, EDA_UNITS::UNSCALED,
                                                             stackup_item->GetLossTangent( sublayer_id ), false ) );
            colTanD.push_back( t );
        }
    }

    texts.push_back( colLayer );
    texts.push_back( colType );
    texts.push_back( colMaterial );
    texts.push_back( colThickness );
    texts.push_back( colColor );
    texts.push_back( colEpsilon );
    texts.push_back( colTanD );
    std::vector<BOARD_ITEM*> table = initTextTable( texts, aOrigin, aLayer, tableSize, true );

    if( aDrawNow )
    {
        for( BOARD_ITEM* item : table )
            commit.Add( item );

        commit.Push( _( "Insert Board Stackup Table" ) );
    }

    return table;
}


std::vector<BOARD_ITEM*> DRAWING_TOOL::DrawBoardCharacteristics( const VECTOR2I& aOrigin,
                                                                 PCB_LAYER_ID aLayer,
                                                                 bool aDrawNow,
                                                                 VECTOR2I* tableSize )
{
    BOARD_COMMIT             commit( m_frame );
    std::vector<BOARD_ITEM*> objects;
    BOARD_DESIGN_SETTINGS&   settings = m_frame->GetBoard()->GetDesignSettings();
    BOARD_STACKUP&           stackup  = settings.GetStackupDescriptor();

    VECTOR2I cursorPos = aOrigin;

    // Style : Section header
    std::unique_ptr<PCB_TEXT> headStyle =
            std::make_unique<PCB_TEXT>( static_cast<FOOTPRINT*>( m_frame->GetModel() ) );
    headStyle->SetLayer( Eco1_User );
    headStyle->SetTextSize( VECTOR2I( pcbIUScale.mmToIU( 2.0 ), pcbIUScale.mmToIU( 2.0 ) ) );
    headStyle->SetTextThickness( pcbIUScale.mmToIU( 0.4 ) );
    headStyle->SetItalic( false );
    headStyle->SetTextPos( VECTOR2I( 0, 0 ) );
    headStyle->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
    headStyle->SetVertJustify( GR_TEXT_V_ALIGN_TOP );

    // Style : Data
    std::unique_ptr<PCB_TEXT> dataStyle =
            std::make_unique<PCB_TEXT>( static_cast<FOOTPRINT*>( m_frame->GetModel() ) );
    dataStyle->SetLayer( Eco1_User );
    dataStyle->SetTextSize( VECTOR2I( pcbIUScale.mmToIU( 1.5 ), pcbIUScale.mmToIU( 1.5 ) ) );
    dataStyle->SetTextThickness( pcbIUScale.mmToIU( 0.2 ) );
    dataStyle->SetItalic( false );
    dataStyle->SetTextPos( VECTOR2I( 0, 0 ) );
    dataStyle->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
    dataStyle->SetVertJustify( GR_TEXT_V_ALIGN_TOP );

    PCB_TEXT* t;

    t = static_cast<PCB_TEXT*>( headStyle->Duplicate() );
    t->SetText( _( "BOARD CHARACTERISTICS" ) );
    t->SetPosition( cursorPos );
    objects.push_back( t );

    cursorPos.y += t->GetBoundingBox().GetHeight()
                      + EDA_UNIT_UTILS::UI::FromUserUnit( pcbIUScale, EDA_UNITS::MM, 1.0 );

    std::vector<std::vector<PCB_TEXT*>> texts;
    std::vector<PCB_TEXT*>              colLabel1;
    std::vector<PCB_TEXT*>              colData1;
    std::vector<PCB_TEXT*>              colbreak;
    std::vector<PCB_TEXT*>              colLabel2;
    std::vector<PCB_TEXT*>              colData2;

    t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
    t->SetText( _( "Copper Layer Count: " ) );
    colLabel1.push_back( t );

    t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
    t->SetText( EDA_UNIT_UTILS::UI::StringFromValue( unityScale, EDA_UNITS::UNSCALED,
                                                     settings.GetCopperLayerCount(), false ) );
    colData1.push_back( t );

    SHAPE_POLY_SET outline;
    m_frame->GetBoard()->GetBoardPolygonOutlines( outline );
    BOX2I size = outline.BBox();
    t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
    t->SetText( _( "Board overall dimensions: " ) );
    colLabel1.push_back( t );

    t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
    t->SetText( wxString::Format( wxT( "%s x %s" ),
                                  m_frame->MessageTextFromValue( size.GetWidth(), true ),
                                  m_frame->MessageTextFromValue( size.GetHeight(), true ) ) );
    colData1.push_back( t );

    t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
    t->SetText( _( "Min track/spacing: " ) );
    colLabel1.push_back( t );

    t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
    t->SetText( wxString::Format( wxT( "%s / %s" ),
                                  m_frame->MessageTextFromValue( settings.m_TrackMinWidth, true ),
                                  m_frame->MessageTextFromValue( settings.m_MinClearance, true ) ) );
    colData1.push_back( t );

    t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
    t->SetText( _( "Copper Finish: " ) );
    colLabel1.push_back( t );

    t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
    t->SetText( stackup.m_FinishType );
    colData1.push_back( t );

    t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
    t->SetText( _( "Castellated pads: " ) );
    colLabel1.push_back( t );

    t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
    t->SetText( stackup.m_CastellatedPads ? _( "Yes" ) : _( "No" ) );
    colData1.push_back( t );

    t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
    t->SetText( _( "Board Thickness: " ) );
    colLabel2.push_back( t );

    t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
    t->SetText( m_frame->MessageTextFromValue( settings.GetBoardThickness(), true ) );
    colData2.push_back( t );

    // some empty cells
    t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
    colLabel2.push_back( t );
    t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
    colData2.push_back( t );

    t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
    t->SetText( _( "Min hole diameter: " ) );
    colLabel2.push_back( t );
    t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );

    double holeSize = std::min( settings.m_MinThroughDrill, settings.m_ViasMinSize );
    t->SetText( m_frame->MessageTextFromValue( holeSize, true ) );
    colData2.push_back( t );

    t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
    t->SetText( _( "Impedance Control: " ) );
    colLabel2.push_back( t );

    t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
    t->SetText( stackup.m_HasDielectricConstrains ? _( "Yes" ) : _( "No" ) );
    colData2.push_back( t );

    t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
    t->SetText( _( "Plated Board Edge: " ) );
    colLabel2.push_back( t );

    t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
    t->SetText( stackup.m_EdgePlating ? _( "Yes" ) : _( "No" ) );
    colData2.push_back( t );

    t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
    t->SetText( _( "Edge card connectors: " ) );
    colLabel1.push_back( t );

    t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
    switch( stackup.m_EdgeConnectorConstraints )
    {
    case BS_EDGE_CONNECTOR_NONE: t->SetText( _( "No" ) ); break;
    case BS_EDGE_CONNECTOR_IN_USE: t->SetText( _( "Yes" ) ); break;
    case BS_EDGE_CONNECTOR_BEVELLED: t->SetText( _( "Yes, Bevelled" ) ); break;
    }
    colData1.push_back( t );

    texts.push_back( colLabel1 );
    texts.push_back( colData1 );
    texts.push_back( colbreak );
    texts.push_back( colLabel2 );
    texts.push_back( colData2 );
    VECTOR2I tableSize2;

    std::vector<BOARD_ITEM*> table = initTextTable( texts, cursorPos, Eco1_User, &tableSize2,
                                                    false );

    for( BOARD_ITEM* item : table )
        objects.push_back( item );

    if( aDrawNow )
    {
        for( BOARD_ITEM* item : objects )
            commit.Add( item );

        commit.Push( _( "Board Characteristics" ) );
    }

    tableSize->x = tableSize2.x;
    tableSize->y = cursorPos.y + tableSize2.y
                       + EDA_UNIT_UTILS::UI::FromUserUnit( pcbIUScale, EDA_UNITS::MM, 2.0 );

    return objects;
}


int DRAWING_TOOL::InteractivePlaceWithPreview( const TOOL_EVENT& aEvent,
                                               std::vector<BOARD_ITEM*>& aItems,
                                               std::vector<BOARD_ITEM*>& aPreview,
                                               LSET* aLayers )
{
    if( m_isFootprintEditor && !m_frame->GetModel() )
        return -1;

    bool cancelled = false;

    BOARD_COMMIT commit( m_frame );

    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear );

    // do not capture or auto-pan until we start placing the table
    SCOPED_DRAW_MODE scopedDrawMode( m_mode, MODE::TEXT );

    m_frame->PushTool( aEvent );

    Activate();
    // Must be done after Activate() so that it gets set into the correct context
    m_controls->ShowCursor( true );

    if( aEvent.HasPosition() )
        m_toolMgr->PrimeTool( aEvent.Position() );

    // Main loop: keep receiving events
    VECTOR2I cursorPosition;
    VECTOR2I previousCursorPosition;

    view()->ClearPreview();
    view()->InitPreview();

    for( BOARD_ITEM* item : aPreview )
    {
        item->Move( cursorPosition - previousCursorPosition );
        view()->AddToPreview( item );
    }

    while( TOOL_EVENT* evt = Wait() )
    {
        m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::PENCIL );
        cursorPosition = m_controls->GetCursorPosition();

        if( evt->IsCancelInteractive() )
        {
            m_frame->PopTool( aEvent );
            cancelled = true;
            break;
        }
        else if( evt->IsMotion() )
        {
            view()->ShowPreview( false );

            for( BOARD_ITEM* item : aPreview )
                item->Move( cursorPosition - previousCursorPosition );

            view()->ShowPreview( true );

            previousCursorPosition = cursorPosition;
        }
        else if( evt->IsActivate() )
        {
            if( evt->IsMoveTool() )
            {
                // leave ourselves on the stack so we come back after the move
                cancelled = true;
                break;
            }
            else
            {
                m_frame->PopTool( aEvent );
                cancelled = true;
                break;
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu->ShowContextMenu( selection() );
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            if( aLayers != nullptr )
            {
                PCB_LAYER_ID destLayer =
                        frame()->SelectOneLayer( PCB_LAYER_ID::PCB_LAYER_ID_COUNT, *aLayers,
                                                 KIPLATFORM::UI::GetMousePosition() );

                view()->ClearPreview();

                if( destLayer == PCB_LAYER_ID::UNDEFINED_LAYER )
                {
                    // The user did not pick any layer.
                    m_frame->PopTool( aEvent );
                    cancelled = true;
                    break;
                }

                for( BOARD_ITEM* item : aItems )
                {
                    item->SetLayer( destLayer );

                    item->RunOnDescendants(
                            [&]( BOARD_ITEM* descendant )
                            {
                                descendant->SetLayer( destLayer );
                            } );
                }
            }

            for( BOARD_ITEM* item : aItems )
            {
                item->Move( cursorPosition );
                commit.Add( item );

                item->RunOnDescendants(
                        [&]( BOARD_ITEM* descendant )
                        {
                            commit.Add( descendant );
                        } );
            }

            commit.Push( _( "Place Items" ) );
            m_frame->PopTool( aEvent );

            break;
        }
        // TODO: It'd be nice to be able to say "don't allow any non-trivial editing actions",
        // but we don't at present have that, so we just knock out some of the egregious ones.
        else if( ZONE_FILLER_TOOL::IsZoneFillAction( evt ) )
        {
            wxBell();
        }
        else
        {
            evt->SetPassEvent();
        }
    }

    view()->ClearPreview();
    frame()->SetMsgPanel( board() );

    if( cancelled )
        return -1;

    return 0;
}


int DRAWING_TOOL::PlaceCharacteristics( const TOOL_EVENT& aEvent )
{
    VECTOR2I tableSize;

    LSET layerSet = ( layerSet.AllCuMask() | layerSet.AllTechMask() );
    layerSet = layerSet.set( Edge_Cuts ).set( Margin );
    layerSet = layerSet.reset( F_Fab ).reset( B_Fab );

    PCB_LAYER_ID layer = m_frame->GetActiveLayer();

    if( ( layerSet & LSET( { layer } ) ).count() ) // if layer is a forbidden layer
        m_frame->SetActiveLayer( Cmts_User );

    std::vector<BOARD_ITEM*> table = DrawBoardCharacteristics( { 0, 0 }, m_frame->GetActiveLayer(),
                                                               false, &tableSize );
    std::vector<BOARD_ITEM*> preview;
    std::vector<BOARD_ITEM*> items;

    PCB_SHAPE* line1 = new PCB_SHAPE;
    PCB_SHAPE* line2 = new PCB_SHAPE;
    PCB_SHAPE* line3 = new PCB_SHAPE;
    PCB_SHAPE* line4 = new PCB_SHAPE;

    line1->SetStart( VECTOR2I( 0, 0 ) );
    line1->SetEnd( VECTOR2I( tableSize.x, 0 ) );

    line2->SetStart( VECTOR2I( 0, 0 ) );
    line2->SetEnd( VECTOR2I( 0, tableSize.y ) );

    line3->SetStart( VECTOR2I( tableSize.x, 0 ) );
    line3->SetEnd( tableSize );

    line4->SetStart( VECTOR2I( 0, tableSize.y ) );
    line4->SetEnd( tableSize );

    line1->SetLayer( m_frame->GetActiveLayer() );
    line2->SetLayer( m_frame->GetActiveLayer() );
    line3->SetLayer( m_frame->GetActiveLayer() );
    line4->SetLayer( m_frame->GetActiveLayer() );

    preview.push_back( line1 );
    preview.push_back( line2 );
    preview.push_back( line3 );
    preview.push_back( line4 );

    PCB_GROUP* group = new PCB_GROUP( m_board );
    group->SetName("group-boardCharacteristics");

    for( auto item : table )
        group->AddItem( static_cast<BOARD_ITEM*>( item ) );

    items.push_back( static_cast<BOARD_ITEM*>( group ) );

    if( InteractivePlaceWithPreview( aEvent, items, preview, &layerSet ) == -1 )
        m_frame->SetActiveLayer( layer );
    else
        m_frame->SetActiveLayer( table.front()->GetLayer() );

    return 0;
}


int DRAWING_TOOL::PlaceStackup( const TOOL_EVENT& aEvent )
{
    VECTOR2I tableSize;

    LSET layerSet = ( layerSet.AllCuMask() | layerSet.AllTechMask() );
    layerSet = layerSet.set( Edge_Cuts ).set( Margin );
    layerSet = layerSet.reset( F_Fab ).reset( B_Fab );

    PCB_LAYER_ID layer      = m_frame->GetActiveLayer();
    PCB_LAYER_ID savedLayer = layer;

    if( ( layerSet & LSET( { layer } ) ).count() ) // if layer is a forbidden layer
    {
        m_frame->SetActiveLayer( Cmts_User );
        layer = Cmts_User;
    }

    std::vector<BOARD_ITEM*> table = DrawSpecificationStackup( VECTOR2I( 0, 0 ),
                                                               m_frame->GetActiveLayer(), false,
                                                               &tableSize );
    std::vector<BOARD_ITEM*> preview;
    std::vector<BOARD_ITEM*> items;

    PCB_SHAPE* line1 = new PCB_SHAPE;
    PCB_SHAPE* line2 = new PCB_SHAPE;
    PCB_SHAPE* line3 = new PCB_SHAPE;
    PCB_SHAPE* line4 = new PCB_SHAPE;

    line1->SetStart( VECTOR2I( 0, 0 ) );
    line1->SetEnd( VECTOR2I( tableSize.x, 0 ) );

    line2->SetStart( VECTOR2I( 0, 0 ) );
    line2->SetEnd( VECTOR2I( 0, tableSize.y ) );

    line3->SetStart( VECTOR2I( tableSize.x, 0 ) );
    line3->SetEnd( tableSize );

    line4->SetStart( VECTOR2I( 0, tableSize.y ) );
    line4->SetEnd( tableSize );

    line1->SetLayer( m_frame->GetActiveLayer() );
    line2->SetLayer( m_frame->GetActiveLayer() );
    line3->SetLayer( m_frame->GetActiveLayer() );
    line4->SetLayer( m_frame->GetActiveLayer() );

    preview.push_back( line1 );
    preview.push_back( line2 );
    preview.push_back( line3 );
    preview.push_back( line4 );

    PCB_GROUP* group = new PCB_GROUP( m_board );
    group->SetName( "group-boardStackUp" );

    for( BOARD_ITEM* item : table )
        group->AddItem( item );

    items.push_back( static_cast<BOARD_ITEM*>( group ) );

    if( InteractivePlaceWithPreview( aEvent, items, preview, &layerSet ) == -1 )
        m_frame->SetActiveLayer( savedLayer );
    else
        m_frame->SetActiveLayer( table.front()->GetLayer() );

    return 0;
}
