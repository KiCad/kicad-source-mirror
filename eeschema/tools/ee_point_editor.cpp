/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "ee_point_editor.h"

#include <functional>
using namespace std::placeholders;

#include <ee_grid_helper.h>
#include <tool/tool_manager.h>
#include <sch_commit.h>
#include <view/view_controls.h>
#include <gal/graphics_abstraction_layer.h>
#include <geometry/seg.h>
#include <geometry/shape_utils.h>
#include <tool/point_editor_behavior.h>
#include <tools/ee_actions.h>
#include <tools/ee_selection_tool.h>
#include <sch_edit_frame.h>
#include <sch_line.h>
#include <sch_bitmap.h>
#include <sch_sheet.h>
#include <sch_textbox.h>
#include <sch_table.h>
#include <symbol_editor/symbol_editor_settings.h>


static const std::vector<KICAD_T> pointEditorTypes = { SCH_SHAPE_T,
                                                       SCH_RULE_AREA_T,
                                                       SCH_TEXTBOX_T,
                                                       SCH_TABLECELL_T,
                                                       SCH_SHEET_T,
                                                       SCH_ITEM_LOCATE_GRAPHIC_LINE_T,
                                                       SCH_BITMAP_T };


// Few constants to avoid using bare numbers for point indices
enum ARC_POINTS
{
    ARC_START, ARC_END, ARC_CENTER
};


enum RECTANGLE_POINTS
{
    RECT_TOPLEFT, RECT_TOPRIGHT, RECT_BOTLEFT, RECT_BOTRIGHT, RECT_CENTER
};


enum RECTANGLE_LINES
{
    RECT_TOP, RECT_RIGHT, RECT_BOT, RECT_LEFT
};


enum REFIMAGE_POINTS
{
    REFIMG_ORIGIN = RECT_BOTRIGHT + 1
};


enum TABLECELL_POINTS
{
    COL_WIDTH, ROW_HEIGHT
};

enum LINE_POINTS
{
    LINE_START, LINE_END
};


class LINE_POINT_EDIT_BEHAVIOR : public POINT_EDIT_BEHAVIOR
{
public:
    LINE_POINT_EDIT_BEHAVIOR( SCH_LINE& aLine, SCH_SCREEN& aScreen ) :
            m_line( aLine ), m_screen( aScreen )
    {
    }

    void MakePoints( EDIT_POINTS& aPoints ) override
    {
        std::pair<EDA_ITEM*, int> connectedStart = { nullptr, STARTPOINT };
        std::pair<EDA_ITEM*, int> connectedEnd = { nullptr, STARTPOINT };

        for( SCH_ITEM* test : m_screen.Items().OfType( SCH_LINE_T ) )
        {
            if( test->GetLayer() != LAYER_NOTES )
                continue;

            if( test == &m_line )
                continue;

            SCH_LINE* testLine = static_cast<SCH_LINE*>( test );

            if( testLine->GetStartPoint() == m_line.GetStartPoint() )
            {
                connectedStart = { testLine, STARTPOINT };
            }
            else if( testLine->GetEndPoint() == m_line.GetStartPoint() )
            {
                connectedStart = { testLine, ENDPOINT };
            }
            else if( testLine->GetStartPoint() == m_line.GetEndPoint() )
            {
                connectedEnd = { testLine, STARTPOINT };
            }
            else if( testLine->GetEndPoint() == m_line.GetEndPoint() )
            {
                connectedEnd = { testLine, ENDPOINT };
            }
        }

        aPoints.AddPoint( m_line.GetStartPoint(), connectedStart );
        aPoints.AddPoint( m_line.GetEndPoint(), connectedEnd );
    }

    void UpdatePoints( EDIT_POINTS& aPoints ) override
    {
        aPoints.Point( LINE_START ).SetPosition( m_line.GetStartPoint() );
        aPoints.Point( LINE_END ).SetPosition( m_line.GetEndPoint() );
    }

    void UpdateItem( const EDIT_POINT& aEditedPoints, EDIT_POINTS& aPoints, COMMIT& aCommit,
                     std::vector<EDA_ITEM*>& aUpdatedItems ) override
    {
        m_line.SetStartPoint( aPoints.Point( LINE_START ).GetPosition() );
        m_line.SetEndPoint( aPoints.Point( LINE_END ).GetPosition() );

        std::pair<EDA_ITEM*, int> connected = aPoints.Point( LINE_START ).GetConnected();

        if( connected.first )
        {
            aCommit.Modify( connected.first, &m_screen );
            aUpdatedItems.push_back( connected.first );

            if( connected.second == STARTPOINT )
                static_cast<SCH_LINE*>( connected.first )->SetStartPoint( m_line.GetStartPoint() );
            else if( connected.second == ENDPOINT )
                static_cast<SCH_LINE*>( connected.first )->SetEndPoint( m_line.GetStartPoint() );
        }

        connected = aPoints.Point( LINE_END ).GetConnected();

        if( connected.first )
        {
            aCommit.Modify( connected.first, &m_screen );
            aUpdatedItems.push_back( connected.first );

            if( connected.second == STARTPOINT )
                static_cast<SCH_LINE*>( connected.first )->SetStartPoint( m_line.GetEndPoint() );
            else if( connected.second == ENDPOINT )
                static_cast<SCH_LINE*>( connected.first )->SetEndPoint( m_line.GetEndPoint() );
        }
    }

private:
    SCH_LINE&   m_line;
    SCH_SCREEN& m_screen;
};


class ARC_POINT_EDIT_BEHAVIOR : public POINT_EDIT_BEHAVIOR
{
public:
    ARC_POINT_EDIT_BEHAVIOR( SCH_SHAPE& aArc ) :
            m_arc( aArc )
    {
        wxASSERT( m_arc.GetShape() == SHAPE_T::ARC );
    }

    void MakePoints( EDIT_POINTS& aPoints ) override
    {
        aPoints.AddPoint( m_arc.GetStart() );
        aPoints.AddPoint( m_arc.GetEnd() );
        aPoints.AddPoint( m_arc.GetPosition() );

        aPoints.AddIndicatorLine( aPoints.Point( ARC_CENTER ), aPoints.Point( ARC_START ) );
        aPoints.AddIndicatorLine( aPoints.Point( ARC_CENTER ), aPoints.Point( ARC_END ) );
    }

    void UpdatePoints( EDIT_POINTS& aPoints ) override
    {
        aPoints.Point( ARC_START ).SetPosition( m_arc.GetStart() );
        aPoints.Point( ARC_END ).SetPosition( m_arc.GetEnd() );
        aPoints.Point( ARC_CENTER ).SetPosition( m_arc.GetPosition() );
    }

