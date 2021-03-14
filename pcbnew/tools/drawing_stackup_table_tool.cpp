/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2017 CERN
 * Copyright (C) 2018-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include "pcb_actions.h"
#include <pcb_edit_frame.h>
#include <view/view.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/tool_event_utils.h>
#include <tools/drawing_tool.h>
#include <board_commit.h>
#include <scoped_set_reset.h>
#include <bitmaps.h>
#include <painter.h>
#include <board.h>
#include <fp_shape.h>
#include <pcb_text.h>
#include <kicad_string.h>
#include <wx/utils.h>


using SCOPED_DRAW_MODE = SCOPED_SET_RESET<DRAWING_TOOL::MODE>;

static std::vector<BOARD_ITEM*> initTextTable( std::vector<std::vector<PCB_TEXT*>> aContent,
                                        wxPoint origin, PCB_LAYER_ID aLayer, wxPoint* aTableSize,
                                        bool aDrawFrame = true )
{
    int i;
    int j;

    int nbCols = aContent.size();
    int nbRows = 0;

    for( auto col : aContent )
        nbRows = std::max( nbRows, static_cast<int>( col.size() ) );

    // Limit the number of cells
    nbCols = std::min( nbCols, 99 );
    nbRows = std::min( nbRows, 99 );

    int rowHeight[99];
    int colWidth[99];

    std::vector<BOARD_ITEM*> table;

    // xmargin and ymargin are margins between the text and the the table lines.
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

    int xmargin = Millimeter2iu( 0.75 );
    int ymargin = Millimeter2iu( 0.75 );

    // Init table
    for( i = 0; i < nbRows; i++ )
        rowHeight[i] = 0;

    for( i = 0; i < nbCols; i++ )
        colWidth[i] = 0;

    // First, we determine what the height/Width should be for every cell
    i = 0;

    for( auto col : aContent )
    {
        j = 0;

        if( i >= nbCols )
            break;

        for( auto cell : col )
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
            line->SetStartX( origin.x );
            line->SetStartY( y );
            line->SetEndX( origin.x + width );
            line->SetEndY( y );
            y += rowHeight[i];
            table.push_back( line );
        }

        line = new PCB_SHAPE;
        line->SetLayer( aLayer );
        line->SetStartX( origin.x );
        line->SetStartY( y );
        line->SetEndX( origin.x + width );
        line->SetEndY( y );
        table.push_back( line );
        int x = origin.x;

        for( i = 0; i < nbCols; i++ )
        {
            line = new PCB_SHAPE;
            line->SetLayer( aLayer );
            line->SetStartX( x );
            line->SetStartY( origin.y );
            line->SetEndX( x );
            line->SetEndY( origin.y + height );
            x += colWidth[i];
            table.push_back( line );
        }

        line = new PCB_SHAPE;
        line->SetLayer( aLayer );
        line->SetStartX( x );
        line->SetStartY( origin.y );
        line->SetEndX( x );
        line->SetEndY( origin.y + height );
        table.push_back( line );
    }
    //Now add the text
    i           = 0;
    wxPoint pos = wxPoint( origin.x + xmargin, origin.y + ymargin );

    for( auto col : aContent )
    {
        j = 0;

        if( i >= nbCols )
            break;

        pos.y = origin.y + ymargin;

        for( auto cell : col )
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


std::vector<BOARD_ITEM*> DRAWING_TOOL::DrawSpecificationStackup(
        wxPoint aOrigin, PCB_LAYER_ID aLayer, bool aDrawNow, wxPoint* tableSize )
{
    BOARD_COMMIT               commit( m_frame );
    std::vector<std::vector<PCB_TEXT*>> texts;

    // Style : Header
    PCB_TEXT* headStyle = new PCB_TEXT( static_cast<FOOTPRINT*>( m_frame->GetModel() ) );
    headStyle->SetLayer( Eco1_User );
    headStyle->SetTextSize( wxSize( Millimeter2iu( 1.5 ), Millimeter2iu( 1.5 ) ) );
    headStyle->SetTextThickness( Millimeter2iu( 0.3 ) );
    headStyle->SetItalic( false );
    headStyle->SetTextPos( wxPoint( 0, 0 ) );
    headStyle->SetText( "Layer" );
    headStyle->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
    headStyle->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );

    // Style : data
    PCB_TEXT* dataStyle = new PCB_TEXT( static_cast<FOOTPRINT*>( m_frame->GetModel() ) );
    dataStyle->SetLayer( Eco1_User );
    dataStyle->SetTextSize( wxSize( Millimeter2iu( 1.5 ), Millimeter2iu( 1.5 ) ) );
    dataStyle->SetTextThickness( Millimeter2iu( 0.1 ) );
    dataStyle->SetItalic( false );
    dataStyle->SetTextPos( wxPoint( 0, 0 ) );
    dataStyle->SetText( "Layer" );
    dataStyle->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
    dataStyle->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );

    //Get Layer names
    BOARD_DESIGN_SETTINGS&           dsnSettings = m_frame->GetDesignSettings();
    BOARD_STACKUP&                   stackup     = dsnSettings.GetStackupDescriptor();
    std::vector<BOARD_STACKUP_ITEM*> layers      = stackup.GetList();

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

    if( m_frame->GetUserUnits() == EDA_UNITS::MILLIMETRES )
        t->SetText( _( "Thickness (mm)" ) );

    else if( m_frame->GetUserUnits() == EDA_UNITS::INCHES )
        t->SetText( _( "Thickness (mils)" ) );

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

    int i;

    for( i = 0; i < stackup.GetCount(); i++ )
    {
        t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
        t->SetText( layers.at( i )->GetLayerName() );
        colLayer.push_back( t );

        t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
        t->SetText( layers.at( i )->GetTypeName() );
        colType.push_back( t );

        t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
        t->SetText( layers.at( i )->GetMaterial() );
        colMaterial.push_back( t );

        t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
        t->SetText(
                StringFromValue( m_frame->GetUserUnits(), layers.at( i )->GetThickness(), true ) );
        colThickness.push_back( t );

        t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
        t->SetText( layers.at( i )->GetColor() );
        colColor.push_back( t );

        t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
        t->SetText( StringFromValue( EDA_UNITS::UNSCALED, layers.at( i )->GetEpsilonR(), false ) );
        colEpsilon.push_back( t );

        t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
        t->SetText(
                StringFromValue( EDA_UNITS::UNSCALED, layers.at( i )->GetLossTangent(), false ) );
        colTanD.push_back( t );
    }

    texts.push_back( colLayer );
    texts.push_back( colType );
    texts.push_back( colMaterial );
    texts.push_back( colThickness );
    texts.push_back( colColor );
    texts.push_back( colEpsilon );
    texts.push_back( colTanD );
    std::vector<BOARD_ITEM*> table =
            initTextTable( texts, aOrigin, aLayer, tableSize, true );

    if( aDrawNow )
    {

        for( auto item : table )
            commit.Add( item );

        commit.Push( _( "Insert board stackup table" ) );
    }

    return table;
}

