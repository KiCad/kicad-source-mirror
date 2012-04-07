/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file base_struct.cpp
 * @brief Implementation of EDA_ITEM and EDA_TEXT base classes for KiCad.
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <trigo.h>
#include <common.h>
#include <macros.h>
#include <kicad_string.h>
#include <wxstruct.h>
#include <class_drawpanel.h>
#include <class_base_screen.h>
#include <drawtxt.h>

#include "../eeschema/dialogs/dialog_schematic_find.h"

enum textbox {
    ID_TEXTBOX_LIST = 8010
};


EDA_ITEM::EDA_ITEM( EDA_ITEM* parent, KICAD_T idType )
{
    InitVars();
    m_StructType = idType;
    m_Parent     = parent;
}


EDA_ITEM::EDA_ITEM( KICAD_T idType )
{
    InitVars();
    m_StructType = idType;
}


EDA_ITEM::EDA_ITEM( const EDA_ITEM& base )
{
    InitVars();
    m_StructType = base.m_StructType;
    m_Parent     = base.m_Parent;
    m_Son        = base.m_Son;
    m_Flags      = base.m_Flags;

    // A copy of an item cannot have the same time stamp as the original item.
    SetTimeStamp( GetNewTimeStamp() );
    m_Status     = base.m_Status;
}


void EDA_ITEM::InitVars()
{
    m_StructType = TYPE_NOT_INIT;
    Pnext       = NULL;     // Linked list: Link (next struct)
    Pback       = NULL;     // Linked list: Link (previous struct)
    m_Parent    = NULL;     // Linked list: Link (parent struct)
    m_Son       = NULL;     // Linked list: Link (son struct)
    m_List      = NULL;     // I am not on any list yet
    m_Image     = NULL;     // Link to an image copy for undelete or abort command
    m_Flags     = 0;        // flags for editions and other
    SetTimeStamp( 0 );      // Time stamp used for logical links
    m_Status    = 0;
    m_forceVisible = false; // true to override the visibility setting of the item.
}


void EDA_ITEM::SetModified()
{
    m_Flags |= IS_CHANGED;

    // If this a child object, then the parent modification state also needs to be set.
    if( m_Parent )
        m_Parent->SetModified();
}


EDA_ITEM* EDA_ITEM::Clone() const
{
    wxCHECK_MSG( false, NULL, wxT( "Clone not implemented in derived class " ) + GetClass() +
                 wxT( ".  Bad programmer!" ) );
}


SEARCH_RESULT EDA_ITEM::IterateForward( EDA_ITEM*     listStart,
                                        INSPECTOR*    inspector,
                                        const void*   testData,
                                        const KICAD_T scanTypes[] )
{
    EDA_ITEM* p = listStart;

    for( ; p; p = p->Pnext )
    {
        if( SEARCH_QUIT == p->Visit( inspector, testData, scanTypes ) )
            return SEARCH_QUIT;
    }

    return SEARCH_CONTINUE;
}


// see base_struct.h
// many classes inherit this method, be careful:
SEARCH_RESULT EDA_ITEM::Visit( INSPECTOR* inspector, const void* testData,
                               const KICAD_T scanTypes[] )
{
    KICAD_T stype;

#if 0 && defined(DEBUG)
    std::cout << GetClass().mb_str() << ' ';
#endif

    for( const KICAD_T* p = scanTypes;  (stype = *p) != EOT;   ++p )
    {
        // If caller wants to inspect my type
        if( stype == Type() )
        {
            if( SEARCH_QUIT == inspector->Inspect( this, testData ) )
                return SEARCH_QUIT;

            break;
        }
    }

    return SEARCH_CONTINUE;
}


wxString EDA_ITEM::GetSelectMenuText() const
{
    wxFAIL_MSG( wxT( "GetSelectMenuText() was not overridden for schematic item type " ) +
                GetClass() );

    return wxString( wxT( "Undefined menu text for " ) + GetClass() );
}


