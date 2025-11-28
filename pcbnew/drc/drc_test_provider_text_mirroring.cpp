/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers.
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
#include <pcb_tablecell.h>
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
    {}

    virtual ~DRC_TEST_PROVIDER_TEXT_MIRRORING() = default;

    virtual bool Run() override;

    virtual const wxString GetName() const override { return wxT( "text_mirroring" ); };
};


bool DRC_TEST_PROVIDER_TEXT_MIRRORING::Run()
{
    if( m_drcEngine->IsErrorLimitExceeded( DRCE_MIRRORED_TEXT_ON_FRONT_LAYER )
            && m_drcEngine->IsErrorLimitExceeded( DRCE_NONMIRRORED_TEXT_ON_BACK_LAYER ) )
    {
        REPORT_AUX( wxT( "Text mirroring violations ignored. Tests not run." ) );
        return true;        // continue with other tests
    }

    if( !reportPhase( _( "Checking text mirroring..." ) ) )
        return false;       // DRC cancelled

    LSET topLayers( { F_Cu, F_SilkS, F_Mask, F_Fab } );
    LSET bottomLayers( { B_Cu, B_SilkS, B_Mask, B_Fab } );

    auto checkTextMirroring =
            [&]( BOARD_ITEM* item, EDA_TEXT* text, bool isMirrored, int errorCode )
            {
                if( m_drcEngine->IsErrorLimitExceeded( errorCode ) )
                    return;

                bool layerMatch = ( isMirrored && topLayers.Contains( item->GetLayer() ) )
                                  || ( !isMirrored && bottomLayers.Contains( item->GetLayer() ) );

                if( layerMatch && text->IsMirrored() == isMirrored )
                {
                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( errorCode );
                    drcItem->SetItems( item );

                    reportViolation( drcItem, item->GetPosition(), item->GetLayer() );
                }
            };

    const int progressDelta = 500;
    int       count = 0;
    int       progressIndex = 0;

    static const std::vector<KICAD_T> itemTypes = {
            PCB_FIELD_T,
            PCB_TEXT_T, PCB_TEXTBOX_T, PCB_TABLECELL_T,
            PCB_DIMENSION_T
    };

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

                if( EDA_TEXT* text = dynamic_cast<EDA_TEXT*>( item ) )
                {
                    if( !text->IsVisible()
                            || !m_drcEngine->GetBoard()->IsLayerEnabled( item->GetLayer() )
                            || !m_drcEngine->GetBoard()->IsLayerVisible( item->GetLayer() ) )
                    {
                        return true;
                    }

                    checkTextMirroring( item, text, true, DRCE_MIRRORED_TEXT_ON_FRONT_LAYER );
                    checkTextMirroring( item, text, false, DRCE_NONMIRRORED_TEXT_ON_BACK_LAYER );
                }

                return true;
            } );

    return !m_drcEngine->IsCancelled();
}


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_TEXT_MIRRORING> dummy;
}