std::vector<BOARD_ITEM*> DRAWING_TOOL::DrawBoardCharacteristics(
        wxPoint aOrigin, PCB_LAYER_ID aLayer, bool aDrawNow, wxPoint* tableSize )
{
    BOARD_COMMIT        commit( m_frame );
    std::vector<BOARD_ITEM*> objects;
    BOARD_DESIGN_SETTINGS&   settings = m_frame->GetBoard()->GetDesignSettings();
    BOARD_STACKUP&           stackup  = settings.GetStackupDescriptor();

    wxPoint cursorPos = aOrigin;

    // Style : Section header
    PCB_TEXT* headStyle = new PCB_TEXT( static_cast<FOOTPRINT*>( m_frame->GetModel() ) );
    headStyle->SetLayer( Eco1_User );
    headStyle->SetTextSize( wxSize( Millimeter2iu( 2.0 ), Millimeter2iu( 2.0 ) ) );
    headStyle->SetTextThickness( Millimeter2iu( 0.4 ) );
    headStyle->SetItalic( false );
    headStyle->SetTextPos( wxPoint( 0, 0 ) );
    headStyle->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
    headStyle->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );

    // Style : Data
    PCB_TEXT* dataStyle = new PCB_TEXT( static_cast<FOOTPRINT*>( m_frame->GetModel() ) );
    dataStyle->SetLayer( Eco1_User );
    dataStyle->SetTextSize( wxSize( Millimeter2iu( 1.5 ), Millimeter2iu( 1.5 ) ) );
    dataStyle->SetTextThickness( Millimeter2iu( 0.2 ) );
    dataStyle->SetItalic( false );
    dataStyle->SetTextPos( wxPoint( 0, 0 ) );
    dataStyle->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
    dataStyle->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );

    PCB_TEXT* t;

    t = static_cast<PCB_TEXT*>( headStyle->Duplicate() );
    t->SetText( _( "BOARD CHARACTERISTICS" ) );
    t->SetPosition( cursorPos );
    objects.push_back( t );

    cursorPos.y = cursorPos.y + t->GetBoundingBox().GetHeight()
                  + From_User_Unit( EDA_UNITS::MILLIMETRES, 1.0 );

    std::vector<std::vector<PCB_TEXT*>> texts;
    std::vector<PCB_TEXT*>              colLabel1;
    std::vector<PCB_TEXT*>              colData1;
    std::vector<PCB_TEXT*>              colbreak;
    std::vector<PCB_TEXT*>              colLabel2;
    std::vector<PCB_TEXT*>              colData2;
    wxString                   text = wxString( "" );

    t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
    t->SetText( _( "Copper Layer Count: " ) );
    colLabel1.push_back( t );

    t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
    t->SetText( StringFromValue( EDA_UNITS::UNSCALED, settings.GetCopperLayerCount(), false ) );
    colData1.push_back( t );

    EDA_RECT size = m_frame->GetBoard()->ComputeBoundingBox( true );
    t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
    t->SetText( _( "Board overall dimensions: " ) );
    colLabel1.push_back( t );

    t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
    t->SetText( wxString::Format( "%s x %s",
                MessageTextFromValue( m_frame->GetUserUnits(), size.GetWidth(), true ),
                MessageTextFromValue( m_frame->GetUserUnits(), size.GetHeight(), true ) ) );
    colData1.push_back( t );

    t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
    t->SetText( _( "Min track/spacing: " ) );
    colLabel1.push_back( t );

    t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
    t->SetText( wxString::Format( "%s / %s",
                MessageTextFromValue( m_frame->GetUserUnits(), settings.m_TrackMinWidth, true ),
                MessageTextFromValue( m_frame->GetUserUnits(), settings.m_MinClearance, true ) ) );
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
    text = MessageTextFromValue( m_frame->GetUserUnits(), settings.GetBoardThickness(), true );

    t->SetText( text );
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
    text            = MessageTextFromValue( m_frame->GetUserUnits(), holeSize, true );
    t->SetText( text );
    colData2.push_back( t );

    t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
    t->SetText( _( "Impedance Control: " ) );
    colLabel2.push_back( t );

    t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
    t->SetText( stackup.m_CastellatedPads ? _( "Yes" ) : _( "No" ) );
    colData2.push_back( t );

    t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
    t->SetText( _( "Plated Board Edge: " ) );
    colLabel2.push_back( t );
    t = static_cast<PCB_TEXT*>( dataStyle->Duplicate() );
    t->SetText( stackup.m_HasDielectricConstrains ? _( "Yes" ) : _( "No" ) );
    colData2.push_back( t );

    texts.push_back( colLabel1 );
    texts.push_back( colData1 );
    texts.push_back( colbreak );
    texts.push_back( colLabel2 );
    texts.push_back( colData2 );
    wxPoint tableSize2 = wxPoint();

    std::vector<BOARD_ITEM*> table = initTextTable( texts, cursorPos, Eco1_User, &tableSize2,
                                                    false );

    for( auto item : table )
        objects.push_back( item );

    if( aDrawNow )
    {
        for( auto item : objects )
            commit.Add( item );

        commit.Push( "Board Characteristics" );
    }

    tableSize->x = tableSize2.x;
    tableSize->y = cursorPos.y + tableSize2.y + From_User_Unit( EDA_UNITS::MILLIMETRES, 2.0 );

    return objects;
}