bool EDA_ITEM::Matches( const wxString& aText, wxFindReplaceData& aSearchData )
{
    wxString text = aText;
    wxString searchText = aSearchData.GetFindString();

    // Don't match if searching for replaceable item and the item doesn't support text replace.
    if( (aSearchData.GetFlags() & FR_SEARCH_REPLACE) && !IsReplaceable() )
        return false;

    if( aSearchData.GetFlags() & wxFR_WHOLEWORD )
        return aText.IsSameAs( searchText, aSearchData.GetFlags() & wxFR_MATCHCASE );

    if( aSearchData.GetFlags() & FR_MATCH_WILDCARD )
    {
        if( aSearchData.GetFlags() & wxFR_MATCHCASE )
            return text.Matches( searchText );

        return text.MakeUpper().Matches( searchText.MakeUpper() );
    }

    if( aSearchData.GetFlags() & wxFR_MATCHCASE )
        return aText.Find( searchText ) != wxNOT_FOUND;

    return text.MakeUpper().Find( searchText.MakeUpper() ) != wxNOT_FOUND;
}


bool EDA_ITEM::Replace( wxFindReplaceData& aSearchData, wxString& aText )
{
    wxCHECK_MSG( IsReplaceable(), false,
                 wxT( "Attempt to replace text in <" ) + GetClass() + wxT( "> item." ) );

    return aText.Replace( aSearchData.GetFindString(),
                          aSearchData.GetReplaceString(), false ) != 0;
}


bool EDA_ITEM::operator<( const EDA_ITEM& aItem ) const
{
    wxFAIL_MSG( wxString::Format( wxT( "Less than operator not defined for item type %s." ),
                                  GetChars( GetClass() ) ) );

    return false;
}


EDA_ITEM& EDA_ITEM::operator=( const EDA_ITEM& aItem )
{
    if( &aItem != this )
    {
        m_Image = aItem.m_Image;
        m_StructType = aItem.m_StructType;
        m_Parent = aItem.m_Parent;
        m_Son = aItem.m_Son;
        m_Flags = aItem.m_Flags;
        m_TimeStamp = aItem.m_TimeStamp;
        m_Status = aItem.m_Status;
        m_forceVisible = aItem.m_forceVisible;
    }

    return *this;
}


void EDA_ITEM::Format( OUTPUTFORMATTER* aFormatter, int aNestLevel, int aControlBits ) const
    throw( IO_ERROR )
{
    wxFAIL_MSG( wxString::Format( wxT( "Format method not defined for item type %s." ),
                                  GetChars( GetClass() ) ) );
}


#if defined(DEBUG)

// A function that should have been in wxWidgets
std::ostream& operator<<( std::ostream& out, const wxSize& size )
{
    out << " width=\"" << size.GetWidth() << "\" height=\"" << size.GetHeight() << "\"";
    return out;
}


// A function that should have been in wxWidgets
std::ostream& operator<<( std::ostream& out, const wxPoint& pt )
{
    out << " x=\"" << pt.x << "\" y=\"" << pt.y << "\"";
    return out;
}


void EDA_ITEM::ShowDummy( std::ostream& os ) const
{
    // XML output:
    wxString s = GetClass();

    os << '<' << s.Lower().mb_str() << ">"
       << " Need ::Show() override for this class "
       << "</" << s.Lower().mb_str() << ">\n";
}


std::ostream& EDA_ITEM::NestedSpace( int nestLevel, std::ostream& os )
{
    for( int i = 0; i<nestLevel; ++i )
        os << "  ";

    // number of spaces here controls indent per nest level

    return os;
}

#endif


/*******************************************/
/* EDA_TEXT (base class, not directly used */
/*******************************************/
EDA_TEXT::EDA_TEXT( const wxString& text )
{
    m_Size.x    = m_Size.y = DEFAULT_SIZE_TEXT;  // Width and height of font.
    m_Orient    = 0;                             // Rotation angle in 0.1 degrees.
    m_Attributs = 0;
    m_Mirror    = false;                         // display mirror if true
    m_HJustify  = GR_TEXT_HJUSTIFY_CENTER;       // Default horizontal justification is centered.
    m_VJustify  = GR_TEXT_VJUSTIFY_CENTER;       // Default vertical justification is centered.
    m_Thickness     = 0;                         // thickness
    m_Italic    = false;                         // true = italic shape.
    m_Bold      = false;
    m_MultilineAllowed = false;                  // Set to true for multiline text.
    m_Text = text;
}


