/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jpe.charras at wanadoo.fr
 * Copyright (C) 2004-2021 KiCad Developers, see change_log.txt for contributors.
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

#ifndef EDA_TEXT_H_
#define EDA_TEXT_H_

#include <memory>
#include <vector>

#include <outline_mode.h>
#include <eda_rect.h>

class OUTPUTFORMATTER;
class SHAPE_COMPOUND;
class SHAPE_POLY_SET;
class wxFindReplaceData;

namespace KIGFX
{
    class RENDER_SETTINGS;
    class COLOR4D;
}

using KIGFX::RENDER_SETTINGS;
using KIGFX::COLOR4D;

/// Frequent text rotations, used with {Set,Get}TextAngle(),
/// in 0.1 degrees for now, hoping to migrate to degrees eventually.
#define TEXT_ANGLE_HORIZ 0
#define TEXT_ANGLE_VERT 900

// part of the kicad_plugin.h family of defines.
// See kicad_plugin.h for the choice of the value
// When set when calling  EDA_TEXT::Format, disable writing the "hide" keyword in save file
#define CTL_OMIT_HIDE               (1 << 6)


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


/**
 * This is the "default-of-the-default" hardcoded text size; individual
 * application define their own default policy starting with this
 * (usually with a user option or project).
 */
#define DEFAULT_SIZE_TEXT   50     // default text height (in mils, i.e. 1/1000")
#define DIM_ANCRE_TEXTE     2      // Anchor size for text


/**
 * A container for text effects.
 *
 * These fields are bundled so they can be easily copied together as a lot. The privacy
 * policy is established by client (incorporating) code.
 */
struct TEXT_EFFECTS
{
    TEXT_EFFECTS( int aSetOfBits = 0 ) :
        bits( aSetOfBits ),
        hjustify( GR_TEXT_HJUSTIFY_CENTER ),
        vjustify( GR_TEXT_VJUSTIFY_CENTER ),
        penwidth( 0 ),
        angle( 0.0 )
    {}

    int         bits;           ///< any set of booleans a client uses.
    signed char hjustify;       ///< horizontal justification
    signed char vjustify;       ///< vertical justification
    wxSize      size;
    int         penwidth;
    double      angle;          ///< now: 0.1 degrees; future: degrees
    wxPoint     pos;

    void Bit( int aBit, bool aValue )   { aValue ? bits |= (1<<aBit) : bits &= ~(1<<aBit); }
    bool Bit( int aBit ) const          { return bits & (1<<aBit); }
};


/**
 * A mix-in class (via multiple inheritance) that handles texts such as labels, parts,
 * components, or footprints.  Because it's a mix-in class, care is used to provide
 * function names (accessors) that to not collide with function names likely to be seen
 * in the combined derived classes.
 */
class EDA_TEXT
{
public:
    EDA_TEXT( const wxString& text = wxEmptyString );

    EDA_TEXT( const EDA_TEXT& aText );

    virtual ~EDA_TEXT();

    /**
     * Return the string associated with the text object.
     *
     * @return a const wxString reference containing the string of the item.
     */
    virtual const wxString& GetText() const { return m_text; }

    /**
     * Return the string actually shown after processing of the base text.
     *
     * @param aDepth is used to prevent infinite recursions and loops when expanding
     * text variables.
     */
    virtual wxString GetShownText( int aDepth = 0 ) const { return m_shown_text; }

    /**
     * Returns a shortened version (max 15 characters) of the shown text
     */
    wxString ShortenedShownText() const;

    /**
     * Indicates the ShownText has text var references which need to be processed.
     */
    bool HasTextVars() const { return m_shown_text_has_text_var_refs; }

    virtual void SetText( const wxString& aText );

    /**
     * The TextThickness is that set by the user.  The EffectiveTextPenWidth also factors
     * in bold text and thickness clamping.
     */
    void SetTextThickness( int aWidth ) { m_e.penwidth = aWidth; };
    int GetTextThickness() const { return m_e.penwidth; };

    /**
     * The EffectiveTextPenWidth uses the text thickness if > 1 or aDefaultWidth.
     */
    int GetEffectiveTextPenWidth( int aDefaultWidth = 0 ) const;

    virtual void SetTextAngle( double aAngle )
    {
        // Higher level classes may be more restrictive than this by overloading
        // SetTextAngle() or merely calling EDA_TEXT::SetTextAngle() after clamping
        // aAngle before calling this lowest inline accessor.
        m_e.angle = aAngle;
    }
    double GetTextAngle() const                 { return m_e.angle; }

