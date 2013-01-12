/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file class_dimension.cpp
 */

#include <fctsys.h>
#include <macros.h>
#include <gr_basic.h>
#include <pcbcommon.h>
#include <trigo.h>
#include <wxstruct.h>
#include <class_drawpanel.h>
#include <colors_selection.h>
#include <kicad_string.h>
#include <richio.h>

#include <class_board.h>
#include <class_pcb_text.h>
#include <class_dimension.h>
#include <base_units.h>


DIMENSION::DIMENSION( BOARD_ITEM* aParent ) :
    BOARD_ITEM( aParent, PCB_DIMENSION_T ),
    m_Text( this )
{
    m_Layer = DRAW_LAYER;
    m_Width = Millimeter2iu( 0.2 );
    m_Value = 0;
    m_Shape = 0;
    m_Unit  = INCHES;
}


DIMENSION::~DIMENSION()
{
}


void DIMENSION::SetPosition( const wxPoint& aPos )
{
    m_Text.SetPos( aPos );
}


const wxPoint& DIMENSION::GetPosition() const
{
    return m_Text.GetPosition();
}


void DIMENSION::SetText( const wxString& aNewText )
{
    m_Text.SetText( aNewText );
}


const wxString DIMENSION::GetText() const
{
    return m_Text.GetText();
}


void DIMENSION::SetLayer( int aLayer )
{
    m_Layer = aLayer;
    m_Text.SetLayer( aLayer );
}


void DIMENSION::Copy( DIMENSION* source )
{
    m_Value = source->m_Value;
    SetLayer( source->GetLayer() );
    m_Width = source->m_Width;
    m_Shape = source->m_Shape;
    m_Unit  = source->m_Unit;
    SetTimeStamp( GetNewTimeStamp() );
    m_Text.Copy( &source->m_Text );

    m_crossBarO     = source->m_crossBarO;
    m_crossBarF     = source->m_crossBarF;
    m_featureLineGO = source->m_featureLineGO;
    m_featureLineGF = source->m_featureLineGF;
    m_featureLineDO = source->m_featureLineDO;
    m_featureLineDF = source->m_featureLineDF;
    m_arrowD1O  = source->m_arrowD1O;
    m_arrowD1F  = source->m_arrowD1F;
    m_arrowD2O  = source->m_arrowD2O;
    m_arrowD2F  = source->m_arrowD2F;
    m_arrowG1O  = source->m_arrowG1O;
    m_arrowG1F  = source->m_arrowG1F;
    m_arrowG2O  = source->m_arrowG2O;
    m_arrowG2F  = source->m_arrowG2F;
}


void DIMENSION::Move( const wxPoint& offset )
{
    m_Text.m_Pos    += offset;
    m_crossBarO     += offset;
    m_crossBarF     += offset;
    m_featureLineGO += offset;
    m_featureLineGF += offset;
    m_featureLineDO += offset;
    m_featureLineDF += offset;
    m_arrowG1O  += offset;
    m_arrowG1F  += offset;
    m_arrowG2O  += offset;
    m_arrowG2F  += offset;
    m_arrowD1O  += offset;
    m_arrowD1F  += offset;
    m_arrowD2O  += offset;
    m_arrowD2F  += offset;
}


void DIMENSION::Rotate( const wxPoint& aRotCentre, double aAngle )
{
    RotatePoint( &m_Text.m_Pos, aRotCentre, aAngle );

    double newAngle = m_Text.GetOrientation() + aAngle;

    if( newAngle >= 3600 )
        newAngle -= 3600;

    if( newAngle > 900  &&  newAngle < 2700 )
        newAngle -= 1800;

    m_Text.SetOrientation( newAngle );

    RotatePoint( &m_crossBarO, aRotCentre, aAngle );
    RotatePoint( &m_crossBarF, aRotCentre, aAngle );
    RotatePoint( &m_featureLineGO, aRotCentre, aAngle );
    RotatePoint( &m_featureLineGF, aRotCentre, aAngle );
    RotatePoint( &m_featureLineDO, aRotCentre, aAngle );
    RotatePoint( &m_featureLineDF, aRotCentre, aAngle );
    RotatePoint( &m_arrowG1O, aRotCentre, aAngle );
    RotatePoint( &m_arrowG1F, aRotCentre, aAngle );
    RotatePoint( &m_arrowG2O, aRotCentre, aAngle );
    RotatePoint( &m_arrowG2F, aRotCentre, aAngle );
    RotatePoint( &m_arrowD1O, aRotCentre, aAngle );
    RotatePoint( &m_arrowD1F, aRotCentre, aAngle );
    RotatePoint( &m_arrowD2O, aRotCentre, aAngle );
    RotatePoint( &m_arrowD2F, aRotCentre, aAngle );
}


