/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021-2023 KiCad Developers.
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

#include <macros.h>
#include <pcb_field.h>
#include <pcb_text.h>
#include <pcb_textbox.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider.h>
#include <drc/drc_rtree.h>
#include <footprint.h>


/*
    Text mirroring tests.
    Errors generated:
    - DRCE_MIRRORED_TEXT_ON_FRONT_LAYER
    - DRCE_NONMIRRORED_TEXT_ON_BACK_LAYER
*/

class DRC_TEST_PROVIDER_TEXT_MIRRORING : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_TEXT_MIRRORING() 
    {
    }

    virtual ~DRC_TEST_PROVIDER_TEXT_MIRRORING() 
    {
    }

    virtual bool Run() override;

    virtual const wxString GetName() const override 
    { 
        return wxT( "text_mirroring" ); 
    };

    virtual const wxString GetDescription() const override
    {
        return wxT( "Tests mirrored text on top layer and non-mirrored text on bottom layer" );
    }
};


bool DRC_TEST_PROVIDER_TEXT_MIRRORING::Run()
{
    if( m_drcEngine->IsErrorLimitExceeded( DRCE_MIRRORED_TEXT_ON_FRONT_LAYER )
            && m_drcEngine->IsErrorLimitExceeded( DRCE_NONMIRRORED_TEXT_ON_BACK_LAYER ) )
    {
        reportAux( wxT( "Text mirroring violations ignored. Tests not run." ) );
        return true;        // continue with other tests
    }

    if( !reportPhase( _( "Checking text mirroring..." ) ) )
        return false; // DRC cancelled

    LSET topLayers( { F_Cu, F_SilkS, F_Mask, F_Fab } );
    LSET bottomLayers( { B_Cu, B_SilkS, B_Mask, B_Fab } );

    auto checkTextMirroring = [&]( BOARD_ITEM* item, EDA_TEXT* text, PCB_LAYER_ID layerId,
                                   bool isMirrored, int errorCode ) -> bool
            {
                if( m_drcEngine->IsErrorLimitExceeded( errorCode ) )
                    return false;

                bool layerMatch = ( isMirrored && topLayers.Contains( layerId ) )
                                  || ( !isMirrored && bottomLayers.Contains( layerId ) );

                if( layerMatch && text->IsMirrored() == isMirrored )
                {
                    auto drcItem = DRC_ITEM::Create( errorCode );

                    drcItem->SetErrorMessage( drcItem->GetErrorText() );
                    drcItem->SetItems( item );

                    reportViolation( drcItem, item->GetPosition(), layerId );
                }

                return true;
            };

    const int                         progressDelta = 250;
    int                               count = 0;
    int                               progressIndex = 0;
    static const std::vector<KICAD_T> itemTypes = { PCB_FIELD_T, PCB_TEXT_T, PCB_TEXTBOX_T };

    forEachGeometryItem( itemTypes, topLayers | bottomLayers,
                         [&]( BOARD_ITEM* item ) -> bool
                         {
                             ++count;
                             return true;
                         } );

    forEachGeometryItem( itemTypes, topLayers | bottomLayers,
                         [&]( BOARD_ITEM* item ) -> bool
                         {
                             if( !reportProgress( progressIndex++, count, progressDelta ) )
                                 return false;

                             EDA_TEXT* text = nullptr;

                             switch( item->Type() )
                             {
                             case PCB_FIELD_T: text = static_cast<PCB_FIELD*>( item ); break;
                             case PCB_TEXT_T: text = static_cast<PCB_TEXT*>( item ); break;
                             case PCB_TEXTBOX_T: text = static_cast<PCB_TEXTBOX*>( item ); break;
                             default: UNIMPLEMENTED_FOR( item->GetClass() ); break;
                             }

                             if( !text || !text->IsVisible()
                                 || !m_drcEngine->GetBoard()->IsLayerEnabled( item->GetLayer() )
                                 || !m_drcEngine->GetBoard()->IsLayerVisible( item->GetLayer() ) )
                                 return true;

                             if( !checkTextMirroring( item, text, item->GetLayer(), true, DRCE_MIRRORED_TEXT_ON_FRONT_LAYER ) ||
                                 !checkTextMirroring( item, text, item->GetLayer(), false, DRCE_NONMIRRORED_TEXT_ON_BACK_LAYER ) )
                             {
                                 return false;
                             }

                             return true;
                         } );

    reportRuleStatistics();

    return !m_drcEngine->IsCancelled();
}


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_TEXT_MIRRORING> dummy;
}