    void UpdateItem( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints, COMMIT& aCommit,
                     std::vector<EDA_ITEM*>& aUpdatedItems ) override
    {
        if( isModified( aEditedPoint, aPoints.Point( ARC_START ) ) )

        {
            m_arc.SetEditState( 2 );
            m_arc.CalcEdit( aPoints.Point( ARC_START ).GetPosition() );
        }
        else if( isModified( aEditedPoint, aPoints.Point( ARC_END ) ) )
        {
            m_arc.SetEditState( 3 );
            m_arc.CalcEdit( aPoints.Point( ARC_END ).GetPosition() );
        }
        else if( isModified( aEditedPoint, aPoints.Point( ARC_CENTER ) ) )
        {
            m_arc.SetEditState( 4 );
            m_arc.CalcEdit( aPoints.Point( ARC_CENTER ).GetPosition() );
        }

        aUpdatedItems.push_back( &m_arc );
    }
private:
    SCH_SHAPE& m_arc;
};


class BITMAP_POINT_EDIT_BEHAVIOR : public POINT_EDIT_BEHAVIOR
{
public:
    BITMAP_POINT_EDIT_BEHAVIOR( SCH_BITMAP& aBitmap ) : m_bitmap( aBitmap ) {}

    void MakePoints( EDIT_POINTS& aPoints ) override
    {
        const REFERENCE_IMAGE& refImage = m_bitmap.GetReferenceImage();
        const VECTOR2I         topLeft = refImage.GetPosition() - refImage.GetSize() / 2;
        const VECTOR2I         botRight = refImage.GetPosition() + refImage.GetSize() / 2;

        aPoints.AddPoint( topLeft );
        aPoints.AddPoint( VECTOR2I( botRight.x, topLeft.y ) );
        aPoints.AddPoint( VECTOR2I( topLeft.x, botRight.y ) );
        aPoints.AddPoint( botRight );

        aPoints.AddPoint( refImage.GetPosition() + refImage.GetTransformOriginOffset() );
    }

    void UpdatePoints( EDIT_POINTS& aPoints ) override
    {
        const REFERENCE_IMAGE& refImage = m_bitmap.GetReferenceImage();
        const VECTOR2I         topLeft = refImage.GetPosition() - refImage.GetSize() / 2;
        const VECTOR2I         botRight = refImage.GetPosition() + refImage.GetSize() / 2;

        aPoints.Point( RECT_TOPLEFT ).SetPosition( topLeft );
        aPoints.Point( RECT_TOPRIGHT ).SetPosition( botRight.x, topLeft.y );
        aPoints.Point( RECT_BOTLEFT ).SetPosition( topLeft.x, botRight.y );
        aPoints.Point( RECT_BOTRIGHT ).SetPosition( botRight );

        aPoints.Point( REFIMG_ORIGIN )
                .SetPosition( refImage.GetPosition() + refImage.GetTransformOriginOffset() );
    }

    void UpdateItem( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints, COMMIT& aCommit,
                     std::vector<EDA_ITEM*>& aUpdatedItems ) override
    {
        REFERENCE_IMAGE& refImg = m_bitmap.GetReferenceImage();
        const VECTOR2I   topLeft = aPoints.Point( RECT_TOPLEFT ).GetPosition();
        const VECTOR2I   topRight = aPoints.Point( RECT_TOPRIGHT ).GetPosition();
        const VECTOR2I   botLeft = aPoints.Point( RECT_BOTLEFT ).GetPosition();
        const VECTOR2I   botRight = aPoints.Point( RECT_BOTRIGHT ).GetPosition();
        const VECTOR2I   xfrmOrigin = aPoints.Point( REFIMG_ORIGIN ).GetPosition();

        if( isModified( aEditedPoint, aPoints.Point( REFIMG_ORIGIN ) ) )
        {
            // Moving the transform origin
            // As the other points didn't move, we can get the image extent from them
            const VECTOR2I newOffset = xfrmOrigin - ( topLeft + botRight ) / 2;
            refImg.SetTransformOriginOffset( newOffset );
        }
        else
        {
            const VECTOR2I oldOrigin = refImg.GetPosition() + refImg.GetTransformOriginOffset();
            const VECTOR2I oldSize = refImg.GetSize();
            const VECTOR2I pos = refImg.GetPosition();

            OPT_VECTOR2I newCorner;
            VECTOR2I     oldCorner = pos;

            if( isModified( aEditedPoint, aPoints.Point( RECT_TOPLEFT ) ) )
            {
                newCorner = topLeft;
                oldCorner -= oldSize / 2;
            }
            else if( isModified( aEditedPoint, aPoints.Point( RECT_TOPRIGHT ) ) )
            {
                newCorner = topRight;
                oldCorner -= VECTOR2I( -oldSize.x, oldSize.y ) / 2;
            }
            else if( isModified( aEditedPoint, aPoints.Point( RECT_BOTLEFT ) ) )
            {
                newCorner = botLeft;
                oldCorner -= VECTOR2I( oldSize.x, -oldSize.y ) / 2;
            }
            else if( isModified( aEditedPoint, aPoints.Point( RECT_BOTRIGHT ) ) )
            {
                newCorner = botRight;
                oldCorner += oldSize / 2;
            }

            if( newCorner )
            {
                // Turn in the respective vectors from the origin
                *newCorner -= xfrmOrigin;
                oldCorner -= oldOrigin;

                // If we tried to cross the origin, clamp it to stop it
                if( sign( newCorner->x ) != sign( oldCorner.x )
                    || sign( newCorner->y ) != sign( oldCorner.y ) )
                {
                    *newCorner = VECTOR2I( 0, 0 );
                }

                const double newLength = newCorner->EuclideanNorm();
                const double oldLength = oldCorner.EuclideanNorm();

                double ratio = oldLength > 0 ? ( newLength / oldLength ) : 1.0;

                // Clamp the scaling to a minimum of 50 mils
                VECTOR2I newSize = oldSize * ratio;
                double newWidth = std::max( newSize.x, EDA_UNIT_UTILS::Mils2IU( schIUScale, 50 ) );
                double newHeight = std::max( newSize.y, EDA_UNIT_UTILS::Mils2IU( schIUScale, 50 ) );
                ratio = std::min( newWidth / oldSize.x, newHeight / oldSize.y );

                // Also handles the origin offset
                refImg.SetImageScale( refImg.GetImageScale() * ratio );
            }
        }
        aUpdatedItems.push_back( &m_bitmap );
    }

private:
    SCH_BITMAP& m_bitmap;
};


class SCH_TABLECELL_POINT_EDIT_BEHAVIOR : public EDA_TABLECELL_POINT_EDIT_BEHAVIOR
{
public:
    SCH_TABLECELL_POINT_EDIT_BEHAVIOR( SCH_TABLECELL& aCell ) :
            EDA_TABLECELL_POINT_EDIT_BEHAVIOR( aCell ), m_cell( aCell )
    {
    }

