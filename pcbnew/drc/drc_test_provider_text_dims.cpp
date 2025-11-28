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
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider.h>
#include <font/font.h>
#include <pcb_dimension.h>


/*
    Text dimensions tests.
    Errors generated:
    - DRCE_TEXT_HEIGHT
    - DRCE_TEXT_THICKNESS
*/

class DRC_TEST_PROVIDER_TEXT_DIMS : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_TEXT_DIMS()
    {}

    virtual ~DRC_TEST_PROVIDER_TEXT_DIMS() = default;

    virtual bool Run() override;

    virtual const wxString GetName() const override { return wxT( "text_dimensions" ); };
};


bool DRC_TEST_PROVIDER_TEXT_DIMS::Run()
{
    const int progressDelta = 250;
    int       count = 0;
    int       ii = 0;

    if( m_drcEngine->IsErrorLimitExceeded( DRCE_TEXT_HEIGHT )
            && m_drcEngine->IsErrorLimitExceeded( DRCE_TEXT_THICKNESS ) )
    {
        REPORT_AUX( wxT( "Text dimension violations ignored. Tests not run." ) );
        return true;        // continue with other tests
    }

    if( !m_drcEngine->HasRulesForConstraintType( TEXT_HEIGHT_CONSTRAINT )
            && !m_drcEngine->HasRulesForConstraintType( TEXT_THICKNESS_CONSTRAINT ) )
    {
        REPORT_AUX( wxT( "No text height or text thickness constraints found. Tests not run." ) );
        return true;        // continue with other tests
    }

    if( !reportPhase( _( "Checking text dimensions..." ) ) )
        return false;       // DRC cancelled

    auto checkTextHeight =
            [&]( BOARD_ITEM* item, EDA_TEXT* text ) -> bool
            {
                if( m_drcEngine->IsErrorLimitExceeded( DRCE_TEXT_HEIGHT ) )
                    return false;

                DRC_CONSTRAINT constraint = m_drcEngine->EvalRules( TEXT_HEIGHT_CONSTRAINT, item,
                                                                    nullptr, item->GetLayer() );

                if( constraint.GetSeverity() == RPT_SEVERITY_IGNORE )
                    return true;

                int  actualHeight = text->GetTextSize().y;

                if( constraint.Value().HasMin() && actualHeight < constraint.Value().Min() )
                {
                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_TEXT_HEIGHT );
                    drcItem->SetErrorDetail( formatMsg( _( "(%s min height %s; actual %s)" ),
                                                        constraint.GetName(),
                                                        constraint.Value().Min(),
                                                        actualHeight ) );
                    drcItem->SetItems( item );
                    drcItem->SetViolatingRule( constraint.GetParentRule() );

                    reportViolation( drcItem, item->GetPosition(), item->GetLayer() );
                }

                if( constraint.Value().HasMax() && actualHeight > constraint.Value().Max() )
                {
                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_TEXT_HEIGHT );
                    drcItem->SetErrorDetail( formatMsg( _( "(%s max height %s; actual %s)" ),
                                                        constraint.GetName(),
                                                        constraint.Value().Max(),
                                                        actualHeight ) );
                    drcItem->SetItems( item );
                    drcItem->SetViolatingRule( constraint.GetParentRule() );

                    reportViolation( drcItem, item->GetPosition(), item->GetLayer() );
                }

                return true;
            };

    auto checkTextThickness =
            [&]( BOARD_ITEM* item, EDA_TEXT* text ) -> bool
            {
                DRC_CONSTRAINT constraint = m_drcEngine->EvalRules( TEXT_THICKNESS_CONSTRAINT, item,
                                                                    nullptr, item->GetLayer() );

                if( constraint.GetSeverity() == RPT_SEVERITY_IGNORE )
                    return true;

                KIFONT::FONT* font = text->GetDrawFont( nullptr );

                if( font->IsOutline() )
                {
                    if( !constraint.Value().HasMin() || constraint.Value().Min() <= 0 )
                        return true;

                    auto* glyphs = text->GetRenderCache( font, text->GetShownText( true ) );
                    bool  collapsedStroke = false;
                    bool  collapsedArea = false;

                    for( const std::unique_ptr<KIFONT::GLYPH>& glyph : *glyphs )
                    {
                        // Ensure the glyph is a OUTLINE_GLYPH (for instance, overbars in outline
                        // font text are represented as STROKE_GLYPHs).
                        if( !glyph->IsOutline() )
                            continue;

                        auto outlineGlyph = static_cast<KIFONT::OUTLINE_GLYPH*>( glyph.get() );
                        int  outlineCount = outlineGlyph->OutlineCount();
                        int  holeCount = 0;

                        if( outlineCount == 0 )
                            continue;           // ignore spaces

                        for( ii = 0; ii < outlineCount; ++ii )
                            holeCount += outlineGlyph->HoleCount( ii );

                        SHAPE_POLY_SET poly = outlineGlyph->CloneDropTriangulation();
                        poly.Deflate( constraint.Value().Min() / 2,
                                      CORNER_STRATEGY::CHAMFER_ALL_CORNERS, ARC_LOW_DEF );
                        poly.Simplify();

                        int resultingOutlineCount = poly.OutlineCount();
                        int resultingHoleCount = 0;

                        for( ii = 0; ii < resultingOutlineCount; ++ii )
                            resultingHoleCount += poly.HoleCount( ii );

                        if( ( resultingOutlineCount != outlineCount )
                                || ( resultingHoleCount != holeCount ) )
                        {
                            collapsedStroke = true;
                            break;
                        }

                        double glyphArea = outlineGlyph->Area();

                        if( glyphArea == 0 )
                            continue;

                        poly.Inflate( constraint.Value().Min() / 2,
                                      CORNER_STRATEGY::CHAMFER_ALL_CORNERS, ARC_LOW_DEF, true );

                        double resultingGlyphArea = poly.Area();

                        if( ( std::abs( resultingGlyphArea - glyphArea ) / glyphArea ) > 0.1 )
                        {
                            collapsedArea = true;
                            break;
                        }
                    }

                    if( collapsedStroke || collapsedArea )
                    {
                        std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_TEXT_THICKNESS );
                        drcItem->SetErrorDetail( _( "(TrueType font characters with insufficient stroke weight)" ) );
                        drcItem->SetItems( item );
                        drcItem->SetViolatingRule( constraint.GetParentRule() );

                        reportViolation( drcItem, item->GetPosition(), item->GetLayer() );
                    }
                }
                else
                {
                    int actualThickness = text->GetEffectiveTextPenWidth();

                    if( constraint.Value().HasMin() && actualThickness < constraint.Value().Min() )
                    {
                        std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_TEXT_THICKNESS );
                        drcItem->SetErrorDetail( formatMsg( _( "(%s min thickness %s; actual %s)" ),
                                                            constraint.GetName(),
                                                            constraint.Value().Min(),
                                                            actualThickness ) );
                        drcItem->SetItems( item );
                        drcItem->SetViolatingRule( constraint.GetParentRule() );

                        reportViolation( drcItem, item->GetPosition(), item->GetLayer() );
                    }

                    if( constraint.Value().HasMax() && actualThickness > constraint.Value().Max() )
                    {
                        std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_TEXT_THICKNESS );
                        drcItem->SetErrorDetail( formatMsg( _( "(%s max thickness %s; actual %s)" ),
                                                            constraint.GetName(),
                                                            constraint.Value().Max(),
                                                            actualThickness ) );
                        drcItem->SetItems( item );
                        drcItem->SetViolatingRule( constraint.GetParentRule() );

                        reportViolation( drcItem, item->GetPosition(), item->GetLayer() );
                    }
                }

                return true;
            };

    static const std::vector<KICAD_T> itemTypes = {
        PCB_FIELD_T,
        PCB_TEXT_T, PCB_TEXTBOX_T, PCB_TABLECELL_T,
        PCB_DIMENSION_T
    };

    forEachGeometryItem( itemTypes, LSET::AllLayersMask(),
            [&]( BOARD_ITEM* item ) -> bool
            {
                ++count;
                return true;
            } );

    forEachGeometryItem( itemTypes, LSET::AllLayersMask(),
            [&]( BOARD_ITEM* item ) -> bool
            {
                if( !reportProgress( ii++, count, progressDelta ) )
                    return false;

                if( EDA_TEXT* text = dynamic_cast<EDA_TEXT*>( item ) )
                {
                    int strikes = 0;

                    if( !text->IsVisible() )
                        return true;

                    if( m_drcEngine->IsErrorLimitExceeded( DRCE_TEXT_THICKNESS ) )
                        strikes++;
                    else
                        checkTextThickness( item, text );

                    if( m_drcEngine->IsErrorLimitExceeded( DRCE_TEXT_HEIGHT ) )
                        strikes++;
                    else
                        checkTextHeight( item, text );

                    if( strikes >= 2 )
                        return false;
                }

                return true;
            } );

    return !m_drcEngine->IsCancelled();
}


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_TEXT_DIMS> dummy;
}
