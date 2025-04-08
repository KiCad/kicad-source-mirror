/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <pgm_base.h>
#include <kiface_base.h>
#include <advanced_config.h>
#include <settings/settings_manager.h>
#include <eeschema_settings.h>
#include <default_values.h>
#include <sch_render_settings.h>


SCH_RENDER_SETTINGS::SCH_RENDER_SETTINGS() :
        m_IsSymbolEditor( false ),
        m_ShowUnit( 0 ),
        m_ShowBodyStyle( 0 ),
        m_ShowPinsElectricalType( true ),
        m_ShowHiddenPins( true ),
        m_ShowHiddenFields( true ),
        m_ShowVisibleFields( true ),
        m_ShowPinNumbers( false ),
        m_ShowPinNames( false ),
        m_ShowPinAltIcons( false ),
        m_ShowDisabled( false ),
        m_ShowGraphicsDisabled( false ),
        m_ShowConnectionPoints( false ),
        m_OverrideItemColors( false ),
        m_LabelSizeRatio( DEFAULT_LABEL_SIZE_RATIO ),
        m_TextOffsetRatio( DEFAULT_TEXT_OFFSET_RATIO ),
        m_PinSymbolSize( DEFAULT_TEXT_SIZE * schIUScale.IU_PER_MILS / 2 ),
        m_SymbolLineWidth( DEFAULT_LINE_WIDTH_MILS * schIUScale.IU_PER_MILS ),
        m_Transform()
{
    SetDefaultPenWidth( DEFAULT_LINE_WIDTH_MILS * schIUScale.IU_PER_MILS );
    SetDashLengthRatio( 12 );       // From ISO 128-2
    SetGapLengthRatio( 3 );         // From ISO 128-2

    m_minPenWidth = KiROUND( ADVANCED_CFG::GetCfg().m_MinPlotPenWidth * schIUScale.IU_PER_MM );
}


void SCH_RENDER_SETTINGS::LoadColors( const COLOR_SETTINGS* aSettings )
{
    for( int layer = SCH_LAYER_ID_START; layer < SCH_LAYER_ID_END; layer ++)
        m_layerColors[ layer ] = aSettings->GetColor( layer );

    for( int layer = GAL_LAYER_ID_START; layer < GAL_LAYER_ID_END; layer ++)
        m_layerColors[ layer ] = aSettings->GetColor( layer );

    m_backgroundColor = aSettings->GetColor( LAYER_SCHEMATIC_BACKGROUND );

    m_layerColors[LAYER_AUX_ITEMS] = m_layerColors[LAYER_SCHEMATIC_AUX_ITEMS];

    m_OverrideItemColors = aSettings->GetOverrideSchItemColors();
}


bool SCH_RENDER_SETTINGS::GetShowPageLimits() const
{
    EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );
    return cfg && cfg->m_Appearance.show_page_limits && !IsPrinting();
}