    void UpdateItem( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints, COMMIT& aCommit,
                     std::vector<EDA_ITEM*>& aUpdatedItems ) override
    {
        SCH_TABLE& table = static_cast<SCH_TABLE&>( *m_cell.GetParent() );
        aCommit.Modify( &table );
        aUpdatedItems.push_back( &table );

        if( isModified( aEditedPoint, aPoints.Point( COL_WIDTH ) ) )
        {
            m_cell.SetEnd( VECTOR2I( aPoints.Point( 0 ).GetX(), m_cell.GetEndY() ) );

            int colWidth = m_cell.GetRectangleWidth();

            for( int ii = 0; ii < m_cell.GetColSpan() - 1; ++ii )
                colWidth -= table.GetColWidth( m_cell.GetColumn() + ii );

            table.SetColWidth( m_cell.GetColumn() + m_cell.GetColSpan() - 1, colWidth );
        }
        else if( isModified( aEditedPoint, aPoints.Point( ROW_HEIGHT ) ) )
        {
            m_cell.SetEnd( VECTOR2I( m_cell.GetEndX(), aPoints.Point( 1 ).GetY() ) );

            int rowHeight = m_cell.GetRectangleHeight();

            for( int ii = 0; ii < m_cell.GetRowSpan() - 1; ++ii )
                rowHeight -= table.GetRowHeight( m_cell.GetRow() + ii );

            table.SetRowHeight( m_cell.GetRow() + m_cell.GetRowSpan() - 1, rowHeight );
        }

        table.Normalize();
    }

private:
    SCH_TABLECELL& m_cell;
};


class RECTANGLE_POINT_EDIT_BEHAVIOR : public POINT_EDIT_BEHAVIOR
{
public:
    RECTANGLE_POINT_EDIT_BEHAVIOR( SCH_SHAPE& aRect, EDA_DRAW_FRAME& aFrame ) :
            m_rect( aRect ), m_frame( aFrame )
    {
    }

    static void MakePoints( SCH_SHAPE& aRect, EDIT_POINTS& aPoints )
    {
        VECTOR2I topLeft = aRect.GetPosition();
        VECTOR2I botRight = aRect.GetEnd();

        aPoints.AddPoint( topLeft );
        aPoints.AddPoint( VECTOR2I( botRight.x, topLeft.y ) );
        aPoints.AddPoint( VECTOR2I( topLeft.x, botRight.y ) );
        aPoints.AddPoint( botRight );
        aPoints.AddPoint( aRect.GetCenter() );

        aPoints.AddLine( aPoints.Point( RECT_TOPLEFT ), aPoints.Point( RECT_TOPRIGHT ) );
        aPoints.Line( RECT_TOP ).SetConstraint( new EC_PERPLINE( aPoints.Line( RECT_TOP ) ) );
        aPoints.AddLine( aPoints.Point( RECT_TOPRIGHT ), aPoints.Point( RECT_BOTRIGHT ) );
        aPoints.Line( RECT_RIGHT ).SetConstraint( new EC_PERPLINE( aPoints.Line( RECT_RIGHT ) ) );
        aPoints.AddLine( aPoints.Point( RECT_BOTRIGHT ), aPoints.Point( RECT_BOTLEFT ) );
        aPoints.Line( RECT_BOT ).SetConstraint( new EC_PERPLINE( aPoints.Line( RECT_BOT ) ) );
        aPoints.AddLine( aPoints.Point( RECT_BOTLEFT ), aPoints.Point( RECT_TOPLEFT ) );
        aPoints.Line( RECT_LEFT ).SetConstraint( new EC_PERPLINE( aPoints.Line( RECT_LEFT ) ) );
    }

    static void UpdatePoints( SCH_SHAPE& aRect, EDIT_POINTS& aPoints )
    {
        VECTOR2I topLeft = aRect.GetPosition();
        VECTOR2I botRight = aRect.GetEnd();

        aPoints.Point( RECT_TOPLEFT ).SetPosition( topLeft );
        aPoints.Point( RECT_TOPRIGHT ).SetPosition( VECTOR2I( botRight.x, topLeft.y ) );
        aPoints.Point( RECT_BOTLEFT ).SetPosition( VECTOR2I( topLeft.x, botRight.y ) );
        aPoints.Point( RECT_BOTRIGHT ).SetPosition( botRight );
        aPoints.Point( RECT_CENTER ).SetPosition( aRect.GetCenter() );
    }

    /**
     * Update the coordinates of 4 corners of a rectangle, according to constraints
     * and the moved corner
     * @param minWidth is the minimal width constraint
     * @param minHeight is the minimal height constraint
     * @param topLeft is the RECT_TOPLEFT to constraint
     * @param topRight is the RECT_TOPRIGHT to constraint
     * @param botLeft is the RECT_BOTLEFT to constraint
     * @param botRight is the RECT_BOTRIGHT to constraint
     */
    static void PinEditedCorner( const EDIT_POINT& aEditedPoint, const EDIT_POINTS& aPoints,
                                 int minWidth, int minHeight, VECTOR2I& topLeft, VECTOR2I& topRight,
                                 VECTOR2I& botLeft, VECTOR2I& botRight )
    {
        if( isModified( aEditedPoint, aPoints.Point( RECT_TOPLEFT ) ) )
        {
            // pin edited point within opposite corner
            topLeft.x = std::min( topLeft.x, botRight.x - minWidth );
            topLeft.y = std::min( topLeft.y, botRight.y - minHeight );

            // push edited point edges to adjacent corners
            topRight.y = topLeft.y;
            botLeft.x = topLeft.x;
        }
        else if( isModified( aEditedPoint, aPoints.Point( RECT_TOPRIGHT ) ) )
        {
            // pin edited point within opposite corner
            topRight.x = std::max( topRight.x, botLeft.x + minWidth );
            topRight.y = std::min( topRight.y, botLeft.y - minHeight );

            // push edited point edges to adjacent corners
            topLeft.y = topRight.y;
            botRight.x = topRight.x;
        }
        else if( isModified( aEditedPoint, aPoints.Point( RECT_BOTLEFT ) ) )
        {
            // pin edited point within opposite corner
            botLeft.x = std::min( botLeft.x, topRight.x - minWidth );
            botLeft.y = std::max( botLeft.y, topRight.y + minHeight );

            // push edited point edges to adjacent corners
            botRight.y = botLeft.y;
            topLeft.x = botLeft.x;
        }
        else if( isModified( aEditedPoint, aPoints.Point( RECT_BOTRIGHT ) ) )
        {
            // pin edited point within opposite corner
            botRight.x = std::max( botRight.x, topLeft.x + minWidth );
            botRight.y = std::max( botRight.y, topLeft.y + minHeight );

            // push edited point edges to adjacent corners
            botLeft.y = botRight.y;
            topRight.x = botRight.x;
        }
        else if( isModified( aEditedPoint, aPoints.Line( RECT_TOP ) ) )
        {
            topLeft.y = std::min( topLeft.y, botRight.y - minHeight );
        }
        else if( isModified( aEditedPoint, aPoints.Line( RECT_LEFT ) ) )
        {
            topLeft.x = std::min( topLeft.x, botRight.x - minWidth );
        }
        else if( isModified( aEditedPoint, aPoints.Line( RECT_BOT ) ) )
        {
            botRight.y = std::max( botRight.y, topLeft.y + minHeight );
        }
        else if( isModified( aEditedPoint, aPoints.Line( RECT_RIGHT ) ) )
        {
            botRight.x = std::max( botRight.x, topLeft.x + minWidth );
        }
    }