void DIMENSION::Flip( const wxPoint& aCentre )
{
    Mirror( aCentre );
    SetLayer( BOARD::ReturnFlippedLayerNumber( GetLayer() ) );
}


void DIMENSION::Mirror( const wxPoint& axis_pos )
{
#define INVERT( pos ) (pos) = axis_pos.y - ( (pos) - axis_pos.y )
    INVERT( m_Text.m_Pos.y );

    // invert angle
    double newAngle = m_Text.GetOrientation();

    if( newAngle >= 3600 )
        newAngle -= 3600;

    if( newAngle > 900  &&  newAngle < 2700 )
        newAngle -= 1800;

    m_Text.SetOrientation( newAngle );

    INVERT( m_crossBarO.y );
    INVERT( m_crossBarF.y );
    INVERT( m_featureLineGO.y );
    INVERT( m_featureLineGF.y );
    INVERT( m_featureLineDO.y );
    INVERT( m_featureLineDF.y );
    INVERT( m_arrowG1O.y );
    INVERT( m_arrowG1F.y );
    INVERT( m_arrowG2O.y );
    INVERT( m_arrowG2F.y );
    INVERT( m_arrowD1O.y );
    INVERT( m_arrowD1F.y );
    INVERT( m_arrowD2O.y );
    INVERT( m_arrowD2F.y );
}


void DIMENSION::AdjustDimensionDetails( bool aDoNotChangeText )
{
    const int   arrowz = DMils2iu( 500 );           // size of arrows
    int         ii;
    int         measure, deltax, deltay;            // value of the measure on X and Y axes
    int         arrow_up_X  = 0, arrow_up_Y = 0;    // coordinates of arrow line /
    int         arrow_dw_X  = 0, arrow_dw_Y = 0;    // coordinates of arrow line '\'
    int         hx, hy;                             // dimension line interval
    double      angle, angle_f;
    wxString    msg;

    // Init layer :
    m_Text.SetLayer( GetLayer() );

    // calculate the size of the dimension (text + line above the text)
    ii = m_Text.m_Size.y +
         m_Text.GetThickness() + (m_Width * 3);

    deltax  = m_featureLineDO.x - m_featureLineGO.x;
    deltay  = m_featureLineDO.y - m_featureLineGO.y;

    // Calculate dimension value
    measure = KiROUND( hypot( (double) deltax, (double) deltay ) );

    if( deltax || deltay )
        angle = atan2( (double) deltay, (double) deltax );
    else
        angle = 0.0;

    // Calculation of parameters X and Y dimensions of the arrows and lines.
    hx = hy = ii;

    // Taking into account the slope of the side lines.
    if( measure )
    {
        hx  = (abs) ( (int) ( ( (double) deltay * hx ) / measure ) );
        hy  = (abs) ( (int) ( ( (double) deltax * hy ) / measure ) );

        if( m_featureLineGO.x > m_crossBarO.x )
            hx = -hx;

        if( m_featureLineGO.x == m_crossBarO.x )
            hx = 0;

        if( m_featureLineGO.y > m_crossBarO.y )
            hy = -hy;

        if( m_featureLineGO.y == m_crossBarO.y )
            hy = 0;

        angle_f     = angle + (M_PI * 27.5 / 180);
        arrow_up_X  = (int) ( arrowz * cos( angle_f ) );
        arrow_up_Y  = (int) ( arrowz * sin( angle_f ) );
        angle_f     = angle - (M_PI * 27.5 / 180);
        arrow_dw_X  = (int) ( arrowz * cos( angle_f ) );
        arrow_dw_Y  = (int) ( arrowz * sin( angle_f ) );
    }

    m_arrowG1O.x    = m_crossBarO.x;
    m_arrowG1O.y    = m_crossBarO.y;
    m_arrowG1F.x    = m_crossBarO.x + arrow_up_X;
    m_arrowG1F.y    = m_crossBarO.y + arrow_up_Y;

    m_arrowG2O.x    = m_crossBarO.x;
    m_arrowG2O.y    = m_crossBarO.y;
    m_arrowG2F.x    = m_crossBarO.x + arrow_dw_X;
    m_arrowG2F.y    = m_crossBarO.y + arrow_dw_Y;

    /* The right arrow is symmetrical to the left.
     *  / = -\  and  \ = -/
     */
    m_arrowD1O.x    = m_crossBarF.x;
    m_arrowD1O.y    = m_crossBarF.y;
    m_arrowD1F.x    = m_crossBarF.x - arrow_dw_X;
    m_arrowD1F.y    = m_crossBarF.y - arrow_dw_Y;

    m_arrowD2O.x    = m_crossBarF.x;
    m_arrowD2O.y    = m_crossBarF.y;
    m_arrowD2F.x    = m_crossBarF.x - arrow_up_X;
    m_arrowD2F.y    = m_crossBarF.y - arrow_up_Y;


    m_featureLineGF.x   = m_crossBarO.x + hx;
    m_featureLineGF.y   = m_crossBarO.y + hy;

    m_featureLineDF.x   = m_crossBarF.x + hx;
    m_featureLineDF.y   = m_crossBarF.y + hy;

    // Calculate the better text position and orientation:
    m_Text.m_Pos.x  = (m_crossBarF.x + m_featureLineGF.x) / 2;
    m_Text.m_Pos.y  = (m_crossBarF.y + m_featureLineGF.y) / 2;

    double newAngle = -(angle * 1800 / M_PI);

    if( newAngle < 0 )
        newAngle += 3600;

    if( newAngle >= 3600 )
        newAngle -= 3600;

    if( newAngle > 900  &&  newAngle < 2700 )
        newAngle -= 1800;

    m_Text.SetOrientation( newAngle );

    if( !aDoNotChangeText )
    {
        m_Value = measure;
        msg     = ::CoordinateToString( m_Value );
        SetText( msg );
    }
}