EDA_TEXT::EDA_TEXT( const EDA_TEXT& aText )
{
    m_Pos = aText.m_Pos;
    m_Size = aText.m_Size;
    m_Orient = aText.m_Orient;
    m_Attributs = aText.m_Attributs;
    m_Mirror = aText.m_Mirror;
    m_HJustify = aText.m_HJustify;
    m_VJustify = aText.m_VJustify;
    m_Thickness = aText.m_Thickness;
    m_Italic = aText.m_Italic;
    m_Bold = aText.m_Bold;
    m_MultilineAllowed = aText.m_MultilineAllowed;
    m_Text = aText.m_Text;
}


EDA_TEXT::~EDA_TEXT()
{
}


int EDA_TEXT::LenSize( const wxString& aLine ) const
{
    return ReturnGraphicTextWidth( aLine, m_Size.x, m_Italic, m_Bold );
}


EDA_RECT EDA_TEXT::GetTextBox( int aLine, int aThickness, bool aInvertY ) const
{
    EDA_RECT       rect;
    wxPoint        pos;
    wxArrayString* list = NULL;
    wxString       text = m_Text;
    int            thickness = ( aThickness < 0 ) ? m_Thickness : aThickness;

    if( m_MultilineAllowed )
    {
        list = wxStringSplit( m_Text, '\n' );

        if ( list->GetCount() )     // GetCount() == 0 for void strings
        {
            if( aLine >= 0 && (aLine < (int)list->GetCount()) )
                text = list->Item( aLine );
            else
                text = list->Item( 0 );
        }
    }

    // calculate the H and V size
    int    dx = LenSize( text );
    int    dy = GetInterline();

    /* Creates bounding box (rectangle) for an horizontal text */
    wxSize textsize = wxSize( dx, dy );

    if( aInvertY )
        rect.SetOrigin( m_Pos.x, -m_Pos.y );
    else
        rect.SetOrigin( m_Pos );

    // extra dy interval for letters like j and y and ]
    int extra_dy = dy - m_Size.y;
    rect.Move( wxPoint( 0, -extra_dy / 2 ) ); // move origin by the half extra interval

    // for multiline texts and aLine < 0, merge all rectangles
    if( m_MultilineAllowed && list && aLine < 0 )
    {
        for( unsigned ii = 1; ii < list->GetCount(); ii++ )
        {
            text = list->Item( ii );
            dx   = LenSize( text );
            textsize.x  = MAX( textsize.x, dx );
            textsize.y += dy;
        }
    }

    delete list;

    rect.SetSize( textsize );

    /* Now, calculate the rect origin, according to text justification
     * At this point the rectangle origin is the text origin (m_Pos).
     * This is true only for left and top text justified texts (using top to bottom Y axis
     * orientation). and must be recalculated for others justifications
     * also, note the V justification is relative to the first line
     */
    switch( m_HJustify )
    {
    case GR_TEXT_HJUSTIFY_LEFT:
        if( m_Mirror )
            rect.SetX( rect.GetX() - rect.GetWidth() );
        break;

    case GR_TEXT_HJUSTIFY_CENTER:
        rect.SetX( rect.GetX() - (rect.GetWidth() / 2) );
        break;

    case GR_TEXT_HJUSTIFY_RIGHT:
        if( !m_Mirror )
            rect.SetX( rect.GetX() - rect.GetWidth() );
        break;
    }

    dy = m_Size.y + thickness;

    switch( m_VJustify )
    {
    case GR_TEXT_VJUSTIFY_TOP:
        break;

    case GR_TEXT_VJUSTIFY_CENTER:
        rect.SetY( rect.GetY() - (dy / 2) );
        break;

    case GR_TEXT_VJUSTIFY_BOTTOM:
        rect.SetY( rect.GetY() - dy );
        break;
    }

    rect.Inflate( thickness / 2 );
    rect.Normalize();       // Make h and v sizes always >= 0

    return rect;
}