    double GetTextAngleDegrees() const          { return GetTextAngle() / 10.0; }
    double GetTextAngleRadians() const          { return GetTextAngle() * M_PI/1800; }

    void SetItalic( bool isItalic )             { m_e.Bit( TE_ITALIC, isItalic ); }
    bool IsItalic() const                       { return m_e.Bit( TE_ITALIC ); }

    void SetBold( bool aBold )                  { m_e.Bit( TE_BOLD, aBold); }
    bool IsBold() const                         { return m_e.Bit( TE_BOLD ); }

    virtual void SetVisible( bool aVisible )    { m_e.Bit( TE_VISIBLE, aVisible ); }
    virtual bool IsVisible() const              { return m_e.Bit( TE_VISIBLE ); }

    void SetMirrored( bool isMirrored )         { m_e.Bit( TE_MIRROR, isMirrored ); }
    bool IsMirrored() const                     { return m_e.Bit( TE_MIRROR ); }

    /**
     * @param aAllow true if ok to use multiline option, false if ok to use only single line
     *               text.  (Single line is faster in calculations than multiline.)
     */
    void SetMultilineAllowed( bool aAllow )     { m_e.Bit( TE_MULTILINE, aAllow ); }
    bool IsMultilineAllowed() const             { return m_e.Bit( TE_MULTILINE ); }

    EDA_TEXT_HJUSTIFY_T GetHorizJustify() const { return EDA_TEXT_HJUSTIFY_T( m_e.hjustify ); };
    EDA_TEXT_VJUSTIFY_T GetVertJustify() const  { return EDA_TEXT_VJUSTIFY_T( m_e.vjustify ); };

    void SetHorizJustify( EDA_TEXT_HJUSTIFY_T aType )   { m_e.hjustify = aType; };
    void SetVertJustify( EDA_TEXT_VJUSTIFY_T aType )    { m_e.vjustify = aType; };

    /**
     * Set the text effects from another instance.
     *
     * #TEXT_EFFECTS is not exposed in the public API, but includes everything except the actual
     * text string itself.
     */
    void SetEffects( const EDA_TEXT& aSrc );

    /**
     * Swap the text effects of the two involved instances.
     *
     * #TEXT_EFFECTS is not exposed in the public API, but includes everything except the actual
     * text string itself.
     */
    void SwapEffects( EDA_TEXT& aTradingPartner );

    void SwapText( EDA_TEXT& aTradingPartner );

    void CopyText( const EDA_TEXT& aSrc );

    /**
     * Helper function used in search and replace dialog.
     *
     * Perform a text replace using the find and replace criteria in \a aSearchData.
     *
     * @param aSearchData A reference to a wxFindReplaceData object containing the
     *                    search and replace criteria.
     * @return True if the text item was modified, otherwise false.
     */
    bool Replace( const wxFindReplaceData& aSearchData );

    bool IsDefaultFormatting() const;

    void SetTextSize( const wxSize& aNewSize )  { m_e.size = aNewSize; }
    const wxSize& GetTextSize() const           { return m_e.size; }

    void SetTextWidth( int aWidth )             { m_e.size.x = aWidth; }
    int GetTextWidth() const                    { return m_e.size.x; }

    void SetTextHeight( int aHeight )           { m_e.size.y = aHeight; }
    int GetTextHeight() const                   { return m_e.size.y; }

    void SetTextPos( const wxPoint& aPoint )    { m_e.pos = aPoint; }
    const wxPoint& GetTextPos() const           { return m_e.pos; }

    void SetTextX( int aX )                     { m_e.pos.x = aX; }
    void SetTextY( int aY )                     { m_e.pos.y = aY; }

    void Offset( const wxPoint& aOffset )       { m_e.pos += aOffset; }

    void Empty()                                { m_text.Empty(); }

    static EDA_TEXT_HJUSTIFY_T MapHorizJustify( int aHorizJustify );

    static EDA_TEXT_VJUSTIFY_T MapVertJustify( int aVertJustify );

    /**
     * Print this text object to the device context \a aDC.
     *
     * @param aDC the current Device Context.
     * @param aOffset draw offset (usually (0,0)).
     * @param aColor text color.
     * @param aDisplay_mode #FILLED or #SKETCH.
     */
    void Print( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset,
                const COLOR4D& aColor, OUTLINE_MODE aDisplay_mode = FILLED );

    /**
     * Convert the text shape to a list of segment.
     *
     * Each segment is stored as 2 wxPoints: the starting point and the ending point
     * there are therefore 2*n points.
     */
    std::vector<wxPoint> TransformToSegmentList() const;

