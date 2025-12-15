/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Color class
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

#pragma once

#include <optional>
#include <kicommon.h>
#include <wx/debug.h>
#include <wx/colour.h>
#include <wx/string.h>
#include <hash.h>
#include <nlohmann/json_fwd.hpp>


/**
 * Legacy color enumeration. Also contains a flag and the alpha value in the upper bits
 */
enum EDA_COLOR_T
{
    UNSPECIFIED_COLOR = -1,
    BLACK = 0,
    DARKDARKGRAY,
    DARKGRAY,
    LIGHTGRAY,
    WHITE,
    LIGHTYELLOW,
    DARKBLUE,
    DARKGREEN,
    DARKCYAN,
    DARKRED,
    DARKMAGENTA,
    DARKBROWN,
    BLUE,
    GREEN,
    CYAN,
    RED,
    MAGENTA,
    BROWN,
    LIGHTBLUE,
    LIGHTGREEN,
    LIGHTCYAN,
    LIGHTRED,
    LIGHTMAGENTA,
    YELLOW,
    PUREBLUE,
    PUREGREEN,
    PURECYAN,
    PURERED,
    PUREMAGENTA,
    PUREYELLOW,
    LIGHTERORANGE,
    DARKORANGE,
    ORANGE,
    LIGHTORANGE,
    PUREORANGE,
    NBCOLORS,                     ///< Number of colors
    HIGHLIGHT_FLAG = ( 1 << 19 ),
    MASKCOLOR      =   31         ///< mask for color index into colorRefs()[]
};


struct KICOMMON_API StructColors
{
    unsigned char m_Blue;
    unsigned char m_Green;
    unsigned char m_Red;
    EDA_COLOR_T   m_Numcolor;
    std::string   m_ColorName;
    EDA_COLOR_T   m_LightColor;
};

/// Global list of legacy color names, still used all over the place for constructing COLOR4D's
KICOMMON_API const StructColors* colorRefs();


namespace KIGFX
{
/**
 * A color representation with 4 components: red, green, blue, alpha.
 */
class KICOMMON_API COLOR4D
{
public:
    // Constructor (creates the Color 0,0,0,0)
    COLOR4D() :
        r( 0 ),
        g( 0 ),
        b( 0 ),
        a( 1.0 )
    {
    }

    /**
     * @param aRed   is the red component   [0.0 .. 1.0].
     * @param aGreen is the green component [0.0 .. 1.0].
     * @param aBlue  is the blue component  [0.0 .. 1.0].
     * @param aAlpha is the alpha value     [0.0 .. 1.0].
     */
    COLOR4D( double aRed, double aGreen, double aBlue, double aAlpha ) :
        r( aRed ),
        g( aGreen ),
        b( aBlue ),
        a( aAlpha )
    {
        wxASSERT( r >= 0.0 && r <= 1.0 );
        wxASSERT( g >= 0.0 && g <= 1.0 );
        wxASSERT( b >= 0.0 && b <= 1.0 );
        wxASSERT( a >= 0.0 && a <= 1.0 );
    }

    /**
     * @param aColor is one of KiCad's palette colors.
     * @see EDA_COLOR_T
     */
    COLOR4D( EDA_COLOR_T aColor );

    /**
     * Initialize the color from a RGBA value with 0-255 red/green/blue and 0-1 alpha.
     *
     * Suitable for taking the values directly from the "CSS syntax" from ToWxString.
     *
     * @return this color.
     */
    COLOR4D& FromCSSRGBA( int aRed, int aGreen, int aBlue, double aAlpha = 1.0 );

    /**
     * Defines a color from a CSS or HTML-type string
     * @param aColorStr input string
     */
    COLOR4D( const wxString& aColorStr );

    /**
     * @param aColor is the color type used by wxWidgets.
     */
    COLOR4D( const wxColour& aColor );

    /**
     * Set color values by parsing a string using wxColour::Set().
     *
     * @param aColorString is a color string that wxColour can understand.
     * @return true if color was set successfully.
     */
    bool SetFromWxString( const wxString& aColorString );

    wxString ToCSSString() const;

    bool SetFromHexString( const wxString& aColorString );
    wxString ToHexString() const;

    wxColour ToColour() const;

    /**
     * Mix this COLOR4D with an input COLOR4D using the OR-mixing of legacy canvas.
     *
     * Can be removed once legacy canvas is removed.  Depends on wxColour for simplicity,
     * but could be re-written to avoid this dependency if desired.
     *
     * @param aColor The color to mix with this one
     */
    COLOR4D LegacyMix( const COLOR4D& aColor ) const;

