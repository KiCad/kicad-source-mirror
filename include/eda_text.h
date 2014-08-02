/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jpe.charras at wanadoo.fr
 * Copyright (C) 2004-2013 KiCad Developers, see change_log.txt for contributors.
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
 * @file eda_text.h
 * @brief Definition of base KiCad text object.
 */

#ifndef EDA_TEXT_H_
#define EDA_TEXT_H_

#include <trigo.h>                  // NORMALIZE_ANGLE_POS( angle );
#include <common.h>                 // wxStringSplit
#include <gr_basic.h>               // EDA_DRAW_MODE_T
#include <base_struct.h>            // EDA_RECT


// Graphic Text justify:
// Values -1,0,1 are used in computations, do not change them
enum EDA_TEXT_HJUSTIFY_T {
    GR_TEXT_HJUSTIFY_LEFT   = -1,
    GR_TEXT_HJUSTIFY_CENTER = 0,
    GR_TEXT_HJUSTIFY_RIGHT  = 1
};


enum EDA_TEXT_VJUSTIFY_T {
    GR_TEXT_VJUSTIFY_TOP    = -1,
    GR_TEXT_VJUSTIFY_CENTER = 0,
    GR_TEXT_VJUSTIFY_BOTTOM = 1
};


/* Options to show solid segments (segments, texts...) */
enum EDA_DRAW_MODE_T {
    LINE = 0,           // segments are drawn as lines
    FILLED,             // normal mode: segments have thickness
    SKETCH              // sketch mode: segments have thickness, but are not filled
};


/** This is the "default-of-the-default" hardcoded text size; individual
 * application define their own default policy starting with this
 * (usually with a user option or project). DO NOT change this value if
 * you do not fully realize the effect it has on sexp serialization
 * (text size equal to this is not explicitly wrote, so it would change
 * subsequent reads) */
#define DEFAULT_SIZE_TEXT 60    // default text height (in mils, i.e. 1/1000")
#define TEXT_NO_VISIBLE   1     //< EDA_TEXT::m_Attribut(e?) visibility flag.
#define DIM_ANCRE_TEXTE  2      // Anchor size for text


/**
 * Class EDA_TEXT
 * is a basic class to handle texts (labels, texts on components or footprints
 * ..) not used directly. The "used" text classes are derived from EDA_ITEM and
 * EDA_TEXT using multiple inheritance.
 */
class EDA_TEXT
{
protected:
    wxString            m_Text;
    int                 m_Thickness;        ///< pen size used to draw this text
    double              m_Orient;           ///< Orient in 0.1 degrees
    wxPoint             m_Pos;              ///< XY position of anchor text.
    wxSize              m_Size;             ///< XY size of text
    bool                m_Mirror;           ///< true if mirrored
    int                 m_Attributs;        ///< bit flags such as visible, etc.
    bool                m_Italic;           ///< should be italic font (if available)
    bool                m_Bold;             ///< should be bold font (if available)
    EDA_TEXT_HJUSTIFY_T m_HJustify;         ///< horizontal justification
    EDA_TEXT_VJUSTIFY_T m_VJustify;         ///< vertical justification
    bool                m_MultilineAllowed; /**< true to use multiline option, false
                                             * to use only single line text
                                             * Single line is faster in
                                             * calculations than multiline */

public:
    EDA_TEXT( const wxString& text = wxEmptyString );
    EDA_TEXT( const EDA_TEXT& aText );
    virtual ~EDA_TEXT();

    /**
     * Function GetText
     * returns the string associated with the text object.
     * <p>
     * This function is virtual to allow derived classes to override getting the
     * string to provide a way for modifying the base string by adding a suffix or
     * prefix to the base string.
     * </p>
     * @return a const wxString object containing the string of the item.
     */
    virtual const wxString GetText() const { return m_Text; }

    virtual void SetText( const wxString& aText ) { m_Text = aText; }

