/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Alex Shvartzkop <dudesuchamazing@gmail.com>
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

#include <pcb_generator.h>
#include <generators_mgr.h>

#include <functional>
#include <optional>
#include <magic_enum.hpp>

#include <wx/debug.h>
#include <wx/log.h>

#include <gal/graphics_abstraction_layer.h>
#include <geometry/shape_circle.h>
#include <geometry/geometry_utils.h>
#include <kiplatform/ui.h>
#include <dialogs/dialog_unit_entry.h>
#include <collectors.h>
#include <scoped_set_reset.h>
#include <core/mirror.h>
#include <string_utils.h>

#include <board.h>
#include <board_design_settings.h>
#include <drc/drc_engine.h>
#include <pcb_track.h>
#include <pcb_shape.h>
#include <pcb_group.h>

#include <tool/edit_points.h>
#include <tool/tool_manager.h>
#include <tools/drawing_tool.h>
#include <tools/generator_tool.h>
#include <tools/pcb_picker_tool.h>
#include <tools/pcb_selection_tool.h>
#include <tools/zone_filler_tool.h>

#include <preview_items/draw_context.h>
#include <preview_items/preview_utils.h>
#include <view/view.h>
#include <view/view_controls.h>

#include <router/pns_dp_meander_placer.h>
#include <router/pns_meander_placer_base.h>
#include <router/pns_meander.h>
#include <router/pns_kicad_iface.h>
#include <router/pns_segment.h>
#include <router/pns_arc.h>
#include <router/pns_solid.h>
#include <router/pns_topology.h>
#include <router/router_preview_item.h>

#include <dialogs/dialog_tuning_pattern_properties.h>

#include <generators/pcb_tuning_pattern.h>
#include <properties/property.h>
#include <properties/property_mgr.h>




static LENGTH_TUNING_MODE tuningFromString( const std::string& aStr )
{
    if( aStr == "single" )
        return LENGTH_TUNING_MODE::SINGLE;
    else if( aStr == "diff_pair" )
        return LENGTH_TUNING_MODE::DIFF_PAIR;
    else if( aStr == "diff_pair_skew" )
        return LENGTH_TUNING_MODE::DIFF_PAIR_SKEW;
    else
    {
        wxFAIL_MSG( wxS( "Unknown length tuning token" ) );
        return LENGTH_TUNING_MODE::SINGLE;
    }
}


static std::string tuningToString( const LENGTH_TUNING_MODE aTuning )
{
    switch( aTuning )
    {
    case LENGTH_TUNING_MODE::SINGLE:         return "single";
    case LENGTH_TUNING_MODE::DIFF_PAIR:      return "diff_pair";
    case LENGTH_TUNING_MODE::DIFF_PAIR_SKEW: return "diff_pair_skew";
    default:            wxFAIL;              return "";
    }
}


static LENGTH_TUNING_MODE fromPNSMode( PNS::ROUTER_MODE aRouterMode )
{
    switch( aRouterMode )
    {
    case PNS::PNS_MODE_TUNE_SINGLE:         return LENGTH_TUNING_MODE::SINGLE;
    case PNS::PNS_MODE_TUNE_DIFF_PAIR:      return LENGTH_TUNING_MODE::DIFF_PAIR;
    case PNS::PNS_MODE_TUNE_DIFF_PAIR_SKEW: return LENGTH_TUNING_MODE::DIFF_PAIR_SKEW;
    default:                                return LENGTH_TUNING_MODE::SINGLE;
    }
}


static PNS::MEANDER_SIDE sideFromString( const std::string& aStr )
{
    if( aStr == "default" )
        return PNS::MEANDER_SIDE_DEFAULT;
    else if( aStr == "left" )
        return PNS::MEANDER_SIDE_LEFT;
    else if( aStr == "right" )
        return PNS::MEANDER_SIDE_RIGHT;
    else
    {
        wxFAIL_MSG( wxS( "Unknown length-tuning side token" ) );
        return PNS::MEANDER_SIDE_DEFAULT;
    }
}