    static void UpdateItem( SCH_SHAPE& aRect, const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints )
    {
        VECTOR2I topLeft = aPoints.Point( RECT_TOPLEFT ).GetPosition();
        VECTOR2I topRight = aPoints.Point( RECT_TOPRIGHT ).GetPosition();
        VECTOR2I botLeft = aPoints.Point( RECT_BOTLEFT ).GetPosition();
        VECTOR2I botRight = aPoints.Point( RECT_BOTRIGHT ).GetPosition();

        PinEditedCorner( aEditedPoint, aPoints, schIUScale.MilsToIU( 1 ), schIUScale.MilsToIU( 1 ),
                         topLeft, topRight, botLeft, botRight );

        if( isModified( aEditedPoint, aPoints.Point( RECT_TOPLEFT ) )
            || isModified( aEditedPoint, aPoints.Point( RECT_TOPRIGHT ) )
            || isModified( aEditedPoint, aPoints.Point( RECT_BOTRIGHT ) )
            || isModified( aEditedPoint, aPoints.Point( RECT_BOTLEFT ) ) )
        {
            aRect.SetPosition( topLeft );
            aRect.SetEnd( botRight );
        }
        else if( isModified( aEditedPoint, aPoints.Line( RECT_TOP ) ) )
        {
            aRect.SetStartY( topLeft.y );
        }
        else if( isModified( aEditedPoint, aPoints.Line( RECT_LEFT ) ) )
        {
            aRect.SetStartX( topLeft.x );
        }
        else if( isModified( aEditedPoint, aPoints.Line( RECT_BOT ) ) )
        {
            aRect.SetEndY( botRight.y );
        }
        else if( isModified( aEditedPoint, aPoints.Line( RECT_RIGHT ) ) )
        {
            aRect.SetEndX( botRight.x );
        }

        for( unsigned i = 0; i < aPoints.LinesSize(); ++i )
        {
            if( !isModified( aEditedPoint, aPoints.Line( i ) ) )
            {
                aPoints.Line( i ).SetConstraint( new EC_PERPLINE( aPoints.Line( i ) ) );
            }
        }
    }

    void MakePoints( EDIT_POINTS& aPoints ) override
    {
        m_rect.Normalize();
        MakePoints( m_rect, aPoints );
    }

    void UpdatePoints( EDIT_POINTS& aPoints ) override
    {
        m_rect.Normalize();
        UpdatePoints( m_rect, aPoints );
    }

    void UpdateItem( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints, COMMIT& aCommit,
                     std::vector<EDA_ITEM*>& aUpdatedItems ) override
    {
        VECTOR2I topLeft = aPoints.Point( RECT_TOPLEFT ).GetPosition();
        VECTOR2I topRight = aPoints.Point( RECT_TOPRIGHT ).GetPosition();
        VECTOR2I botLeft = aPoints.Point( RECT_BOTLEFT ).GetPosition();
        VECTOR2I botRight = aPoints.Point( RECT_BOTRIGHT ).GetPosition();

        PinEditedCorner( aEditedPoint, aPoints, schIUScale.MilsToIU( 1 ), schIUScale.MilsToIU( 1 ),
                         topLeft, topRight, botLeft, botRight );

        const BOX2I           oldBox = BOX2I::ByCorners( m_rect.GetStart(), m_rect.GetEnd() );
        std::vector<SEG>      oldSegs;
        std::vector<VECTOR2I> moveVecs;

        if( isModified( aEditedPoint, aPoints.Point( RECT_TOPLEFT ) )
            || isModified( aEditedPoint, aPoints.Point( RECT_TOPRIGHT ) )
            || isModified( aEditedPoint, aPoints.Point( RECT_BOTRIGHT ) )
            || isModified( aEditedPoint, aPoints.Point( RECT_BOTLEFT ) ) )
        {
            // Corner drags don't update pins. Not only is it an escape hatch to avoid
            // moving pins, it also avoids tricky problems when the pins "fall off"
            // the ends of one of the two segments and get either left behind or
            // "swept up" into the corner.
            m_rect.SetPosition( topLeft );
            m_rect.SetEnd( botRight );
        }
        else if( isModified( aEditedPoint, aPoints.Point( RECT_CENTER ) ) )
        {
            VECTOR2I moveVec = aPoints.Point( RECT_CENTER ).GetPosition() - oldBox.GetCenter();
            m_rect.Move( moveVec );
        }
        else if( isModified( aEditedPoint, aPoints.Line( RECT_TOP ) ) )
        {
            oldSegs = KIGEOM::GetSegsInDirection( oldBox, DIRECTION_45::Directions::N );
            moveVecs.emplace_back( 0, topLeft.y - oldBox.GetTop() );
            m_rect.SetStartY( topLeft.y );
        }
        else if( isModified( aEditedPoint, aPoints.Line( RECT_LEFT ) ) )
        {
            oldSegs = KIGEOM::GetSegsInDirection( oldBox, DIRECTION_45::Directions::W );
            moveVecs.emplace_back( topLeft.x - oldBox.GetLeft(), 0 );
            m_rect.SetStartX( topLeft.x );
        }
        else if( isModified( aEditedPoint, aPoints.Line( RECT_BOT ) ) )
        {
            oldSegs = KIGEOM::GetSegsInDirection( oldBox, DIRECTION_45::Directions::S );
            moveVecs.emplace_back( 0, botRight.y - oldBox.GetBottom() );
            m_rect.SetEndY( botRight.y );
        }
        else if( isModified( aEditedPoint, aPoints.Line( RECT_RIGHT ) ) )
        {
            oldSegs = KIGEOM::GetSegsInDirection( oldBox, DIRECTION_45::Directions::E );
            moveVecs.emplace_back( botRight.x - oldBox.GetRight(), 0 );
            m_rect.SetEndX( botRight.x );
        }

        dragPinsOnEdge( oldSegs, moveVecs, m_rect.GetUnit(), aCommit, aUpdatedItems );

        for( unsigned i = 0; i < aPoints.LinesSize(); ++i )
        {
            if( !isModified( aEditedPoint, aPoints.Line( i ) ) )
            {
                aPoints.Line( i ).SetConstraint( new EC_PERPLINE( aPoints.Line( i ) ) );
            }
        }
    }

private:
    void dragPinsOnEdge( const std::vector<SEG>& aOldEdges, const std::vector<VECTOR2I>& aMoveVecs,
                         int aEdgeUnit, COMMIT& aCommit,
                         std::vector<EDA_ITEM*>& aUpdatedItems ) const
    {
        wxCHECK( aOldEdges.size() == aMoveVecs.size(), /* void */ );

        // This only make sense in the symbol editor
        if( !m_frame.IsType( FRAME_SCH_SYMBOL_EDITOR ) )
            return;

        SYMBOL_EDIT_FRAME& editor = static_cast<SYMBOL_EDIT_FRAME&>( m_frame );

        // And only if the setting is enabled
        if( !editor.GetSettings()->m_dragPinsAlongWithEdges )
            return;

        // Adjuting pins on a different unit to a unit-limited shape
        // seems suspect.
        wxCHECK( aEdgeUnit == 0 || aEdgeUnit == editor.GetUnit(), /* void */ );

        /*
        * Get a list of pins on a line segment
        */
        const auto getPinsOnSeg = []( LIB_SYMBOL& aSymbol, int aUnit, const SEG& aSeg,
                                      bool aIncludeEnds ) -> std::vector<SCH_PIN*>
        {
            // const BOX2I     segBox = BOX2I::ByCorners( aSeg.A, aSeg.B ).GetInflated( 1 );
            // const EE_RTREE& rtree = m_frame->GetScreen()->Items().Overlapping( SCH_PIN_T, segBox );

            std::vector<SCH_PIN*> pins;

            for( SCH_PIN* pin : aSymbol.GetPins( aUnit ) )
            {
                // Figure out if the pin "connects" to the line
                const VECTOR2I pinRootPos = pin->GetPinRoot();

                if( aSeg.Contains( pinRootPos ) )
                {
                    if( aIncludeEnds || ( pinRootPos != aSeg.A && pinRootPos != aSeg.B ) )
                    {
                        pins.push_back( pin );
                    }
                }
            }

            return pins;
        };

        LIB_SYMBOL* const symbol = editor.GetCurSymbol();

        for( std::size_t i = 0; i < aOldEdges.size(); ++i )
        {
            if( aMoveVecs[i] == VECTOR2I( 0, 0 ) || !symbol )
                continue;

            const std::vector<SCH_PIN*> pins =
                    getPinsOnSeg( *symbol, aEdgeUnit, aOldEdges[i], false );

            for( SCH_PIN* pin : pins )
            {
                aCommit.Modify( pin );
                aUpdatedItems.push_back( pin );

                // Move the pin
                pin->Move( aMoveVecs[i] );
            }
        }
    }

