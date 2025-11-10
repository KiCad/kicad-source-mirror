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


#include <board.h>
#include <board_design_settings.h>
#include <board_item.h>
#include <pcb_table.h>
#include <pcb_tablecell.h>
#include <pcb_edit_frame.h>
#include <units_provider.h>
#include <board_stackup_manager/stackup_predefined_prms.h>
#include <string_utils.h>

PCB_TABLE* Build_Board_Stackup_Table( BOARD* aBoard, EDA_UNITS aDisplayUnits )
{
    BOARD_DESIGN_SETTINGS& settings = aBoard->GetDesignSettings();
    BOARD_STACKUP&         stackup  = settings.GetStackupDescriptor();
    UNITS_PROVIDER         units_provider( pcbIUScale, aDisplayUnits );

    stackup.SynchronizeWithBoard( &settings );

    std::vector<BOARD_STACKUP_ITEM*> layers = stackup.GetList();

    PCB_TABLE* table = new PCB_TABLE( aBoard, pcbIUScale.mmToIU( DEFAULT_LINE_WIDTH ) );
    table->SetColCount( 7 );

    const auto addHeaderCell =
            [&]( const wxString& text )
            {
                PCB_TABLECELL* c = new PCB_TABLECELL( table );
                c->SetTextSize( VECTOR2I( pcbIUScale.mmToIU( 1.5 ), pcbIUScale.mmToIU( 1.5 ) ) );
                c->SetTextThickness( pcbIUScale.mmToIU( 0.3 ) );
                c->SetText( text );
                table->AddCell( c );
            };

    const auto addDataCell =
            [&]( const wxString& text, const char align = 'L' )
            {
                PCB_TABLECELL* c = new PCB_TABLECELL( table );
                c->SetTextSize( VECTOR2I( pcbIUScale.mmToIU( 1.5 ), pcbIUScale.mmToIU( 1.5 ) ) );
                c->SetTextThickness( pcbIUScale.mmToIU( 0.2 ) );

                if( align == 'R' )
                    c->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );

                c->SetText( text );
                table->AddCell( c );
            };

    const auto layerThicknessString =
            [&]( const BOARD_STACKUP_ITEM& aStackupItem, int aSublayerId )
            {
                const int layerThickness = aStackupItem.GetThickness( aSublayerId );

                // Layers like silkscreen, paste, etc. have no defined thickness, but that
                // does not mean that they are specified as exactly 0mm
                if( !aStackupItem.IsThicknessEditable() )
                    return NotSpecifiedPrm();

                return units_provider.StringFromValue( layerThickness, true );
            };

    addHeaderCell( _( "Layer Name" ) );
    addHeaderCell( _( "Type" ) );
    addHeaderCell( _( "Material" ) );
    addHeaderCell( _( "Thickness" ) );
    addHeaderCell( _( "Color" ) );
    addHeaderCell( _( "Epsilon R" ) );
    addHeaderCell( _( "Loss Tangent" ) );

    for( int i = 0; i < stackup.GetCount(); i++ )
    {
        BOARD_STACKUP_ITEM* stackup_item = layers.at( i );

        for( int sublayer_id = 0; sublayer_id < stackup_item->GetSublayersCount(); sublayer_id++ )
        {
            // Layer names are empty until we close at least once the board setup dialog.
            // If the user did not open the dialog, then get the names from the board.
            // But dielectric layer names will be missing. And in stackup, the name is not very good
            // So, for dielectric, a name will be used, similar to the name build in gerber job file
            wxString layerName = stackup_item->GetLayerName();

            if( layerName.IsEmpty() )
            {
                if( IsValidLayer( stackup_item->GetBrdLayerId() ) )
                    layerName = aBoard->GetLayerName( stackup_item->GetBrdLayerId() );
            }

            if( stackup_item->GetType() == BS_ITEM_TYPE_DIELECTRIC )
            {
                layerName = _( "Dielectric" );

                if( stackup_item->GetSublayersCount() < 2 )
                    layerName <<  wxT( " " ) << stackup_item->GetDielectricLayerId();
                else
                    layerName << wxString::Format( _( " %d (%d/%d)" ), stackup_item->GetDielectricLayerId(),
                                        sublayer_id + 1, stackup_item->GetSublayersCount() );
            }

            addDataCell( layerName );

            addDataCell( InitialCaps( stackup_item->GetTypeName() ) );
            addDataCell( stackup_item->GetMaterial( sublayer_id ) );
            addDataCell( layerThicknessString( *stackup_item, sublayer_id ), 'R' );
            addDataCell( stackup_item->GetColor( sublayer_id ) );
            addDataCell( EDA_UNIT_UTILS::UI::StringFromValue( unityScale, EDA_UNITS::UNSCALED,
                                                              stackup_item->GetEpsilonR( sublayer_id ) ), 'R' );
            addDataCell( EDA_UNIT_UTILS::UI::StringFromValue( unityScale, EDA_UNITS::UNSCALED,
                                                              stackup_item->GetLossTangent( sublayer_id ) ), 'R' );
        }
    }

    table->Autosize();

    return table;
}