static std::string statusToString( const PNS::MEANDER_PLACER_BASE::TUNING_STATUS aStatus )
{
    switch( aStatus )
    {
    case PNS::MEANDER_PLACER_BASE::TOO_LONG:  return "too_long";
    case PNS::MEANDER_PLACER_BASE::TOO_SHORT: return "too_short";
    case PNS::MEANDER_PLACER_BASE::TUNED:     return "tuned";
    default:           wxFAIL;                return "";
    }
}


static PNS::MEANDER_PLACER_BASE::TUNING_STATUS statusFromString( const std::string& aStr )
{
    if( aStr == "too_long" )
        return PNS::MEANDER_PLACER_BASE::TOO_LONG;
    else if( aStr == "too_short" )
        return PNS::MEANDER_PLACER_BASE::TOO_SHORT;
    else if( aStr == "tuned" )
        return PNS::MEANDER_PLACER_BASE::TUNED;
    else
    {
        wxFAIL_MSG( wxS( "Unknown tuning status token" ) );
        return PNS::MEANDER_PLACER_BASE::TUNED;
    }
}


static std::string sideToString( const PNS::MEANDER_SIDE aValue )
{
    switch( aValue )
    {
    case PNS::MEANDER_SIDE_DEFAULT: return "default";
    case PNS::MEANDER_SIDE_LEFT:    return "left";
    case PNS::MEANDER_SIDE_RIGHT:   return "right";
    default:        wxFAIL;         return "";
    }
}




PCB_TUNING_PATTERN::PCB_TUNING_PATTERN( BOARD_ITEM* aParent, PCB_LAYER_ID aLayer,
                                        LENGTH_TUNING_MODE aMode ) :
        PCB_GENERATOR( aParent, aLayer ),
        m_trackWidth( 0 ),
        m_diffPairGap( 0 ),
        m_tuningMode( aMode ),
        m_tuningLength( 0 ),
        m_tuningStatus( PNS::MEANDER_PLACER_BASE::TUNING_STATUS::TUNED ),
        m_updateSideFromEnd( false )
{
    m_generatorType = GENERATOR_TYPE;
    m_name = DISPLAY_NAME;
    m_end = VECTOR2I( pcbIUScale.mmToIU( 10 ), 0 );
    m_settings.m_initialSide = PNS::MEANDER_SIDE_LEFT;
}

PCB_TUNING_PATTERN* PCB_TUNING_PATTERN::CreateNew( GENERATOR_TOOL* aTool,
                                                   PCB_BASE_EDIT_FRAME* aFrame,
                                                   BOARD_CONNECTED_ITEM* aStartItem,
                                                   LENGTH_TUNING_MODE aMode )
{
    return nullptr;
}


bool PCB_TUNING_PATTERN::baselineValid()
{
    return false;
}


void PCB_TUNING_PATTERN::EditStart( GENERATOR_TOOL* aTool, BOARD* aBoard, BOARD_COMMIT* aCommit )
{
}


static PNS::LINKED_ITEM* pickSegment( PNS::ROUTER* aRouter, const VECTOR2I& aWhere, int aLayer,
                                      VECTOR2I&               aPointOut,
                                      const SHAPE_LINE_CHAIN& aBaseline = SHAPE_LINE_CHAIN() )
{
    return nullptr;
}


static std::optional<PNS::LINE> getPNSLine( const VECTOR2I& aStart, const VECTOR2I& aEnd,
                                            PNS::ROUTER* router, int layer, VECTOR2I& aStartOut,
                                            VECTOR2I& aEndOut )
{
    return std::nullopt;
}


bool PCB_TUNING_PATTERN::initBaseLine( PNS::ROUTER* aRouter, int aPNSLayer, BOARD* aBoard,
                                       VECTOR2I& aStart, VECTOR2I& aEnd, NETINFO_ITEM* aNet,
                                       std::optional<SHAPE_LINE_CHAIN>& aBaseLine )
{
    return true;
}


