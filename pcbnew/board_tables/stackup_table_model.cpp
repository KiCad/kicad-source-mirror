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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <board_tables/stackup_table_model.h>

#include <string>

#include <base_units.h>
#include <board.h>
#include <board_design_settings.h>
#include <board_stackup_manager/board_stackup.h>
#include <board_stackup_manager/stackup_predefined_prms.h>
#include <eda_units.h>
#include <i18n_utility.h>
#include <string_utils.h>

#include <wx/translation.h>


const GENERATED_TABLE_SCHEMA& StackupTableSchema()
{
    // Headings match the one-shot stackup table this generator replaces
    static const GENERATED_TABLE_SCHEMA schema(
            { { (int) STACKUP_TABLE_COLUMN_ID::LAYER_NAME, "layer", _HKI( "Layer Name" ), GENERATED_TABLE_ALIGN::LEFT,
                true, _HKI( "Layer" ) },
              { (int) STACKUP_TABLE_COLUMN_ID::TYPE, "type", _HKI( "Type" ), GENERATED_TABLE_ALIGN::LEFT, true,
                _HKI( "Type" ) },
              { (int) STACKUP_TABLE_COLUMN_ID::MATERIAL, "material", _HKI( "Material" ), GENERATED_TABLE_ALIGN::LEFT,
                true, _HKI( "Material" ) },
              { (int) STACKUP_TABLE_COLUMN_ID::THICKNESS, "thickness", _HKI( "Thickness" ),
                GENERATED_TABLE_ALIGN::RIGHT, true, _HKI( "Thickness" ) },
              { (int) STACKUP_TABLE_COLUMN_ID::COLOR, "color", _HKI( "Color" ), GENERATED_TABLE_ALIGN::LEFT, true,
                _HKI( "Color" ) },
              { (int) STACKUP_TABLE_COLUMN_ID::EPSILON_R, "epsilon_r", _HKI( "Epsilon R" ),
                GENERATED_TABLE_ALIGN::RIGHT, true, _HKI( "Epsilon R" ) },
              { (int) STACKUP_TABLE_COLUMN_ID::LOSS_TANGENT, "loss_tangent", _HKI( "Loss Tangent" ),
                GENERATED_TABLE_ALIGN::RIGHT, true, _HKI( "Loss Tan" ) } },
            GENERATED_TABLE_UNITS::MM, 3 );

    return schema;
}


namespace
{

wxString layerName( const BOARD& aBoard, const BOARD_STACKUP_ITEM& aItem, int aSublayer )
{
    // Dielectric names are built like the gerber job file's, since the stackup's own are not very good
    if( aItem.GetType() == BS_ITEM_TYPE_DIELECTRIC )
    {
        wxString name = _( "Dielectric" );

        if( aItem.GetSublayersCount() < 2 )
            name << wxT( " " ) << aItem.GetDielectricLayerId();
        else
            name << wxString::Format( _( " %d (%d/%d)" ), aItem.GetDielectricLayerId(), aSublayer + 1,
                                      aItem.GetSublayersCount() );

        return name;
    }

    // The stackup keeps the name from when Board Setup last closed, so a later rename would be missed
    if( IsValidLayer( aItem.GetBrdLayerId() ) )
        return aBoard.GetLayerName( aItem.GetBrdLayerId() );

    return aItem.GetLayerName();
}


wxString cellText( const BOARD_STACKUP_ITEM& aItem, int aSublayer, const wxString& aName, int aColumnId,
                   GENERATED_TABLE_UNITS aUnits, int aPrecision )
{
    switch( static_cast<STACKUP_TABLE_COLUMN_ID>( aColumnId ) )
    {
    case STACKUP_TABLE_COLUMN_ID::LAYER_NAME:
        return aName;

    case STACKUP_TABLE_COLUMN_ID::TYPE:
        return InitialCaps( aItem.GetTypeName() );

    case STACKUP_TABLE_COLUMN_ID::MATERIAL:
        return aItem.GetMaterial( aSublayer );

    case STACKUP_TABLE_COLUMN_ID::THICKNESS:
        // Layers like silkscreen, paste, etc. have no defined thickness, but that
        // does not mean that they are specified as exactly 0mm
        if( !aItem.IsThicknessEditable() )
            return NotSpecifiedPrm();

        // The table's own units, so the drawing does not change with the editor's display units
        return FormatGeneratedTableLength( aItem.GetThickness( aSublayer ), aUnits, aPrecision );

    case STACKUP_TABLE_COLUMN_ID::COLOR:
        return aItem.GetColor( aSublayer );

    case STACKUP_TABLE_COLUMN_ID::EPSILON_R:
        return EDA_UNIT_UTILS::UI::StringFromValue( unityScale, EDA_UNITS::UNSCALED, aItem.GetEpsilonR( aSublayer ) );

    case STACKUP_TABLE_COLUMN_ID::LOSS_TANGENT:
        return EDA_UNIT_UTILS::UI::StringFromValue( unityScale, EDA_UNITS::UNSCALED,
                                                    aItem.GetLossTangent( aSublayer ) );

    default:
        return wxEmptyString;
    }
}


std::string canonicalName( PCB_LAYER_ID aLayer )
{
    return std::string( BOARD::GetStandardLayerName( aLayer ).ToUTF8() );
}

} // namespace


std::vector<GENERATED_TABLE_ROW> BuildStackupTableRows( const BOARD& aBoard,
                                                        const std::vector<GENERATED_TABLE_COLUMN>& aColumns,
                                                        GENERATED_TABLE_UNITS aUnits, int aPrecision )
{
    // A copy, so generating the table never rewrites a stale stored stackup behind the user's back
    BOARD_STACKUP stackup = aBoard.GetStackupOrDefault();
    stackup.SynchronizeWithBoard( &aBoard.GetDesignSettings() );

    std::vector<GENERATED_TABLE_ROW> rows;
    PCB_LAYER_ID                     copperAbove = UNDEFINED_LAYER;

    for( const BOARD_STACKUP_ITEM* item : stackup.GetList() )
    {
        if( item->GetType() == BS_ITEM_TYPE_COPPER )
            copperAbove = item->GetBrdLayerId();

        for( int sublayer = 0; sublayer < item->GetSublayersCount(); ++sublayer )
        {
            GENERATED_TABLE_ROW& row = rows.emplace_back();

            // Canonical names, so a rename does not move a row's formatting onto a different layer
            if( item->GetType() == BS_ITEM_TYPE_DIELECTRIC )
                row.m_Key = "D:" + canonicalName( copperAbove ) + ":" + std::to_string( sublayer );
            else
            {
                row.m_Key = "L:" + canonicalName( item->GetBrdLayerId() );
            }

            const wxString name = layerName( aBoard, *item, sublayer );

            row.m_Cells.reserve( aColumns.size() );

            for( const GENERATED_TABLE_COLUMN& column : aColumns )
                row.m_Cells.push_back( cellText( *item, sublayer, name, column.m_Id, aUnits, aPrecision ) );
        }
    }

    return rows;
}