    SCH_SHAPE&      m_rect;
    EDA_DRAW_FRAME& m_frame;
};


class TEXTBOX_POINT_EDIT_BEHAVIOR : public POINT_EDIT_BEHAVIOR
{
public:
    TEXTBOX_POINT_EDIT_BEHAVIOR( SCH_TEXTBOX& aTextbox ) : m_textbox( aTextbox ) {}

    void MakePoints( EDIT_POINTS& aPoints ) override
    {
        m_textbox.Normalize();
        RECTANGLE_POINT_EDIT_BEHAVIOR::MakePoints( m_textbox, aPoints );
    }

    void UpdatePoints( EDIT_POINTS& aPoints ) override
    {
        // point editor works only with rectangles having width and height > 0
        // Some symbols can have rectangles with width or height < 0
        // So normalize the size:
        m_textbox.Normalize();
        RECTANGLE_POINT_EDIT_BEHAVIOR::UpdatePoints( m_textbox, aPoints );
    }

    void UpdateItem( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints, COMMIT& aCommit,
                     std::vector<EDA_ITEM*>& aUpdatedItems ) override
    {
        RECTANGLE_POINT_EDIT_BEHAVIOR::UpdateItem( m_textbox, aEditedPoint, aPoints );
        m_textbox.ClearRenderCache();
    }

private:
    SCH_TEXTBOX& m_textbox;
};


class SHEET_POINT_EDIT_BEHAVIOR : public POINT_EDIT_BEHAVIOR
{
public:
    SHEET_POINT_EDIT_BEHAVIOR( SCH_SHEET& aSheet ) : m_sheet( aSheet ) {}

    void MakePoints( EDIT_POINTS& aPoints ) override
    {
        VECTOR2I topLeft = m_sheet.GetPosition();
        VECTOR2I botRight = m_sheet.GetPosition() + m_sheet.GetSize();

        aPoints.AddPoint( topLeft );
        aPoints.AddPoint( VECTOR2I( botRight.x, topLeft.y ) );
        aPoints.AddPoint( VECTOR2I( topLeft.x, botRight.y ) );
        aPoints.AddPoint( botRight );

        aPoints.AddLine( aPoints.Point( RECT_TOPLEFT ), aPoints.Point( RECT_TOPRIGHT ) );
        aPoints.Line( RECT_TOP ).SetConstraint( new EC_PERPLINE( aPoints.Line( RECT_TOP ) ) );
        aPoints.AddLine( aPoints.Point( RECT_TOPRIGHT ), aPoints.Point( RECT_BOTRIGHT ) );
        aPoints.Line( RECT_RIGHT ).SetConstraint( new EC_PERPLINE( aPoints.Line( RECT_RIGHT ) ) );
        aPoints.AddLine( aPoints.Point( RECT_BOTRIGHT ), aPoints.Point( RECT_BOTLEFT ) );
        aPoints.Line( RECT_BOT ).SetConstraint( new EC_PERPLINE( aPoints.Line( RECT_BOT ) ) );
        aPoints.AddLine( aPoints.Point( RECT_BOTLEFT ), aPoints.Point( RECT_TOPLEFT ) );
        aPoints.Line( RECT_LEFT ).SetConstraint( new EC_PERPLINE( aPoints.Line( RECT_LEFT ) ) );
    }

    void UpdatePoints( EDIT_POINTS& aPoints ) override
    {
        VECTOR2I topLeft = m_sheet.GetPosition();
        VECTOR2I botRight = m_sheet.GetPosition() + m_sheet.GetSize();

        aPoints.Point( RECT_TOPLEFT ).SetPosition( topLeft );
        aPoints.Point( RECT_TOPRIGHT ).SetPosition( botRight.x, topLeft.y );
        aPoints.Point( RECT_BOTLEFT ).SetPosition( topLeft.x, botRight.y );
        aPoints.Point( RECT_BOTRIGHT ).SetPosition( botRight );
    }