    /**
     * Function SetThickness
     * sets text thickness.
     * @param aNewThickness is the new text thickness.
     */
    void SetThickness( int aNewThickness ) { m_Thickness = aNewThickness; };

    /**
     * Function GetThickness
     * returns text thickness.
     * @return int - text thickness.
     */
    int GetThickness() const               { return m_Thickness; };

    void SetOrientation( double aOrientation )
    {
        NORMALIZE_ANGLE_POS( aOrientation );
        m_Orient = aOrientation;
    }
    double GetOrientation() const          { return m_Orient; }

    void SetItalic( bool isItalic )        { m_Italic = isItalic; }
    bool IsItalic() const                  { return m_Italic; }

    void SetBold( bool aBold )             { m_Bold = aBold; }
    bool IsBold() const                    { return m_Bold; }

    void SetVisible( bool aVisible )
    {
        ( aVisible ) ? m_Attributs &= ~TEXT_NO_VISIBLE : m_Attributs |= TEXT_NO_VISIBLE;
    }
    bool IsVisible() const                 { return !( m_Attributs & TEXT_NO_VISIBLE ); }

    void SetMirrored( bool isMirrored )    { m_Mirror = isMirrored; }
    bool IsMirrored() const                { return m_Mirror; }

    void SetAttributes( int aAttributes )  { m_Attributs = aAttributes; }
    int GetAttributes() const              { return m_Attributs; }

    bool IsDefaultFormatting() const;

    /**
     * Function SetSize
     * sets text size.
     * @param aNewSize is the new text size.
     */
    void SetSize( const wxSize& aNewSize ) { m_Size = aNewSize; };

    /**
     * Function GetSize
     * returns text size.
     * @return wxSize - text size.
     */
    const wxSize& GetSize() const          { return m_Size; };

    void SetWidth( int aWidth ) { m_Size.x = aWidth; }
    int GetWidth() const { return m_Size.x; }

    void SetHeight( int aHeight ) { m_Size.y = aHeight; }
    int GetHeight() const { return m_Size.y; }

    /// named differently than the ones using multiple inheritance and including this class
    void SetTextPosition( const wxPoint& aPoint ) { m_Pos = aPoint; }
    const wxPoint& GetTextPosition() const { return m_Pos; }

    void SetMultilineAllowed( bool aAllow )  { m_MultilineAllowed = aAllow; }
    bool IsMultilineAllowed() const          { return m_MultilineAllowed; }

    void Offset( const wxPoint& aOffset ) { m_Pos += aOffset; }

    void Empty() { m_Text.Empty(); }

    /**
     * Function Draw
     * @param aClipBox = the clipping rect, or NULL if no clipping
     * @param aDC = the current Device Context
     * @param aOffset = draw offset (usually (0,0))
     * @param aColor = text color
     * @param aDrawMode = GR_OR, GR_XOR.., -1 to use the current mode.
     * @param aDisplay_mode = LINE, FILLED or SKETCH
     * @param aAnchor_color = anchor color ( UNSPECIFIED = do not draw anchor ).
     */
    void Draw( EDA_RECT* aClipBox, wxDC* aDC,
               const wxPoint& aOffset, EDA_COLOR_T aColor,
               GR_DRAWMODE aDrawMode, EDA_DRAW_MODE_T aDisplay_mode = LINE,
               EDA_COLOR_T aAnchor_color = UNSPECIFIED_COLOR );

    /**
     * Function TextHitTest
     * Test if \a aPoint is within the bounds of this object.
     * @param aPoint- A wxPoint to test
     * @param aAccuracy - Amount to inflate the bounding box.
     * @return bool - true if a hit, else false
     */
    bool TextHitTest( const wxPoint& aPoint, int aAccuracy = 0 ) const;

    /**
     * Function TextHitTest (overloaded)
     * Tests if object bounding box is contained within or intersects \a aRect.
     *
     * @param aRect - Rect to test against.
     * @param aContains - Test for containment instead of intersection if true.
     * @param aAccuracy - Amount to inflate the bounding box.
     * @return bool - true if a hit, else false
     */
    bool TextHitTest( const EDA_RECT& aRect, bool aContains = false, int aAccuracy = 0 ) const;