bool PCB_TUNING_PATTERN::initBaseLines( PNS::ROUTER* aRouter, int aPNSLayer, BOARD* aBoard )
{
    return true;
}


bool PCB_TUNING_PATTERN::removeToBaseline( PNS::ROUTER* aRouter, int aPNSLayer, SHAPE_LINE_CHAIN& aBaseLine )
{
 
    return true;
}


void PCB_TUNING_PATTERN::Remove( GENERATOR_TOOL* aTool, BOARD* aBoard, BOARD_COMMIT* aCommit )
{
    
}


bool PCB_TUNING_PATTERN::recoverBaseline( PNS::ROUTER* aRouter )
{
    return true;
}


bool PCB_TUNING_PATTERN::resetToBaseline( GENERATOR_TOOL* aTool, int aPNSLayer, SHAPE_LINE_CHAIN& aBaseLine,
                                          bool aPrimary )
{
    return true;
}


bool PCB_TUNING_PATTERN::Update( GENERATOR_TOOL* aTool, BOARD* aBoard, BOARD_COMMIT* aCommit )
{
    return true;
}


void PCB_TUNING_PATTERN::EditFinish( GENERATOR_TOOL* aTool, BOARD* aBoard, BOARD_COMMIT* aCommit  )
{
}


void PCB_TUNING_PATTERN::EditCancel( GENERATOR_TOOL* aTool, BOARD* aBoard, BOARD_COMMIT* aCommit )
{
}


bool PCB_TUNING_PATTERN::MakeEditPoints( EDIT_POINTS& aPoints ) const
{
    return true;
}


bool PCB_TUNING_PATTERN::UpdateFromEditPoints( EDIT_POINTS& aEditPoints )
{
 
    return true;
}


bool PCB_TUNING_PATTERN::UpdateEditPoints( EDIT_POINTS& aEditPoints )
{
 
    return true;
}


SHAPE_LINE_CHAIN PCB_TUNING_PATTERN::getOutline() const
{
    return SHAPE_LINE_CHAIN();
}


void PCB_TUNING_PATTERN::ViewDraw( int aLayer, KIGFX::VIEW* aView ) const
{
}


const STRING_ANY_MAP PCB_TUNING_PATTERN::GetProperties() const
{
   
}


void PCB_TUNING_PATTERN::SetProperties( const STRING_ANY_MAP& aProps )
{
    
}


void PCB_TUNING_PATTERN::ShowPropertiesDialog( PCB_BASE_EDIT_FRAME* aEditFrame )
{
}


std::vector<EDA_ITEM*> PCB_TUNING_PATTERN::GetPreviewItems( GENERATOR_TOOL* aTool,
                                                            PCB_BASE_EDIT_FRAME* aFrame,
                                                            bool aStatusItemsOnly )
{
    std::vector<EDA_ITEM*> previewItems;

    return previewItems;
}


void PCB_TUNING_PATTERN::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame,
                                          std::vector<MSG_PANEL_ITEM>& aList )
{
}


const wxString PCB_TUNING_PATTERN::DISPLAY_NAME = _HKI( "Tuning Pattern" );
const wxString PCB_TUNING_PATTERN::GENERATOR_TYPE = wxS( "tuning_pattern" );


using SCOPED_DRAW_MODE = SCOPED_SET_RESET<DRAWING_TOOL::MODE>;


#define HITTEST_THRESHOLD_PIXELS 5


int DRAWING_TOOL::PlaceTuningPattern( const TOOL_EVENT& aEvent )
{
}


static GENERATORS_MGR::REGISTER<PCB_TUNING_PATTERN> registerMe;

// Also register under the 7.99 name
template <typename T>
struct REGISTER_LEGACY_TUNING_PATTERN
{
    REGISTER_LEGACY_TUNING_PATTERN()
    {
        GENERATORS_MGR::Instance().Register( wxS( "meanders" ), T::DISPLAY_NAME,
                                             []()
                                             {
                                                 return new T;
                                             } );
    }
};

static REGISTER_LEGACY_TUNING_PATTERN<PCB_TUNING_PATTERN> registerMeToo;