int DRAWING_TOOL::InteractivePlaceWithPreview( const TOOL_EVENT& aEvent, std::vector<BOARD_ITEM*> aItems,
        std::vector<BOARD_ITEM*> aPreview, LSET* aLayers )
{
    if( m_isFootprintEditor && !m_frame->GetModel() )
        return -1;

    bool cancelled = false;

    BOARD_COMMIT commit( m_frame );

    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
    m_controls->ShowCursor( true );
    // do not capture or auto-pan until we start placing the table

    SCOPED_DRAW_MODE scopedDrawMode( m_mode, MODE::TEXT );

    std::string tool = aEvent.GetCommandStr().get();
    m_frame->PushTool( tool );
    Activate();

    // Prime the pump
    if( aEvent.HasPosition() )
        m_toolMgr->RunAction( ACTIONS::cursorClick );

    // Main loop: keep receiving events
    wxPoint wxCursorPosition = wxPoint();
    wxPoint wxPreviousCursorPosition = wxPoint( 0, 0 );

    view()->ClearPreview();
    view()->InitPreview();

    for( auto item : aPreview )
    {
        item->Move( wxCursorPosition - wxPreviousCursorPosition );
        view()->AddToPreview( item );
    }

    while( TOOL_EVENT* evt = Wait() )
    {
        m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::PENCIL );
        VECTOR2D pos       = m_controls->GetCursorPosition();
        wxCursorPosition.x = pos.x;
        wxCursorPosition.y = pos.y;

        if( evt->IsCancelInteractive() )
        {
            m_frame->PopTool( tool );
            cancelled = true;
            break;
        }

        if( evt->IsMotion() )
        {
            view()->ShowPreview( false );

            for( auto item : aPreview )
            {
                item->Move( wxCursorPosition - wxPreviousCursorPosition );
            }

            view()->ShowPreview( true );

            wxPreviousCursorPosition.x = wxCursorPosition.x;
            wxPreviousCursorPosition.y = wxCursorPosition.y;

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
                m_frame->PopTool( tool );
                cancelled = true;
                break;
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu( selection() );
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            if( aLayers != NULL )
            {
                PCB_LAYER_ID targetLayer = frame()->SelectOneLayer(
                                            PCB_LAYER_ID::PCB_LAYER_ID_COUNT,
                                            *aLayers, wxGetMousePosition() );

                view()->ClearPreview();

                if( targetLayer == PCB_LAYER_ID::UNDEFINED_LAYER )
                {
                    // The user did not pick any layer.
                    m_frame->PopTool( tool );
                    cancelled = true;
                    break;
                }

                for( auto item : aItems )
                {
                    if( item->Type() == PCB_GROUP_T )
                    {
                        static_cast<PCB_GROUP*>( item )->SetLayerRecursive( targetLayer, 200 );
                    }
                    else
                    {
                        item->SetLayer( targetLayer );
                    }
                }
            }

            for( auto item : aItems )
            {
                item->Move( wxCursorPosition );

                if( item->Type() == PCB_GROUP_T )
                {
                    static_cast<PCB_GROUP*>( item )->AddChildrenToCommit( commit );
                }

                commit.Add( item );
            }

            commit.Push( "Placing items" );
            m_frame->PopTool( tool );

            break;
        }

        else
            evt->SetPassEvent();
    }

    view()->ClearPreview();
    frame()->SetMsgPanel( board() );

    if( cancelled )
        return -1;

    return 0;
}