    /**
     * Converts current color (stored in RGB) to HSL format.
     *
     * @param aOutHue is the conversion result for hue component, in degrees 0 ... 360.0.
     * @param aOutSaturation is the conversion result for saturation component (0 ... 1.0).
     * @param aOutLightness is conversion result for value component (0 ... 1.0).
     * @note saturation is set to 0.0 for black color if r = g = b,
     */
    void ToHSL( double& aOutHue, double& aOutSaturation, double& aOutLightness ) const;

    /**
     * Change currently used color to the one given by hue, saturation and lightness parameters.
     *
     * @param aInHue is hue component, in degrees (0.0 - 360.0).
     * @param aInSaturation is saturation component (0.0 - 1.0).
     * @param aInLightness is lightness component (0.0 - 1.0).
     */
    void FromHSL( double aInHue, double aInSaturation, double aInLightness );

    /**
     * Makes the color brighter by a given factor.
     *
     * @param aFactor Specifies how bright the color should become (valid values: 0.0 .. 1.0).
     * @return COLOR4D& Brightened color.
     */
    COLOR4D& Brighten( double aFactor )
    {
        wxASSERT( aFactor >= 0.0 && aFactor <= 1.0 );

        r = r * ( 1.0 - aFactor ) + aFactor;
        g = g * ( 1.0 - aFactor ) + aFactor;
        b = b * ( 1.0 - aFactor ) + aFactor;

        return *this;
    }

    /**
     * Makes the color darker by a given factor.
     *
     * @param aFactor Specifies how dark the color should become (valid values: 0.0 .. 1.0).
     * @return COLOR4D& Darkened color.
     */
    COLOR4D& Darken( double aFactor )
    {
        wxASSERT( aFactor >= 0.0 && aFactor <= 1.0 );

        r = r * ( 1.0 - aFactor );
        g = g * ( 1.0 - aFactor );
        b = b * ( 1.0 - aFactor );

        return *this;
    }

    /**
     * Makes the color inverted, alpha remains the same.
     *
     * @return COLOR4D& Inverted color.
     */
    COLOR4D& Invert()
    {
        r = ( 1.0 - r );
        g = ( 1.0 - g );
        b = ( 1.0 - b );

        return *this;
    }

    /**
     * Saturates the color to a given factor (in HSV model)
     */
    COLOR4D& Saturate( double aFactor );

    /**
     * Removes color (in HSL model)
     * @return greyscale version of color
     */
    COLOR4D& Desaturate();

    /**
     * Return a color that is brighter by a given factor, without modifying object.
     *
     * @param aFactor Specifies how bright the color should become (valid values: 0.0 .. 1.0).
     * @return COLOR4D Highlighted color.
     */
    COLOR4D Brightened( double aFactor ) const
    {
        wxASSERT( aFactor >= 0.0 && aFactor <= 1.0 );

        return COLOR4D( r * ( 1.0 - aFactor ) + aFactor, g * ( 1.0 - aFactor ) + aFactor,
                        b * ( 1.0 - aFactor ) + aFactor, a );
    }

    /**
     * Return a color that is darker by a given factor, without modifying object.
     *
     * @param aFactor Specifies how dark the color should become (valid values: 0.0 .. 1.0).
     * @return COLOR4D Darkened color.
     */
    COLOR4D Darkened( double aFactor ) const
    {
        wxASSERT( aFactor >= 0.0 && aFactor <= 1.0 );

        return COLOR4D( r * ( 1.0 - aFactor ), g * ( 1.0 - aFactor ), b * ( 1.0 - aFactor ), a );
    }

    /**
     * Return a color that is mixed with the input by a factor.
     *
     * @param aFactor Specifies how much of the original color to keep (valid values: 0.0 .. 1.0).
     * @return COLOR4D Mixed color.
     */
    COLOR4D Mix( const COLOR4D& aColor, double aFactor ) const
    {
        wxASSERT( aFactor >= 0.0 && aFactor <= 1.0 );

        return COLOR4D( aColor.r * ( 1.0 - aFactor ) + r * aFactor,
                        aColor.g * ( 1.0 - aFactor ) + g * aFactor,
                        aColor.b * ( 1.0 - aFactor ) + b * aFactor,
                        a );
    }

