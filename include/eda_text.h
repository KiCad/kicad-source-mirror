/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#ifndef EDA_TEXT_H_
#define EDA_TEXT_H_

#include <memory>
#include <mutex>
#include <vector>

#include <eda_search_data.h>
#include <font/glyph.h>
#include <font/text_attributes.h>
#include <api/serializable.h>


class OUTPUTFORMATTER;
class SHAPE_COMPOUND;
class SHAPE_POLY_SET;


// These are only here for algorithmic safety, not to tell the user what to do.
// PL_EDITOR has the least resolution (its internal units are microns), so the min size is chosen
// to yield 1 in PL_EDITOR.
// The max size chosen is somewhat arbitrary, but no one has complained yet.
#define TEXT_MIN_SIZE_MM  0.001    ///< Minimum text size (1 micron).
#define TEXT_MAX_SIZE_MM  250.0    ///< Maximum text size in mm (~10 inches)


namespace KIGFX
{
    class RENDER_SETTINGS;
    class COLOR4D;
}

namespace KIFONT
{
    class METRICS;
    }

using KIGFX::RENDER_SETTINGS;
using KIGFX::COLOR4D;


/**
 * This is the "default-of-the-default" hardcoded text size; individual application define their
 * own default policy starting with this (usually with a user option or project).
 */
#define DEFAULT_SIZE_TEXT   50     // default text height (in mils, i.e. 1/1000")


/**
 * A mix-in class (via multiple inheritance) that handles texts such as labels, parts,
 * components, or footprints.  Because it's a mix-in class, care is used to provide
 * function names (accessors) that to not collide with function names likely to be seen
 * in the combined derived classes.
 */
class EDA_TEXT : public SERIALIZABLE
{
public:
    EDA_TEXT( const EDA_IU_SCALE& aIuScale, const wxString& aText = wxEmptyString );

    EDA_TEXT( const EDA_TEXT& aText );

    virtual ~EDA_TEXT();

    EDA_TEXT& operator=( const EDA_TEXT& aItem );

    void Serialize( google::protobuf::Any &aContainer ) const override;
    bool Deserialize( const google::protobuf::Any &aContainer ) override;

    /**
     * Return the string associated with the text object.
     *
     * @return a const wxString reference containing the string of the item.
     */
    virtual const wxString& GetText() const { return m_text; }

    /**
     * Return the string actually shown after processing of the base text.
     *
     * @param aAllowExtraText is true to allow adding more text than the initial expanded text,
     * for intance a title, a prefix for texts in display functions.
     * False to disable any added text (for instance when writing the shown text in netlists).
     * @param aDepth is used to prevent infinite recursions and loops when expanding
     * text variables.
     */
    virtual wxString GetShownText( bool aAllowExtraText, int aDepth = 0 ) const
    {
        return m_shown_text;
    }

    /**
     * Indicates the ShownText has text var references which need to be processed.
     */
    bool HasTextVars() const { return m_shown_text_has_text_var_refs; }

    virtual void SetText( const wxString& aText );

    wxString EvaluateText( const wxString& aText ) const;

    /**
     * The TextThickness is that set by the user.  The EffectiveTextPenWidth also factors
     * in bold text and thickness clamping.
     */
    void SetTextThickness( int aWidth );
    int GetTextThickness() const                { return m_attributes.m_StrokeWidth; };

    int GetTextThicknessProperty() const
    {
        if( GetAutoThickness() )
            return GetEffectiveTextPenWidth();
        else
            return GetTextThickness();
    }

    void SetAutoThickness( bool aAuto );
    bool GetAutoThickness() const               { return GetTextThickness() == 0; };

    /**
     * The EffectiveTextPenWidth uses the text thickness if > 1 or aDefaultPenWidth.
     */
    int GetEffectiveTextPenWidth( int aDefaultPenWidth = 0 ) const;

    virtual void SetTextAngle( const EDA_ANGLE& aAngle );
    const EDA_ANGLE& GetTextAngle() const       { return m_attributes.m_Angle; }

    // For property system:
    void SetTextAngleDegrees( double aOrientation )
    {
        SetTextAngle( EDA_ANGLE( aOrientation, DEGREES_T ) );
    }
    double GetTextAngleDegrees() const          { return m_attributes.m_Angle.AsDegrees(); }