int DRAWING_TOOL::PlaceCharacteristics( const TOOL_EVENT& aEvent )
{
    wxPoint             tableSize = wxPoint();

    LSET layerSet = ( layerSet.AllCuMask() | layerSet.AllTechMask() );
    layerSet      = static_cast<LSET>( layerSet.set( Edge_Cuts ).set( Margin ) );
    layerSet      = static_cast<LSET>( layerSet.reset( F_Fab ).reset( B_Fab ) );

    PCB_LAYER_ID layer      = m_frame->GetActiveLayer();
    PCB_LAYER_ID savedLayer = layer;

    if( ( layerSet & LSET( layer ) ).count() ) // if layer is a forbidden layer
    {
        m_frame->SetActiveLayer( Cmts_User );
        layer = Cmts_User;
    }

    std::vector<BOARD_ITEM*> table     = DrawBoardCharacteristics(
            wxPoint( 0, 0 ), m_frame->GetActiveLayer(), false, &tableSize );
    std::vector<BOARD_ITEM*>* preview = new std::vector<BOARD_ITEM*>;
    std::vector<BOARD_ITEM*>* items   = new std::vector<BOARD_ITEM*>;

    PCB_SHAPE* line1 = new PCB_SHAPE;
    PCB_SHAPE* line2 = new PCB_SHAPE;
    PCB_SHAPE* line3 = new PCB_SHAPE;
    PCB_SHAPE* line4 = new PCB_SHAPE;

    line1->SetStartX( 0 );
    line1->SetStartY( 0 );
    line1->SetEndX( tableSize.x );
    line1->SetEndY( 0 );

    line2->SetStartX( 0 );
    line2->SetStartY( 0 );
    line2->SetEndX( 0 );
    line2->SetEndY( tableSize.y );

    line3->SetStartX( tableSize.x );
    line3->SetStartY( 0 );
    line3->SetEndX( tableSize.x );
    line3->SetEndY( tableSize.y );

    line4->SetStartX( 0 );
    line4->SetStartY( tableSize.y );
    line4->SetEndX( tableSize.x );
    line4->SetEndY( tableSize.y );

    line1->SetLayer( m_frame->GetActiveLayer() );
    line2->SetLayer( m_frame->GetActiveLayer() );
    line3->SetLayer( m_frame->GetActiveLayer() );
    line4->SetLayer( m_frame->GetActiveLayer() );

    preview->push_back( line1 );
    preview->push_back( line2 );
    preview->push_back( line3 );
    preview->push_back( line4 );

    PCB_GROUP* group = new PCB_GROUP( m_board );
    group->SetName("group-boardCharacteristics");

    for( auto item : table )
        group->AddItem( static_cast<BOARD_ITEM*>( item ) );

    items->push_back( static_cast<BOARD_ITEM*>( group ) );

    if( InteractivePlaceWithPreview( aEvent, *items, *preview, &layerSet ) == -1 )
        m_frame->SetActiveLayer( savedLayer );
    else
        m_frame->SetActiveLayer( table.front()->GetLayer() );

    return 0;
}