    /**
     * Return a color with the same color, but the given alpha.
     *
     * @param aAlpha specifies the alpha of the new color
     * @return COLOR4D color with that alpha
     */
     COLOR4D WithAlpha( double aAlpha ) const
     {
         wxASSERT( aAlpha >= 0.0 && aAlpha <= 1.0 );

         return COLOR4D( r, g, b, aAlpha );
     }

    /**
     * Returns an inverted color, alpha remains the same.
     *
     * @return COLOR4D& Inverted color.
     */
    COLOR4D Inverted() const
    {
        return COLOR4D( 1.0 - r, 1.0 - g, 1.0 - b, a );
    }

    /**
     * Returns the brightness value of the color ranged from 0.0 to 1.0.
     *
     * @return The brightness value.
     */
    double GetBrightness() const
    {
        // Weighted W3C formula
        return r * 0.299 + g * 0.587 + b * 0.117;
    }

    /**
     * Convert current color (stored in RGB) to HSV format.
     *
     * @param aOutHue is the conversion result for hue component, in degrees 0 ... 360.0.
     * @param aOutSaturation is the conversion result for saturation component (0 ... 1.0).
     * @param aOutValue is conversion result for value component (0 ... 1.0).
     * @param aAlwaysDefineHue controls the way hue is defined when r = v = b
     * @note saturation is set to 0.0 for black color (r = v = b = 0), and if r = v = b,
     * hue is set to 0.0 if aAlwaysDefineHue = true, and set to NAN if aAlwaysDefineHue = false.
     * this option is useful to convert a 4D color to a legacy color, because Red has hue = 0,
     * therefore aAlwaysDefineHue = false makes difference between Red and Gray colors.
     */
    void ToHSV( double& aOutHue, double& aOutSaturation, double& aOutValue,
                bool aAlwaysDefineHue = false ) const;

    /**
     * Changes currently used color to the one given by hue, saturation and value parameters.
     *
     * @param aInH is hue component, in degrees.
     * @param aInS is saturation component.
     * @param aInV is value component.
     */
    void FromHSV( double aInH, double aInS, double aInV );

    /**
     * Returns the distance (in RGB space) between two colors.
     */
    double Distance( const COLOR4D& other ) const;

    int Compare( const COLOR4D& aRhs ) const;

    /**
     * Compute the relative luminance of a color using the formula from WCAG21.
     *
     * @return relative luminance between 0 and 1 where 0 is darkest black and 1 is lightest white.
     */
    double RelativeLuminance() const;

    /**
     * Compute the contrast ration between two colors using the formula from WCAG21.
     *
     * @param aLeft is the first color to use in the ratio
     * @param aRight is the second color to use in the ratio
     * @return contrast ratio between 1.0:1 and 21.0:1 between the two colors.
     */
    static double ContrastRatio( const COLOR4D& aLeft, const COLOR4D& aRight );

    /**
     * Returns a legacy color ID that is closest to the given 8-bit RGB values.
     */
    static EDA_COLOR_T FindNearestLegacyColor( int aR, int aG, int aB );

    // Color components: red, green, blue, alpha
    double r; ///< Red component
    double g; ///< Green component
    double b; ///< Blue component
    double a; ///< Alpha component

    // Optional text for evaluated colors
    std::optional<wxString> m_text;

    /// For legacy support; used as a value to indicate color hasn't been set yet
    static const COLOR4D UNSPECIFIED;

    // Declare a few color shortcuts that are used for comparisons frequently
    static const COLOR4D WHITE;
    static const COLOR4D BLACK;
    static const COLOR4D CLEAR;
};

/// @brief Equality operator, are two colors equal
KICOMMON_API bool operator==( const COLOR4D& lhs, const COLOR4D& rhs );

/// @brief Not equality operator, are two colors not equal
KICOMMON_API bool operator!=( const COLOR4D& lhs, const COLOR4D& rhs );

KICOMMON_API bool operator<( const COLOR4D& lhs, const COLOR4D& rhs );

/// Syntactic sugar for outputting colors to strings
KICOMMON_API std::ostream& operator<<( std::ostream& aStream, COLOR4D const& aColor );

// to allow json( COLOR4D )
KICOMMON_API void to_json( nlohmann::json& aJson, const COLOR4D& aColor );

// To allow json::get<COLOR4D>()
KICOMMON_API void from_json( const nlohmann::json& aJson, COLOR4D& aColor );

} // namespace KIGFX

template<>
struct KICOMMON_API std::hash<KIGFX::COLOR4D>
{
    std::size_t operator()( const KIGFX::COLOR4D& aColor ) const
    {
        return hash_val( aColor.r, aColor.b, aColor.g, aColor.a );
    }
};