void DIMENSION::Draw( EDA_DRAW_PANEL* panel, wxDC* DC, GR_DRAWMODE mode_color,
                      const wxPoint& offset )
{
    int         typeaff, width;
    EDA_COLOR_T gcolor;
    BOARD*      brd = GetBoard();

    if( brd->IsLayerVisible( m_Layer ) == false )
        return;

    m_Text.Draw( panel, DC, mode_color, offset );

    gcolor = brd->GetLayerColor( m_Layer );

    GRSetDrawMode( DC, mode_color );
    typeaff = DisplayOpt.DisplayDrawItems;
    width   = m_Width;

    if( DC->LogicalToDeviceXRel( width ) < 2 )
        typeaff = LINE;

    switch( typeaff )
    {
    case LINE:
        width = 0;

    case FILLED:
        GRLine( panel->GetClipBox(), DC, m_crossBarO + offset,
                m_crossBarF + offset, width, gcolor );
        GRLine( panel->GetClipBox(), DC, m_featureLineGO + offset,
                m_featureLineGF + offset, width, gcolor );
        GRLine( panel->GetClipBox(), DC, m_featureLineDO + offset,
                m_featureLineDF + offset, width, gcolor );
        GRLine( panel->GetClipBox(), DC, m_arrowD1O + offset,
                m_arrowD1F + offset, width, gcolor );
        GRLine( panel->GetClipBox(), DC, m_arrowD2O + offset,
                m_arrowD2F + offset, width, gcolor );
        GRLine( panel->GetClipBox(), DC, m_arrowG1O + offset,
                m_arrowG1F + offset, width, gcolor );
        GRLine( panel->GetClipBox(), DC, m_arrowG2O + offset,
                m_arrowG2F + offset, width, gcolor );
        break;

    case SKETCH:
        GRCSegm( panel->GetClipBox(), DC, m_crossBarO + offset,
                 m_crossBarF + offset, width, gcolor );
        GRCSegm( panel->GetClipBox(), DC, m_featureLineGO + offset,
                 m_featureLineGF + offset, width, gcolor );
        GRCSegm( panel->GetClipBox(), DC, m_featureLineDO + offset,
                 m_featureLineDF + offset, width, gcolor );
        GRCSegm( panel->GetClipBox(), DC, m_arrowD1O + offset,
                 m_arrowD1F + offset, width, gcolor );
        GRCSegm( panel->GetClipBox(), DC, m_arrowD2O + offset,
                 m_arrowD2F + offset, width, gcolor );
        GRCSegm( panel->GetClipBox(), DC, m_arrowG1O + offset,
                 m_arrowG1F + offset, width, gcolor );
        GRCSegm( panel->GetClipBox(), DC, m_arrowG2O + offset,
                 m_arrowG2F + offset, width, gcolor );
        break;
    }
}