    void UpdateItem( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints, COMMIT& aCommit,
                     std::vector<EDA_ITEM*>& aUpdatedItems ) override
    {
        VECTOR2I topLeft = aPoints.Point( RECT_TOPLEFT ).GetPosition();
        VECTOR2I topRight = aPoints.Point( RECT_TOPRIGHT ).GetPosition();
        VECTOR2I botLeft = aPoints.Point( RECT_BOTLEFT ).GetPosition();
        VECTOR2I botRight = aPoints.Point( RECT_BOTRIGHT ).GetPosition();
        VECTOR2I sheetNewPos = m_sheet.GetPosition();
        VECTOR2I sheetNewSize = m_sheet.GetSize();

        bool editedTopRight = isModified( aEditedPoint, aPoints.Point( RECT_TOPRIGHT ) );
        bool editedBotLeft = isModified( aEditedPoint, aPoints.Point( RECT_BOTLEFT ) );
        bool editedBotRight = isModified( aEditedPoint, aPoints.Point( RECT_BOTRIGHT ) );

        if( isModified( aEditedPoint, aPoints.Line( RECT_RIGHT ) ) )
            editedTopRight = true;
        else if( isModified( aEditedPoint, aPoints.Line( RECT_BOT ) ) )
            editedBotLeft = true;

        RECTANGLE_POINT_EDIT_BEHAVIOR::PinEditedCorner(
                aEditedPoint, aPoints, m_sheet.GetMinWidth( editedTopRight || editedBotRight ),
                m_sheet.GetMinHeight( editedBotLeft || editedBotRight ), topLeft, topRight, botLeft,
                botRight );

        if( isModified( aEditedPoint, aPoints.Point( RECT_TOPLEFT ) )
            || isModified( aEditedPoint, aPoints.Point( RECT_TOPRIGHT ) )
            || isModified( aEditedPoint, aPoints.Point( RECT_BOTRIGHT ) )
            || isModified( aEditedPoint, aPoints.Point( RECT_BOTLEFT ) ) )
        {
            sheetNewPos = topLeft;
            sheetNewSize = VECTOR2I( botRight.x - topLeft.x, botRight.y - topLeft.y );
        }
        else if( isModified( aEditedPoint, aPoints.Line( RECT_TOP ) ) )
        {
            sheetNewPos = VECTOR2I( m_sheet.GetPosition().x, topLeft.y );
            sheetNewSize = VECTOR2I( m_sheet.GetSize().x, botRight.y - topLeft.y );
        }
        else if( isModified( aEditedPoint, aPoints.Line( RECT_LEFT ) ) )
        {
            sheetNewPos = VECTOR2I( topLeft.x, m_sheet.GetPosition().y );
            sheetNewSize = VECTOR2I( botRight.x - topLeft.x, m_sheet.GetSize().y );
        }
        else if( isModified( aEditedPoint, aPoints.Line( RECT_BOT ) ) )
        {
            sheetNewSize = VECTOR2I( m_sheet.GetSize().x, botRight.y - topLeft.y );
        }
        else if( isModified( aEditedPoint, aPoints.Line( RECT_RIGHT ) ) )
        {
            sheetNewSize = VECTOR2I( botRight.x - topLeft.x, m_sheet.GetSize().y );
        }

        for( unsigned i = 0; i < aPoints.LinesSize(); ++i )
        {
            if( !isModified( aEditedPoint, aPoints.Line( i ) ) )
            {
                aPoints.Line( i ).SetConstraint( new EC_PERPLINE( aPoints.Line( i ) ) );
            }
        }

        if( m_sheet.GetPosition() != sheetNewPos )
            m_sheet.SetPositionIgnoringPins( sheetNewPos );

        if( m_sheet.GetSize() != sheetNewSize )
            m_sheet.Resize( sheetNewSize );
    }

private:
    SCH_SHEET& m_sheet;
};


void EE_POINT_EDITOR::makePointsAndBehavior( EDA_ITEM* aItem )
{
    m_editBehavior = nullptr;
    m_editPoints = std::make_shared<EDIT_POINTS>( aItem );

    if( !aItem )
        return;

    // Generate list of edit points based on the item type
    switch( aItem->Type() )
    {
    case SCH_SHAPE_T:
    {
        SCH_SHAPE* shape = static_cast<SCH_SHAPE*>( aItem );

        switch( shape->GetShape() )
        {
        case SHAPE_T::ARC:
            m_editBehavior = std::make_unique<ARC_POINT_EDIT_BEHAVIOR>( *shape );
            break;
        case SHAPE_T::CIRCLE:
            m_editBehavior = std::make_unique<EDA_CIRCLE_POINT_EDIT_BEHAVIOR>( *shape );
            break;
        case SHAPE_T::RECTANGLE:
            m_editBehavior = std::make_unique<RECTANGLE_POINT_EDIT_BEHAVIOR>( *shape, *m_frame );
            break;
        case SHAPE_T::POLY:
            m_editBehavior = std::make_unique<EDA_POLYGON_POINT_EDIT_BEHAVIOR>( *shape );
            break;
        case SHAPE_T::BEZIER:
            m_editBehavior = std::make_unique<EDA_BEZIER_POINT_EDIT_BEHAVIOR>( *shape );
            break;
        default:
            UNIMPLEMENTED_FOR( shape->SHAPE_T_asString() );
        }

        break;
    }
    case SCH_RULE_AREA_T:
    {
        SCH_SHAPE* shape = static_cast<SCH_SHAPE*>( aItem );
        // Implemented directly as a polygon
        m_editBehavior = std::make_unique<EDA_POLYGON_POINT_EDIT_BEHAVIOR>( *shape );
        break;
    }
    case SCH_TEXTBOX_T:
    {
        SCH_TEXTBOX* textbox = static_cast<SCH_TEXTBOX*>( aItem );
        m_editBehavior = std::make_unique<TEXTBOX_POINT_EDIT_BEHAVIOR>( *textbox );
        break;
    }
    case SCH_TABLECELL_T:
    {
        SCH_TABLECELL* cell = static_cast<SCH_TABLECELL*>( aItem );
        m_editBehavior = std::make_unique<SCH_TABLECELL_POINT_EDIT_BEHAVIOR>( *cell );
        break;
    }
    case SCH_SHEET_T:
    {
        SCH_SHEET& sheet = static_cast<SCH_SHEET&>( *aItem );
        m_editBehavior = std::make_unique<SHEET_POINT_EDIT_BEHAVIOR>( sheet );
        break;
    }
    case SCH_BITMAP_T:
    {
        SCH_BITMAP& bitmap = static_cast<SCH_BITMAP&>( *aItem );
        m_editBehavior = std::make_unique<BITMAP_POINT_EDIT_BEHAVIOR>( bitmap );
        break;
    }
    case SCH_LINE_T:
    {
        SCH_LINE& line = static_cast<SCH_LINE&>( *aItem );
        m_editBehavior = std::make_unique<LINE_POINT_EDIT_BEHAVIOR>( line, *m_frame->GetScreen() );
        break;
    }
    default:
    {
        m_editPoints.reset();
        break;
    }
    }

    // If we got a behavior, generate the points
    if( m_editBehavior )
    {
        wxCHECK( m_editPoints, /* void */ );
        m_editBehavior->MakePoints( *m_editPoints );
    }
}