    /**
     * Set the text to be italic - this will also update the font if needed.
     *
     * This is the properties system interface.
     */
    void SetItalic( bool aItalic );

    /**
     * Set only the italic flag, without changing the font.
     *
     * Used when bulk-changing text attributes (e.g. from a dialog or import).
     */
    void SetItalicFlag( bool aItalic );
    bool IsItalic() const                       { return m_attributes.m_Italic; }

    /**
     * Set the text to be bold - this will also update the font if needed.
     *
     * This is the properties system interface.
     */
    void SetBold( bool aBold );

    /**
     * Set only the bold flag, without changing the font.
     *
     * Used when bulk-changing text attributes (e.g. from a dialog or import).
     */
    void SetBoldFlag( bool aBold );
    bool IsBold() const                         { return m_attributes.m_Bold; }

    virtual void SetVisible( bool aVisible );
    virtual bool IsVisible() const              { return m_visible; }

    void SetMirrored( bool isMirrored );
    bool IsMirrored() const                     { return m_attributes.m_Mirrored; }

    /**
     * @param aAllow true if ok to use multiline option, false if ok to use only single line
     *               text.  (Single line is faster in calculations than multiline.)
     */
    void SetMultilineAllowed( bool aAllow );
    bool IsMultilineAllowed() const             { return m_attributes.m_Multiline; }

    void SetHorizJustify( GR_TEXT_H_ALIGN_T aType );
    GR_TEXT_H_ALIGN_T GetHorizJustify() const   { return m_attributes.m_Halign; };

    void SetVertJustify( GR_TEXT_V_ALIGN_T aType );
    GR_TEXT_V_ALIGN_T GetVertJustify() const    { return m_attributes.m_Valign; };

    void SetKeepUpright( bool aKeepUpright );
    bool IsKeepUpright() const                  { return m_attributes.m_KeepUpright; }

    void FlipHJustify()
    {
        if( GetHorizJustify() == GR_TEXT_H_ALIGN_RIGHT )
            SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        else if( GetHorizJustify() == GR_TEXT_H_ALIGN_LEFT )
            SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
    }

    /**
     * Set the text attributes from another instance.
     */
    void SetAttributes( const EDA_TEXT& aSrc, bool aSetPosition = true );

    /**
     * Swap the text attributes of the two involved instances.
     */
    void SwapAttributes( EDA_TEXT& aTradingPartner );

    void SwapText( EDA_TEXT& aTradingPartner );

    void CopyText( const EDA_TEXT& aSrc );

    void SetAttributes( const TEXT_ATTRIBUTES& aTextAttrs ) { m_attributes = aTextAttrs; }
    const TEXT_ATTRIBUTES& GetAttributes() const { return m_attributes; }

    /**
     * Helper function used in search and replace dialog.
     *
     * Perform a text replace using the find and replace criteria in \a aSearchData.
     *
     * @param aSearchData A reference to a EDA_SEARCH_DATA object containing the
     *                    search and replace criteria.
     * @return True if the text item was modified, otherwise false.
     */
    bool Replace( const EDA_SEARCH_DATA& aSearchData );

    bool IsDefaultFormatting() const;

    void SetFont( KIFONT::FONT* aFont );
    KIFONT::FONT* GetFont() const               { return m_attributes.m_Font; }

    void SetUnresolvedFontName( const wxString& aFontName ) { m_unresolvedFontName = aFontName; }
    bool ResolveFont( const std::vector<wxString>* aEmbeddedFonts );

    wxString GetFontName() const;

    void SetFontProp( const wxString& aFontName );
    wxString GetFontProp() const;

    void SetLineSpacing( double aLineSpacing );
    double GetLineSpacing() const               { return m_attributes.m_LineSpacing; }

    void SetTextSize( VECTOR2I aNewSize, bool aEnforceMinTextSize = true );
    VECTOR2I GetTextSize() const                { return m_attributes.m_Size; }

    void SetTextWidth( int aWidth );
    int GetTextWidth() const                    { return m_attributes.m_Size.x; }

    void SetTextHeight( int aHeight );
    int GetTextHeight() const                   { return m_attributes.m_Size.y; }

    void SetTextColor( const COLOR4D& aColor )  { m_attributes.m_Color = aColor; }
    COLOR4D GetTextColor() const                { return m_attributes.m_Color; }