    /**
     * Convert the text bounding box to a rectangular polygon depending on the text
     * orientation, the bounding box is not always horizontal or vertical
     *
     * Used in filling zones calculations
     * Circles and arcs are approximated by segments
     *
     * @param aCornerBuffer a buffer to store the polygon.
     * @param aClearanceValue the clearance around the text bounding box
     * to the real clearance value (usually near from 1.0).
     */
    void TransformBoundingBoxWithClearanceToPolygon( SHAPE_POLY_SET* aCornerBuffer,
                                                     int aClearanceValue ) const;

    std::shared_ptr<SHAPE_COMPOUND> GetEffectiveTextShape() const;

    /**
     * Test if \a aPoint is within the bounds of this object.
     *
     * @param aPoint A wxPoint to test.
     * @param aAccuracy Amount to inflate the bounding box.
     * @return true if a hit, else false.
     */
    virtual bool TextHitTest( const wxPoint& aPoint, int aAccuracy = 0 ) const;

    /**
     * Test if object bounding box is contained within or intersects \a aRect.
     *
     * @param aRect Rect to test against.
     * @param aContains Test for containment instead of intersection if true.
     * @param aAccuracy Amount to inflate the bounding box.
     * @return true if a hit, else false.
     */
    virtual bool TextHitTest( const EDA_RECT& aRect, bool aContains, int aAccuracy = 0 ) const;

    /**
     * @return the text length in internal units.
     * @param aLine the line of text to consider.  For single line text, this parameter
     *              is always m_Text.
     * @param aThickness the stroke width of the text.
     */
    int LenSize( const wxString& aLine, int aThickness ) const;

    /**
     * Useful in multiline texts to calculate the full text or a line area (for zones filling,
     * locate functions....)
     *
     * @param aLine The line of text to consider.  Pass -1 for all lines.
     * @param aInvertY Invert the Y axis when calculating bounding box.
     * @return the rect containing the line of text (i.e. the position and the size of one line)
     *         this rectangle is calculated for 0 orient text.
     *         If orientation is not 0 the rect must be rotated to match the physical area
     */
    EDA_RECT GetTextBox( int aLine = -1, bool aInvertY = false ) const;

    /**
     * Return the distance between two lines of text.
     *
     * Calculates the distance (pitch) between two lines of text.  This distance includes the
     * interline distance plus room for characters like j, {, and [.  It also used for single
     * line text, to calculate the text bounding box.
     */
    int GetInterline() const;

    /**
     * @return a wxString with the style name( Normal, Italic, Bold, Bold+Italic).
     */
    wxString GetTextStyleName() const;

    /**
     * Populate \a aPositions with the position of each line of a multiline text, according
     * to the vertical justification and the rotation of the whole text.
     *
     * @param aPositions is the list to populate by the wxPoint positions.
     * @param aLineCount is the number of lines (not recalculated here for efficiency reasons.
     */
    void GetLinePositions( std::vector<wxPoint>& aPositions, int aLineCount ) const;

    /**
     * Output the object to \a aFormatter in s-expression form.
     *
     * @param aFormatter The #OUTPUTFORMATTER object to write to.
     * @param aNestLevel The indentation next level.
     * @param aControlBits The control bit definition for object specific formatting.
     * @throw IO_ERROR on write error.
     */
    virtual void Format( OUTPUTFORMATTER* aFormatter, int aNestLevel, int aControlBits ) const;

    virtual double GetDrawRotation() const;

    int Compare( const EDA_TEXT* aOther ) const;

private:
    void cacheShownText();

    /**
     * Print each line of this EDA_TEXT..
     *
     * @param aOffset draw offset (usually (0,0)).
     * @param aColor text color.
     * @param aFillMode FILLED or SKETCH
     * @param aText the single line of text to draw.
     * @param aPos the position of this line ).
     */
    void printOneLineOfText( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset,
                             const COLOR4D& aColor, OUTLINE_MODE aFillMode, const wxString& aText,
                             const wxPoint& aPos );

    wxString      m_text;
    wxString      m_shown_text;           // Cache of unescaped text for efficient access
    bool          m_shown_text_has_text_var_refs;

    TEXT_EFFECTS  m_e;                    // Private bitflags for text styling.  API above
                                          // provides accessor funcs.
    enum TE_FLAGS {
        TE_MIRROR,
        TE_ITALIC,
        TE_BOLD,
        TE_MULTILINE,
        TE_VISIBLE,
    };
};


#endif   //  EDA_TEXT_H_