EE_POINT_EDITOR::EE_POINT_EDITOR() :
    EE_TOOL_BASE<SCH_BASE_FRAME>( "eeschema.PointEditor" ),
    m_editedPoint( nullptr ),
    m_inPointEditor( false )
{
}


void EE_POINT_EDITOR::Reset( RESET_REASON aReason )
{
    EE_TOOL_BASE::Reset( aReason );

    m_editPoints.reset();
    m_editedPoint = nullptr;
}


bool EE_POINT_EDITOR::Init()
{
    EE_TOOL_BASE::Init();

    auto& menu = m_selectionTool->GetToolMenu().GetMenu();
    menu.AddItem( EE_ACTIONS::pointEditorAddCorner,
                              std::bind( &EE_POINT_EDITOR::addCornerCondition, this, _1 ) );
    menu.AddItem( EE_ACTIONS::pointEditorRemoveCorner,
                              std::bind( &EE_POINT_EDITOR::removeCornerCondition, this, _1 ) );

    return true;
}


int EE_POINT_EDITOR::clearEditedPoints( const TOOL_EVENT& aEvent )
{
    setEditedPoint( nullptr );

    return 0;
}


void EE_POINT_EDITOR::updateEditedPoint( const TOOL_EVENT& aEvent )
{
    EDIT_POINT* point = m_editedPoint;

    if( !m_editPoints )
    {
        point = nullptr;
    }
    else if( aEvent.IsMotion() )
    {
        point = m_editPoints->FindPoint( aEvent.Position(), getView() );
    }
    else if( aEvent.IsDrag( BUT_LEFT ) )
    {
        point = m_editPoints->FindPoint( aEvent.DragOrigin(), getView() );
    }
    else
    {
        point = m_editPoints->FindPoint( getViewControls()->GetCursorPosition( false ), getView() );
    }

    if( m_editedPoint != point )
        setEditedPoint( point );
}


int EE_POINT_EDITOR::Main( const TOOL_EVENT& aEvent )
{
    if( !m_selectionTool )
        return 0;

    if( m_inPointEditor )
        return 0;

    REENTRANCY_GUARD guard( &m_inPointEditor );

    if( m_isSymbolEditor )
    {
        SYMBOL_EDIT_FRAME* editor = getEditFrame<SYMBOL_EDIT_FRAME>();

        if( !editor->IsSymbolEditable() || editor->IsSymbolAlias() )
            return 0;
    }

    const EE_SELECTION& selection = m_selectionTool->GetSelection();

    if( selection.Size() != 1 || !selection.Front()->IsType( pointEditorTypes ) )
    {
        return 0;
    }

    // Wait till drawing tool is done
    if( selection.Front()->IsNew() )
        return 0;

    Activate();

    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    EE_GRID_HELPER*       grid = new EE_GRID_HELPER( m_toolMgr );
    VECTOR2I              cursorPos;
    KIGFX::VIEW*          view = getView();
    EDA_ITEM*             item = selection.Front();
    SCH_COMMIT            commit( m_toolMgr );

    controls->ShowCursor( true );

    makePointsAndBehavior( item );
    view->Add( m_editPoints.get() );
    setEditedPoint( nullptr );
    updateEditedPoint( aEvent );
    bool inDrag = false;

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        if( grid )
        {
            grid->SetSnap( !evt->Modifier( MD_SHIFT ) );
            grid->SetUseGrid( getView()->GetGAL()->GetGridSnapping()
                              && !evt->DisableGridSnapping() );
        }
        else
        {
            // This check is based on the assumption that the grid object must be valid.
            // If this assumption is wrong, please fix the code above.
            wxCHECK( false, 0 );
        }

        if( !m_editPoints || evt->IsSelectionEvent() )
            break;

        if ( !inDrag )
            updateEditedPoint( *evt );

        if( evt->IsDrag( BUT_LEFT ) && m_editedPoint )
        {
            if( !inDrag )
            {
                commit.Modify( m_editPoints->GetParent(), m_frame->GetScreen() );
                inDrag = true;
            }

            bool snap = !evt->DisableGridSnapping();

            cursorPos =
                    grid->Align( controls->GetMousePosition(), GRID_HELPER_GRIDS::GRID_GRAPHICS );
            controls->ForceCursorPosition( true, cursorPos );

            m_editedPoint->SetPosition( controls->GetCursorPosition( snap ) );

            updateParentItem( snap, commit );
            updatePoints();
        }
        else if( inDrag && evt->IsMouseUp( BUT_LEFT ) )
        {
            if( !commit.Empty() )
                commit.Push( _( "Move Point" ) );

            controls->SetAutoPan( false );
            inDrag = false;
        }
        else if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            if( inDrag )      // Restore the last change
            {
                // Currently we are manually managing the lifetime of the grid
                // helpers because there is a bug in the tool stack that adds
                // the point editor again when commit.Revert() rebuilds the selection.
                // We remove this grid here so the its destructor is called before it
                // is added again.
                if( grid )
                {
                    delete grid;
                    grid = nullptr;
                }

                commit.Revert();
                inDrag = false;
                break;
            }
            else if( evt->IsCancelInteractive() )
            {
                break;
            }

            if( evt->IsActivate() )
                break;
        }
        else
        {
            evt->SetPassEvent();
        }

        controls->SetAutoPan( inDrag );
        controls->CaptureCursor( inDrag );
    }

    controls->SetAutoPan( false );
    controls->CaptureCursor( false );
    setEditedPoint( nullptr );

    if( m_editPoints )
    {
        view->Remove( m_editPoints.get() );

        m_editPoints.reset();
        m_frame->GetCanvas()->Refresh();
    }

    delete grid;

    return 0;
}