    void SetTextPos( const VECTOR2I& aPoint );
    const VECTOR2I& GetTextPos() const          { return m_pos; }

    void SetTextX( int aX );
    void SetTextY( int aY );

    void Offset( const VECTOR2I& aOffset );

    void Empty();

    static GR_TEXT_H_ALIGN_T MapHorizJustify( int aHorizJustify );
    static GR_TEXT_V_ALIGN_T MapVertJustify( int aVertJustify );

    /**
     * Print this text object to the device context \a aDC.
     *
     * @param aDC the current Device Context.
     * @param aOffset draw offset (usually (0,0)).
     * @param aColor text color.
     */
    void Print( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset, const COLOR4D& aColor );

    /**
     * build a list of segments (SHAPE_SEGMENT) to describe a text shape.
     * @param aTriangulate: true to build also the triangulation of each shape
     * @param aUseTextRotation: true to use the actual text draw rotation.
     * false to build a list of shape for a not rotated text ("native" shapes).
     */
    std::shared_ptr<SHAPE_COMPOUND> GetEffectiveTextShape( bool aTriangulate = true,
                                                           const BOX2I& aBBox = BOX2I(),
                                                           const EDA_ANGLE& aAngle = ANGLE_0 ) const;

    /**
     * Test if \a aPoint is within the bounds of this object.
     *
     * @param aPoint A VECTOR2I to test.
     * @param aAccuracy Amount to inflate the bounding box.
     * @return true if a hit, else false.
     */
    virtual bool TextHitTest( const VECTOR2I& aPoint, int aAccuracy = 0 ) const;

    /**
     * Test if object bounding box is contained within or intersects \a aRect.
     *
     * @param aRect Rect to test against.
     * @param aContains Test for containment instead of intersection if true.
     * @param aAccuracy Amount to inflate the bounding box.
     * @return true if a hit, else false.
     */
    virtual bool TextHitTest( const BOX2I& aRect, bool aContains, int aAccuracy = 0 ) const;

    /**
     * Useful in multiline texts to calculate the full text or a line area (for zones filling,
     * locate functions....)
     *
     * @param aLine The line of text to consider.  Pass -1 for all lines.
     * @return the rect containing the line of text (i.e. the position and the size of one line)
     *         this rectangle is calculated for 0 orient text.
     *         If orientation is not 0 the rect must be rotated to match the physical area
     */
    BOX2I GetTextBox( const RENDER_SETTINGS* aSettings, int aLine = -1 ) const;

    /**
     * Return the distance between two lines of text.
     *
     * Calculates the distance (pitch) between two lines of text.  This distance includes the
     * interline distance plus room for characters like j, {, and [.  It also used for single
     * line text, to calculate the text bounding box.
     */
    int GetInterline( const RENDER_SETTINGS* aSettings ) const;

    /**
     * @return a wxString with the style name( Normal, Italic, Bold, Bold+Italic).
     */
    wxString GetTextStyleName() const;

    /**
     * Populate \a aPositions with the position of each line of a multiline text, according
     * to the vertical justification and the rotation of the whole text.
     *
     * @param aPositions is the list to populate by the VECTOR2I positions.
     * @param aLineCount is the number of lines (not recalculated here for efficiency reasons.
     */
    void GetLinePositions( const RENDER_SETTINGS* aSettings, std::vector<VECTOR2I>& aPositions,
                           int aLineCount ) const;

    /**
     * Return the levenstein distance between two texts.
     *
     * Return a value of 0.0 - 1.0 where 1.0 is a perfect match.
    */
    double Levenshtein( const EDA_TEXT& aOther ) const;


    double Similarity( const EDA_TEXT& aOther ) const;

    /**
     * Output the object to \a aFormatter in s-expression form.
     *
     * @param aFormatter The #OUTPUTFORMATTER object to write to.
     * @param aControlBits The control bit definition for object specific formatting.
     * @throw IO_ERROR on write error.
     */
    virtual void Format( OUTPUTFORMATTER* aFormatter, int aControlBits ) const;

    virtual EDA_ANGLE GetDrawRotation() const               { return GetTextAngle(); }
    virtual VECTOR2I GetDrawPos() const                     { return GetTextPos(); }

    virtual KIFONT::FONT* GetDrawFont( const RENDER_SETTINGS* aSettings ) const;

