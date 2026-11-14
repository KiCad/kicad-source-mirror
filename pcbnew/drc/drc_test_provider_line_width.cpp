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

#include <pcb_shape.h>
#include <pcb_textbox.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider.h>

/*
    Stroke width of graphic shapes and text box borders, text strokes are in the text dimensions test
    Errors generated:
    - DRCE_LINE_WIDTH
*/

class DRC_TEST_PROVIDER_LINE_WIDTH : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_LINE_WIDTH() = default;

    ~DRC_TEST_PROVIDER_LINE_WIDTH() override = default;

    bool Run() override;

    const wxString GetName() const override { return wxT( "line_width" ); }
};


bool DRC_TEST_PROVIDER_LINE_WIDTH::Run()
{
    const int progressDelta = 250;
    int       count = 0;
    int       ii = 0;

    if( m_drcEngine->IsErrorLimitExceeded( DRCE_LINE_WIDTH ) )
    {
        REPORT_AUX( wxT( "Line width violations ignored. Tests not run." ) );
        return true;        // continue with other tests
    }

    if( !m_drcEngine->HasRulesForConstraintType( LINE_WIDTH_CONSTRAINT ) )
    {
        REPORT_AUX( wxT( "No line width constraints found. Tests not run." ) );
        return true;        // continue with other tests
    }

    if( !reportPhase( _( "Checking graphic line widths..." ) ) )
        return false;       // DRC cancelled

    auto checkLineWidth =
            [&]( BOARD_ITEM* aItem, int aWidth )
            {
                DRC_CONSTRAINT constraint = m_drcEngine->EvalRules( LINE_WIDTH_CONSTRAINT, aItem, nullptr,
                                                                    aItem->GetLayer() );

                if( constraint.GetSeverity() == RPT_SEVERITY_IGNORE )
                    return;

                auto report =
                        [&]( const wxString& aFormat, int aLimit )
                        {
                            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_LINE_WIDTH );
                            drcItem->SetErrorDetail( formatMsg( aFormat, constraint.GetName(), aLimit, aWidth ) );
                            drcItem->SetItems( aItem );
                            drcItem->SetViolatingRule( constraint.GetParentRule() );

                            reportViolation( drcItem, aItem->GetPosition(), aItem->GetLayer() );
                        };

                if( constraint.Value().HasMin() && aWidth < constraint.Value().Min() )
                    report( _( "(%s min width %s; actual %s)" ), constraint.Value().Min() );

                if( constraint.Value().HasMax() && aWidth > constraint.Value().Max() )
                    report( _( "(%s max width %s; actual %s)" ), constraint.Value().Max() );
            };

    static const std::vector<KICAD_T> itemTypes = { PCB_SHAPE_T, PCB_TEXTBOX_T };

    forEachGeometryItem( itemTypes, LSET::AllLayersMask(),
            [&]( BOARD_ITEM* item ) -> bool
            {
                ++count;
                return true;
            } );

    forEachGeometryItem( itemTypes, LSET::AllLayersMask(),
            [&]( BOARD_ITEM* item ) -> bool
            {
                if( m_drcEngine->IsErrorLimitExceeded( DRCE_LINE_WIDTH ) )
                    return false;

                if( !reportProgress( ii++, count, progressDelta ) )
                    return false;

                if( item->Type() == PCB_TEXTBOX_T )
                {
                    PCB_TEXTBOX* textbox = static_cast<PCB_TEXTBOX*>( item );

                    if( textbox->IsBorderEnabled() )
                        checkLineWidth( textbox, textbox->GetBorderWidth() );

                    return true;
                }

                PCB_SHAPE* shape = static_cast<PCB_SHAPE*>( item );

                // A solid fill covers the stroke, so its width does not matter
                if( !shape->IsSolidFill() )
                    checkLineWidth( shape, shape->GetWidth() );

                return true;
            } );

    return !m_drcEngine->IsCancelled();
}


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_LINE_WIDTH> dummy;
}