void EE_POINT_EDITOR::updateParentItem( bool aSnapToGrid, SCH_COMMIT& aCommit ) const
{
    EDA_ITEM* item = m_editPoints->GetParent();

    if( !item )
        return;

    if( !m_editBehavior )
        return;

    std::vector<EDA_ITEM*> updatedItems;
    m_editBehavior->UpdateItem( *m_editedPoint, *m_editPoints, aCommit, updatedItems );

    for( EDA_ITEM* updatedItem : updatedItems )
    {
        updateItem( updatedItem, true );
    }
    m_frame->SetMsgPanel( item );
}


void EE_POINT_EDITOR::updatePoints()
{
    if( !m_editPoints || !m_editBehavior )
        return;

    m_editBehavior->UpdatePoints( *m_editPoints );
    getView()->Update( m_editPoints.get() );
}


void EE_POINT_EDITOR::setEditedPoint( EDIT_POINT* aPoint )
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();

    if( aPoint )
    {
        m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
        controls->ForceCursorPosition( true, aPoint->GetPosition() );
        controls->ShowCursor( true );
    }
    else
    {
        if( m_frame->ToolStackIsEmpty() )
            controls->ShowCursor( false );

        controls->ForceCursorPosition( false );
    }

    m_editedPoint = aPoint;
}


bool EE_POINT_EDITOR::removeCornerCondition( const SELECTION& )
{
    bool isRuleArea = false;

    if( m_editPoints )
        isRuleArea = m_editPoints->GetParent()->Type() == SCH_RULE_AREA_T;

    if( !m_editPoints || !m_editedPoint
        || !( m_editPoints->GetParent()->Type() == SCH_SHAPE_T || isRuleArea ) )
    {
        return false;
    }

    SCH_SHAPE* shape = static_cast<SCH_SHAPE*>( m_editPoints->GetParent() );

    if( shape->GetPolyShape().IsEmpty() )
        return false;

    SHAPE_LINE_CHAIN& poly = shape->GetPolyShape().Outline( 0 );

    if( m_editPoints->GetParent()->Type() == SCH_SHAPE_T && poly.GetPointCount() <= 2 )
        return false;
    if( m_editPoints->GetParent()->Type() == SCH_RULE_AREA_T && poly.GetPointCount() <= 3 )
        return false;

    for( const VECTOR2I& pt : poly.CPoints() )
    {
        if( pt == m_editedPoint->GetPosition() )
            return true;
    }

    return false;
}


bool EE_POINT_EDITOR::addCornerCondition( const SELECTION& )
{
    if( !m_editPoints
        || !( m_editPoints->GetParent()->Type() == SCH_SHAPE_T
              || m_editPoints->GetParent()->Type() == SCH_RULE_AREA_T ) )
    {
        return false;
    }

    SCH_SHAPE* shape = static_cast<SCH_SHAPE*>( m_editPoints->GetParent() );

    if( shape->GetShape() != SHAPE_T::POLY )
        return false;

    VECTOR2I cursorPos = getViewControls()->GetCursorPosition( false );
    double   threshold = getView()->ToWorld( EDIT_POINT::POINT_SIZE );

    return shape->HitTest( cursorPos, (int) threshold );
}


int EE_POINT_EDITOR::addCorner( const TOOL_EVENT& aEvent )
{
    if( !m_editPoints
        || !( m_editPoints->GetParent()->Type() == SCH_SHAPE_T
              || m_editPoints->GetParent()->Type() == SCH_RULE_AREA_T ) )
        return 0;

    SCH_SHAPE*        shape = static_cast<SCH_SHAPE*>( m_editPoints->GetParent() );
    SHAPE_LINE_CHAIN& poly = shape->GetPolyShape().Outline( 0 );
    SCH_COMMIT        commit( m_toolMgr );

    commit.Modify( shape, m_frame->GetScreen() );

    VECTOR2I cursor = getViewControls()->GetCursorPosition( !aEvent.DisableGridSnapping() );
    int      currentMinDistance = INT_MAX;
    int      closestLineStart = 0;
    unsigned numPoints = poly.GetPointCount();

    if( !shape->IsClosed() )
        numPoints -= 1;

    for( unsigned i = 0; i < numPoints; ++i )
    {
        SEG seg = poly.GetSegment( i );
        int distance = seg.Distance( cursor );

        if( distance < currentMinDistance )
        {
            currentMinDistance = distance;
            closestLineStart = i;
        }
    }

    poly.Insert( closestLineStart + 1, cursor );

    updateItem( shape, true );
    updatePoints();

    commit.Push( _( "Add Corner" ) );
    return 0;
}


int EE_POINT_EDITOR::removeCorner( const TOOL_EVENT& aEvent )
{
    if( !m_editPoints || !m_editedPoint
        || !m_editPoints->GetParent()->IsType( { SCH_SHAPE_T, SCH_RULE_AREA_T } ) )
    {
        return 0;
    }

    SCH_SHAPE*        shape = static_cast<SCH_SHAPE*>( m_editPoints->GetParent() );
    SHAPE_LINE_CHAIN& poly = shape->GetPolyShape().Outline( 0 );
    SCH_COMMIT        commit( m_toolMgr );

    if( m_editPoints->GetParent()->Type() == SCH_SHAPE_T && poly.GetPointCount() <= 2 )
        return 0;
    if( m_editPoints->GetParent()->Type() == SCH_RULE_AREA_T && poly.GetPointCount() <= 3 )
        return 0;

    commit.Modify( shape, m_frame->GetScreen() );

    int idx = getEditedPointIndex();
    int last = (int) poly.GetPointCount() - 1;

    if( idx == 0 && poly.GetPoint( 0 ) == poly.GetPoint( last ) )
    {
        poly.Remove( idx );
        poly.SetPoint( last-1, poly.GetPoint( 0 ) );
    }
    else
    {
        poly.Remove( idx );
    }

    setEditedPoint( nullptr );

    updateItem( shape, true );
    updatePoints();

    commit.Push( _( "Remove Corner" ) );
    return 0;
}


int EE_POINT_EDITOR::modifiedSelection( const TOOL_EVENT& aEvent )
{
    updatePoints();
    return 0;
}


void EE_POINT_EDITOR::setTransitions()
{
    Go( &EE_POINT_EDITOR::Main,              EVENTS::PointSelectedEvent );
    Go( &EE_POINT_EDITOR::Main,              EVENTS::SelectedEvent );
    Go( &EE_POINT_EDITOR::Main,              ACTIONS::activatePointEditor.MakeEvent() );
    Go( &EE_POINT_EDITOR::addCorner,         EE_ACTIONS::pointEditorAddCorner.MakeEvent() );
    Go( &EE_POINT_EDITOR::removeCorner,      EE_ACTIONS::pointEditorRemoveCorner.MakeEvent() );
    Go( &EE_POINT_EDITOR::modifiedSelection, EVENTS::SelectedItemsModified );
    Go( &EE_POINT_EDITOR::clearEditedPoints, EVENTS::ClearedEvent );
}