bool EDA_TEXT::TextHitTest( const wxPoint& aPoint, int aAccuracy ) const
{
    EDA_RECT rect = GetTextBox( -1 );   // Get the full text area.
    wxPoint location = aPoint;

    rect.Inflate( aAccuracy );
    RotatePoint( &location, m_Pos, -m_Orient );

    return rect.Contains( location );
}


bool EDA_TEXT::TextHitTest( const EDA_RECT& aRect, bool aContains, int aAccuracy ) const
{
    EDA_RECT rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContains )
        return rect.Contains( GetTextBox( -1 ) );

    return rect.Intersects( GetTextBox( -1 ) );
}


void EDA_TEXT::Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aOffset,
                     EDA_COLOR_T aColor, int aDrawMode,
                     EDA_DRAW_MODE_T aFillMode, EDA_COLOR_T aAnchor_color )
{
    if( m_MultilineAllowed )
    {
        wxPoint        pos  = m_Pos;
        wxArrayString* list = wxStringSplit( m_Text, '\n' );
        wxPoint        offset;

        offset.y = GetInterline();

        RotatePoint( &offset, m_Orient );

        for( unsigned i = 0; i<list->Count(); i++ )
        {
            wxString txt = list->Item( i );
            DrawOneLineOfText( aPanel,
                               aDC,
                               aOffset,
                               aColor,
                               aDrawMode,
                               aFillMode,
                               i ?  UNSPECIFIED : aAnchor_color,
                               txt,
                               pos );
            pos += offset;
        }

        delete (list);
    }
    else
        DrawOneLineOfText( aPanel,
                           aDC,
                           aOffset,
                           aColor,
                           aDrawMode,
                           aFillMode,
                           aAnchor_color,
                           m_Text,
                           m_Pos );
}


void EDA_TEXT::DrawOneLineOfText( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
                                  const wxPoint& aOffset, EDA_COLOR_T aColor,
                                  int aDrawMode, EDA_DRAW_MODE_T aFillMode,
                                  EDA_COLOR_T aAnchor_color,
                                  wxString& aText, wxPoint aPos )
{
    int width = m_Thickness;

    if( aFillMode == LINE )
        width = 0;

    if( aDrawMode != -1 )
        GRSetDrawMode( aDC, aDrawMode );

    /* Draw text anchor, if allowed */
    if( aAnchor_color != UNSPECIFIED )
    {

        int anchor_size = aDC->DeviceToLogicalXRel( 2 );

        aAnchor_color = (EDA_COLOR_T) ( aAnchor_color & MASKCOLOR );

        int cX = aPos.x + aOffset.x;
        int cY = aPos.y + aOffset.y;

        GRLine( aPanel->GetClipBox(), aDC, cX - anchor_size, cY,
                cX + anchor_size, cY, 0, aAnchor_color );

        GRLine( aPanel->GetClipBox(), aDC, cX, cY - anchor_size,
                cX, cY + anchor_size, 0, aAnchor_color );
    }

    if( aFillMode == SKETCH )
        width = -width;

    wxSize size = m_Size;

    if( m_Mirror )
        size.x = -size.x;

    DrawGraphicText( aPanel, aDC, aOffset + aPos, aColor, aText, m_Orient, size,
                     m_HJustify, m_VJustify, width, m_Italic, m_Bold );
}


wxString EDA_TEXT::GetTextStyleName()
{
    int style = 0;

    if( m_Italic )
        style = 1;

    if( m_Bold )
        style += 2;

    wxString stylemsg[4] = {
        _("Normal"),
        _("Italic"),
        _("Bold"),
        _("Bold+Italic")
    };

    return stylemsg[style];
}


bool EDA_TEXT::IsDefaultFormatting() const
{
    return (  ( m_Size.x == DEFAULT_SIZE_TEXT )
           && ( m_Size.y == DEFAULT_SIZE_TEXT )
           && ( m_Attributs == 0 )
           && ( m_Mirror == false )
           && ( m_HJustify == GR_TEXT_HJUSTIFY_CENTER )
           && ( m_VJustify == GR_TEXT_VJUSTIFY_CENTER )
           && ( m_Thickness == 0 )
           && ( m_Italic == false )
           && ( m_Bold == false )
           && ( m_MultilineAllowed == false ) );
}