int DRAWING_TOOL::PlaceStackup( const TOOL_EVENT& aEvent )
{
    wxPoint             tableSize = wxPoint();

    LSET layerSet = ( layerSet.AllCuMask() | layerSet.AllTechMask() );
    layerSet      = static_cast<LSET>( layerSet.set( Edge_Cuts ).set( Margin ) );
    layerSet      = static_cast<LSET>( layerSet.reset( F_Fab ).reset( B_Fab ) );

    PCB_LAYER_ID layer      = m_frame->GetActiveLayer();
    PCB_LAYER_ID savedLayer = layer;

    if( ( layerSet & LSET( layer ) ).count() ) // if layer is a forbidden layer
    {
        m_frame->SetActiveLayer( Cmts_User );
        layer = Cmts_User;
    }

    std::vector<BOARD_ITEM*> table     = DrawSpecificationStackup(
            wxPoint( 0, 0 ), m_frame->GetActiveLayer(), false, &tableSize );
    std::vector<BOARD_ITEM*>* preview = new std::vector<BOARD_ITEM*>;
    std::vector<BOARD_ITEM*>* items   = new std::vector<BOARD_ITEM*>;

    PCB_SHAPE* line1 = new PCB_SHAPE;
    PCB_SHAPE* line2 = new PCB_SHAPE;
    PCB_SHAPE* line3 = new PCB_SHAPE;
    PCB_SHAPE* line4 = new PCB_SHAPE;

    line1->SetStartX( 0 );
    line1->SetStartY( 0 );
    line1->SetEndX( tableSize.x );
    line1->SetEndY( 0 );

    line2->SetStartX( 0 );
    line2->SetStartY( 0 );
    line2->SetEndX( 0 );
    line2->SetEndY( tableSize.y );

    line3->SetStartX( tableSize.x );
    line3->SetStartY( 0 );
    line3->SetEndX( tableSize.x );
    line3->SetEndY( tableSize.y );

    line4->SetStartX( 0 );
    line4->SetStartY( tableSize.y );
    line4->SetEndX( tableSize.x );
    line4->SetEndY( tableSize.y );

    line1->SetLayer( m_frame->GetActiveLayer() );
    line2->SetLayer( m_frame->GetActiveLayer() );
    line3->SetLayer( m_frame->GetActiveLayer() );
    line4->SetLayer( m_frame->GetActiveLayer() );

    preview->push_back( line1 );
    preview->push_back( line2 );
    preview->push_back( line3 );
    preview->push_back( line4 );

    PCB_GROUP* group = new PCB_GROUP( m_board );
    group->SetName("group-boardStackUp");

    for( auto item : table )
        group->AddItem( item );

    items->push_back( static_cast<BOARD_ITEM*>( group ) );

    if( InteractivePlaceWithPreview( aEvent, *items, *preview, &layerSet ) == -1 )
        m_frame->SetActiveLayer( savedLayer );
    else
        m_frame->SetActiveLayer( table.front()->GetLayer() );

    return 0;
}