    /**
     * Function LenSize
     * @return the text length in internal units
     * @param aLine : the line of text to consider.
     * For single line text, this parameter is always m_Text
     */
    int LenSize( const wxString& aLine ) const;

    /**
     * Function GetTextBox
     * useful in multiline texts to calculate the full text or a line area (for
     * zones filling, locate functions....)
     * @return the rect containing the line of text (i.e. the position and the
     *         size of one line)
     *         this rectangle is calculated for 0 orient text.
     *         If orientation is not 0 the rect must be rotated to match the
     *         physical area
     * @param aLine The line of text to consider.
     * for single line text, aLine is unused
     * If aLine == -1, the full area (considering all lines) is returned
     * @param aThickness Overrides the current thickness when greater than 0.
     * this is needed when the current m_Thickness is 0 and a default line thickness
     * is used
     * @param aInvertY Invert the Y axis when calculating bounding box.
     */
    EDA_RECT GetTextBox( int aLine = -1, int aThickness = -1, bool aInvertY = false ) const;

    /**
     * Function GetInterline
     * return the distance between 2 text lines
     * has meaning only for multiline texts
     * @param aTextThickness Overrides the current thickness when greater than 0.
     * this is needed when the current m_Thickness is 0 and a default line thickness
     * is used
     */
    int GetInterline( int aTextThickness = -1 ) const;

    /**
     * Function GetTextStyleName
     * @return a wxString with the style name( Normal, Italic, Bold, Bold+Italic)
     */
    wxString GetTextStyleName();

    EDA_TEXT_HJUSTIFY_T GetHorizJustify() const         { return m_HJustify; };
    EDA_TEXT_VJUSTIFY_T GetVertJustify() const          { return m_VJustify; };

    void SetHorizJustify( EDA_TEXT_HJUSTIFY_T aType )   { m_HJustify = aType; };
    void SetVertJustify( EDA_TEXT_VJUSTIFY_T aType )    { m_VJustify = aType; };

    /**
     * Function GetPositionsOfLinesOfMultilineText
     * Populates aPositions with the position of each line of
     * a multiline text, according to the vertical justification and the
     * rotation of the whole text
     * @param aPositions is the list to populate by the wxPoint positions
     * @param aLineCount is the number of lines (not recalculated here
     * for efficiency reasons
     */
    void GetPositionsOfLinesOfMultilineText(
                std::vector<wxPoint>& aPositions, int aLineCount ) const;
    /**
     * Function Format
     * outputs the object to \a aFormatter in s-expression form.
     *
     * @param aFormatter The #OUTPUTFORMATTER object to write to.
     * @param aNestLevel The indentation next level.
     * @param aControlBits The control bit definition for object specific formatting.
     * @throw IO_ERROR on write error.
     */
    virtual void Format( OUTPUTFORMATTER* aFormatter, int aNestLevel, int aControlBits ) const
        throw( IO_ERROR );

private:

    /**
     * Function drawOneLineOfText
     * Draw a single text line.
     * Used to draw each line of this EDA_TEXT, that can be multiline
     * @param aClipBox = the clipping rect, or NULL if no clipping
     * @param aDC = the current Device Context
     * @param aOffset = draw offset (usually (0,0))
     * @param aColor = text color
     * @param aDrawMode = GR_OR, GR_XOR.., -1 to use the current mode.
     * @param aFillMode = LINE, FILLED or SKETCH
     * @param aText = the single line of text to draw.
     * @param aPos = the position of this line ).
     */
    void drawOneLineOfText( EDA_RECT* aClipBox, wxDC* aDC,
                            const wxPoint& aOffset, EDA_COLOR_T aColor,
                            GR_DRAWMODE aDrawMode, EDA_DRAW_MODE_T aFillMode,
                            wxString& aText, wxPoint aPos );
};


#endif   //  EDA_TEXT_H_