void EDA_TEXT::Format( OUTPUTFORMATTER* aFormatter, int aNestLevel, int aControlBits ) const
    throw( IO_ERROR )
{
    aFormatter->Print( aNestLevel, "(text %s (at %s",
                       aFormatter->Quotew( m_Text ).c_str(), FormatBIU( m_Pos ).c_str() );

    if( m_Orient != 0.0 )
        aFormatter->Print( 0, " %0.1f", m_Orient );

    aFormatter->Print( 0, ")\n" );

    if( !IsDefaultFormatting() )
    {
        aFormatter->Print( aNestLevel+1, "(effects\n" );

        if( ( m_Size.x != DEFAULT_SIZE_TEXT ) || ( m_Size.y != DEFAULT_SIZE_TEXT ) || m_Bold
          || m_Italic )
        {
            aFormatter->Print( aNestLevel+2, "(font" );

            // Add font support here at some point in the future.

            if( ( m_Size.x != DEFAULT_SIZE_TEXT ) || ( m_Size.y != DEFAULT_SIZE_TEXT ) )
                aFormatter->Print( 0, " (size %s)", FormatBIU( m_Size ).c_str() );

            if( m_Bold )
                aFormatter->Print( 0, " bold" );

            if( m_Bold )
                aFormatter->Print( 0, " italic" );

            aFormatter->Print( 0, ")\n");
        }

        if( m_Mirror || ( m_HJustify != GR_TEXT_HJUSTIFY_CENTER )
          || ( m_VJustify != GR_TEXT_VJUSTIFY_CENTER ) )
        {
            aFormatter->Print( aNestLevel+2, "(justify");

            if( m_HJustify != GR_TEXT_HJUSTIFY_CENTER )
                aFormatter->Print( 0, (m_HJustify == GR_TEXT_HJUSTIFY_LEFT) ? " left" : " right" );

            if( m_VJustify != GR_TEXT_VJUSTIFY_CENTER )
                aFormatter->Print( 0, (m_VJustify == GR_TEXT_VJUSTIFY_TOP) ? " top" : " bottom" );

            if( m_Mirror )
                aFormatter->Print( 0, " mirror" );

            aFormatter->Print( 0, ")\n" );
        }

        // As of now the only place this is used is in Eeschema to hide or show the text.
        if( m_Attributs )
            aFormatter->Print( aNestLevel+2, "hide\n" );

        aFormatter->Print( aNestLevel+1, ")\n" );
    }

    aFormatter->Print( aNestLevel, ")\n" );
}


/******************/
/* Class EDA_RECT */
/******************/

void EDA_RECT::Normalize()
{
    if( m_Size.y < 0 )
    {
        m_Size.y = -m_Size.y;
        m_Pos.y -= m_Size.y;
    }

    if( m_Size.x < 0 )
    {
        m_Size.x = -m_Size.x;
        m_Pos.x -= m_Size.x;
    }
}


void EDA_RECT::Move( const wxPoint& aMoveVector )
{
    m_Pos += aMoveVector;
}


bool EDA_RECT::Contains( const wxPoint& aPoint ) const
{
    wxPoint rel_pos = aPoint - m_Pos;
    wxSize size     = m_Size;

    if( size.x < 0 )
    {
        size.x    = -size.x;
        rel_pos.x += size.x;
    }

    if( size.y < 0 )
    {
        size.y    = -size.y;
        rel_pos.y += size.y;
    }

    return (rel_pos.x >= 0) && (rel_pos.y >= 0) && ( rel_pos.y <= size.y) && ( rel_pos.x <= size.x);
}


/*
 * return true if aRect is inside me (or on boundaries)
 */
bool EDA_RECT::Contains( const EDA_RECT& aRect ) const
{
    return Contains( aRect.GetOrigin() ) && Contains( aRect.GetEnd() );
}


/* Intersects
 * test for a common area between 2 rect.
 * return true if at least a common point is found
 */
