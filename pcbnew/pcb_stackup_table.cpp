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

#include <pcb_stackup_table.h>

#include <base_units.h>
#include <board_design_settings.h>
#include <board_design_settings_defaults.h>
#include <pcb_tablecell.h>
#include <properties/property_mgr.h>
#include <api/api_generated_table_utils.h>
#include <api/board/board_types.pb.h>


PCB_STACKUP_TABLE::PCB_STACKUP_TABLE( BOARD_ITEM* aParent ) :
        PCB_GENERATED_TABLE( aParent, PCB_STACKUP_TABLE_T, pcbIUScale.mmToIU( DEFAULT_LINE_WIDTH ) )
{
    m_columns = StackupTableSchema().Defaults();
    m_units = StackupTableSchema().DefaultUnits();
    m_precision = StackupTableSchema().DefaultPrecision();
}


wxString PCB_STACKUP_TABLE::GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const
{
    return wxString::Format( _( "Stackup Table (%d rows)" ), GetRowCount() );
}


bool PCB_STACKUP_TABLE::operator==( const BOARD_ITEM& aOther ) const
{
    if( aOther.Type() != Type() )
        return false;

    return generatedEquals( static_cast<const PCB_STACKUP_TABLE&>( aOther ) );
}


std::unique_ptr<GENERATED_TABLE_CONTENT> PCB_STACKUP_TABLE::generate( const BOARD&             aBoard,
                                                                      GENERATED_TABLE_REFRESH* aRefresh ) const
{
    auto content = std::make_unique<GENERATED_TABLE_CONTENT>();
    content->m_Rows = BuildStackupTableRows( aBoard, m_columns, m_units, m_precision );

    return content;
}


void PCB_STACKUP_TABLE::styleNewCell( PCB_TABLECELL* aCell, bool aIsHeading )
{
    aCell->SetTextSize( VECTOR2I( pcbIUScale.mmToIU( 1.5 ), pcbIUScale.mmToIU( 1.5 ) ) );
    aCell->SetTextThickness( pcbIUScale.mmToIU( aIsHeading ? 0.3 : 0.2 ) );
}


void PCB_STACKUP_TABLE::Serialize( google::protobuf::Any& aContainer ) const
{
    kiapi::board::types::StackupTable table;

    PackGeneratedTable( *this, table );

    aContainer.PackFrom( table );
}


bool PCB_STACKUP_TABLE::Deserialize( const google::protobuf::Any& aContainer )
{
    kiapi::board::types::StackupTable table;

    if( !aContainer.UnpackTo( &table ) )
        return false;

    return UnpackGeneratedTable( table, *this );
}


static struct PCB_STACKUP_TABLE_DESC
{
    PCB_STACKUP_TABLE_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PCB_STACKUP_TABLE );

        propMgr.AddTypeCast( new TYPE_CAST<PCB_STACKUP_TABLE, BOARD_ITEM> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_STACKUP_TABLE, BOARD_ITEM_CONTAINER> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_STACKUP_TABLE, PCB_TABLE> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_STACKUP_TABLE, PCB_GENERATED_TABLE> );
        propMgr.InheritsAfter( TYPE_HASH( PCB_STACKUP_TABLE ), TYPE_HASH( PCB_GENERATED_TABLE ) );
    }
} _PCB_STACKUP_TABLE_DESC;