    virtual void ClearRenderCache();
    virtual void ClearBoundingBoxCache();

    std::vector<std::unique_ptr<KIFONT::GLYPH>>*
    GetRenderCache( const KIFONT::FONT* aFont, const wxString& forResolvedText,
                    const VECTOR2I& aOffset = { 0, 0 } ) const;

    // Support for reading the cache from disk.
    void SetupRenderCache( const wxString& aResolvedText, const KIFONT::FONT* aFont,
                           const EDA_ANGLE& aAngle, const VECTOR2I& aOffset );
    void AddRenderCacheGlyph( const SHAPE_POLY_SET& aPoly );

    int Compare( const EDA_TEXT* aOther ) const;

    bool operator==( const EDA_TEXT& aRhs ) const { return Compare( &aRhs ) == 0; }
    bool operator<( const EDA_TEXT& aRhs ) const { return Compare( &aRhs ) < 0; }
    bool operator>( const EDA_TEXT& aRhs ) const { return Compare( &aRhs ) > 0; }

    virtual bool HasHyperlink() const           { return !m_hyperlink.IsEmpty(); }
    wxString     GetHyperlink() const           { return m_hyperlink; }
    void         SetHyperlink( wxString aLink ) { m_hyperlink = aLink; }
    void         RemoveHyperlink()              { m_hyperlink = wxEmptyString; }

    /**
     * Check if aURL is a valid hyperlink.
     *
     * @param aURL String to validate
     * @return true if aURL is a valid hyperlink
     */
    static bool ValidateHyperlink( const wxString& aURL );

    /**
     * Check if aHref is a valid internal hyperlink.
     *
     * @param aHref String to validate
     * @param aDestination [optional] pointer to populate with the destination page
     * @return true if aHref is a valid internal hyperlink.  Does *not* check if the destination
     *         page actually exists.
     */
    static bool IsGotoPageHref( const wxString& aHref, wxString* aDestination = nullptr );

    /**
     * Generate a href to a page in the current schematic.
     *
     * @param aDestination Destination sheet's page number.
     * @return A hyperlink href string that goes to the specified page.
     */
    static wxString GotoPageHref( const wxString& aDestination );

protected:
    virtual const KIFONT::METRICS& getFontMetrics() const;

    virtual void cacheShownText();

    /**
     * Print each line of this EDA_TEXT.
     *
     * @param aOffset draw offset (usually (0,0)).
     * @param aColor text color.
     * @param aText the single line of text to draw.
     * @param aPos the position of this line ).
     */
    void printOneLineOfText( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset,
                             const COLOR4D& aColor, const wxString& aText, const VECTOR2I& aPos );

    bool containsURL() const;

protected:
    /**
     * A hyperlink URL.  If empty, this text object is not a hyperlink.
     */
    wxString m_hyperlink;

private:
    wxString         m_text;
    wxString         m_shown_text;           // Cache of unescaped text for efficient access
    bool             m_shown_text_has_text_var_refs;

    std::reference_wrapper<const EDA_IU_SCALE>          m_IuScale;

    mutable wxString                                    m_render_cache_text;
    mutable const KIFONT::FONT*                         m_render_cache_font;
    mutable EDA_ANGLE                                   m_render_cache_angle;
    mutable VECTOR2I                                    m_render_cache_offset;
    mutable std::vector<std::unique_ptr<KIFONT::GLYPH>> m_render_cache;

    struct BBOX_CACHE_ENTRY
    {
        VECTOR2I m_pos;
        BOX2I    m_bbox;
    };

    mutable std::map<int, BBOX_CACHE_ENTRY> m_bbox_cache;
    mutable std::mutex m_bbox_cacheMutex;

    TEXT_ATTRIBUTES  m_attributes;
    wxString         m_unresolvedFontName;
    VECTOR2I         m_pos;
    bool             m_visible;                 // For SCH_FIELDs and PCB_FIELDs
};


extern std::ostream& operator<<( std::ostream& aStream, const EDA_TEXT& aAttributes );


template<>
struct std::hash<EDA_TEXT>
{
    std::size_t operator()( const EDA_TEXT& aText ) const
    {
        return hash_val( aText.GetText(), aText.GetAttributes(), aText.GetTextPos().x,
                         aText.GetTextPos().y );
    }
};

#endif   //  EDA_TEXT_H_