bool EDA_RECT::Intersects( const EDA_RECT& aRect ) const
{
    // this logic taken from wxWidgets' geometry.cpp file:
    bool rc;
    EDA_RECT me(*this);
    EDA_RECT rect(aRect);
    me.Normalize();         // ensure size is >= 0
    rect.Normalize();       // ensure size is >= 0

    // calculate the left common area coordinate:
    int  left   = MAX( me.m_Pos.x, rect.m_Pos.x );
    // calculate the right common area coordinate:
    int  right  = MIN( me.m_Pos.x + m_Size.x, rect.m_Pos.x + rect.m_Size.x );
    // calculate the upper common area coordinate:
    int  top    = MAX( me.m_Pos.y, aRect.m_Pos.y );
    // calculate the lower common area coordinate:
    int  bottom = MIN( me.m_Pos.y + m_Size.y, rect.m_Pos.y + rect.m_Size.y );

    // if a common area exists, it must have a positive (null accepted) size
    if( left <= right && top <= bottom )
        rc = true;
    else
        rc = false;

    return rc;
}


EDA_RECT& EDA_RECT::Inflate( int aDelta )
{
    Inflate( aDelta, aDelta );
    return *this;
}


EDA_RECT& EDA_RECT::Inflate( wxCoord dx, wxCoord dy )
{
    if( m_Size.x >= 0 )
    {
        if( m_Size.x < -2 * dx )
        {
            // Don't allow deflate to eat more width than we have,
            m_Pos.x += m_Size.x / 2;
            m_Size.x = 0;
        }
        else
        {
            // The inflate is valid.
            m_Pos.x  -= dx;
            m_Size.x += 2 * dx;
        }
    }
    else    // size.x < 0:
    {
        if( m_Size.x > -2 * dx )
        {
            // Don't allow deflate to eat more width than we have,
            m_Pos.x -= m_Size.x / 2;
            m_Size.x = 0;
        }
        else
        {
            // The inflate is valid.
            m_Pos.x  += dx;
            m_Size.x -= 2 * dx; // m_Size.x <0: inflate when dx > 0
        }
    }

    if( m_Size.y >= 0 )
    {
        if( m_Size.y < -2 * dy )
        {
            // Don't allow deflate to eat more height than we have,
            m_Pos.y += m_Size.y / 2;
            m_Size.y = 0;
        }
        else
        {
            // The inflate is valid.
            m_Pos.y  -= dy;
            m_Size.y += 2 * dy;
        }
    }
    else    // size.y < 0:
    {
        if( m_Size.y > 2 * dy )
        {
            // Don't allow deflate to eat more height than we have,
            m_Pos.y -= m_Size.y / 2;
            m_Size.y = 0;
        }
        else
        {
            // The inflate is valid.
            m_Pos.y  += dy;
            m_Size.y -= 2 * dy; // m_Size.y <0: inflate when dy > 0
        }
    }

    return *this;
}


void EDA_RECT::Merge( const EDA_RECT& aRect )
{
    Normalize();        // ensure width and height >= 0
    EDA_RECT rect = aRect;
    rect.Normalize();   // ensure width and height >= 0
    wxPoint  end = GetEnd();
    wxPoint  rect_end = rect.GetEnd();

    // Change origin and size in order to contain the given rect
    m_Pos.x = MIN( m_Pos.x, rect.m_Pos.x );
    m_Pos.y = MIN( m_Pos.y, rect.m_Pos.y );
    end.x   = MAX( end.x, rect_end.x );
    end.y   = MAX( end.y, rect_end.y );
    SetEnd( end );
}


void EDA_RECT::Merge( const wxPoint& aPoint )
{
    Normalize();        // ensure width and height >= 0

    wxPoint  end = GetEnd();
    // Change origin and size in order to contain the given rect
    m_Pos.x = MIN( m_Pos.x, aPoint.x );
    m_Pos.y = MIN( m_Pos.y, aPoint.y );
    end.x   = MAX( end.x, aPoint.x );
    end.y   = MAX( end.y, aPoint.y );
    SetEnd( end );
}


double EDA_RECT::GetArea() const
{
    return (double) GetWidth() * (double) GetHeight();
}