// see class_cotation.h
void DIMENSION::GetMsgPanelInfo( std::vector< MSG_PANEL_ITEM >& aList )
{
    // for now, display only the text within the DIMENSION using class TEXTE_PCB.
    m_Text.GetMsgPanelInfo( aList );
}


bool DIMENSION::HitTest( const wxPoint& aPosition )
{
    if( m_Text.TextHitTest( aPosition ) )
        return true;

    int dist_max = m_Width / 2;

    // Locate SEGMENTS

    if( TestSegmentHit( aPosition, m_crossBarO, m_crossBarF, dist_max ) )
        return true;

    if( TestSegmentHit( aPosition, m_featureLineGO, m_featureLineGF, dist_max ) )
        return true;

    if( TestSegmentHit( aPosition, m_featureLineDO, m_featureLineDF, dist_max ) )
        return true;

    if( TestSegmentHit( aPosition, m_arrowD1O, m_arrowD1F, dist_max ) )
        return true;

    if( TestSegmentHit( aPosition, m_arrowD2O, m_arrowD2F, dist_max ) )
        return true;

    if( TestSegmentHit( aPosition, m_arrowG1O, m_arrowG1F, dist_max ) )
        return true;

    if( TestSegmentHit( aPosition, m_arrowG2O, m_arrowG2F, dist_max ) )
        return true;

    return false;
}


bool DIMENSION::HitTest( const EDA_RECT& aRect ) const
{
    if( aRect.Contains( GetPosition() ) )
        return true;

    return false;
}


EDA_RECT DIMENSION::GetBoundingBox() const
{
    EDA_RECT    bBox;
    int         xmin, xmax, ymin, ymax;

    bBox    = m_Text.GetTextBox( -1 );
    xmin    = bBox.GetX();
    xmax    = bBox.GetRight();
    ymin    = bBox.GetY();
    ymax    = bBox.GetBottom();

    xmin    = std::min( xmin, m_crossBarO.x );
    xmin    = std::min( xmin, m_crossBarF.x );
    ymin    = std::min( ymin, m_crossBarO.y );
    ymin    = std::min( ymin, m_crossBarF.y );
    xmax    = std::max( xmax, m_crossBarO.x );
    xmax    = std::max( xmax, m_crossBarF.x );
    ymax    = std::max( ymax, m_crossBarO.y );
    ymax    = std::max( ymax, m_crossBarF.y );

    xmin    = std::min( xmin, m_featureLineGO.x );
    xmin    = std::min( xmin, m_featureLineGF.x );
    ymin    = std::min( ymin, m_featureLineGO.y );
    ymin    = std::min( ymin, m_featureLineGF.y );
    xmax    = std::max( xmax, m_featureLineGO.x );
    xmax    = std::max( xmax, m_featureLineGF.x );
    ymax    = std::max( ymax, m_featureLineGO.y );
    ymax    = std::max( ymax, m_featureLineGF.y );

    bBox.SetX( xmin );
    bBox.SetY( ymin );
    bBox.SetWidth( xmax - xmin + 1 );
    bBox.SetHeight( ymax - ymin + 1 );

    bBox.Normalize();

    return bBox;
}


wxString DIMENSION::GetSelectMenuText() const
{
    wxString text;

    text << _( "Dimension" ) << wxT( " \"" ) << GetText() << wxT( "\"" );

    return text;
}


EDA_ITEM* DIMENSION::Clone() const
{
    return new DIMENSION( *this );
}
