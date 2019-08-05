/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright (C) 2017-2019 Kicad Developers, see AUTHORS.txt for contributors.
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

#ifndef COLOR4D_H_
#define COLOR4D_H_

#include <colors.h>
#include <cassert>

namespace KIGFX
{
/**
 * Class COLOR4D
 * is the color representation with 4 components: red, green, blue, alpha.
 */
class COLOR4D
{
public:
    // Constructor (creates the Color 0,0,0,0)
    COLOR4D() :
        r( 0 ), g( 0 ), b( 0 ), a( 1.0 )
    {
    }

    /**
     * @brief Constructor
     *
     * @param aRed   is the red component   [0.0 .. 1.0].
     * @param aGreen is the green component [0.0 .. 1.0].
     * @param aBlue  is the blue component  [0.0 .. 1.0].
     * @param aAlpha is the alpha value     [0.0 .. 1.0].
     */
    constexpr COLOR4D( double aRed, double aGreen, double aBlue, double aAlpha ) :
        r( aRed ), g( aGreen ), b( aBlue ), a( aAlpha )
    {
        assert( r >= 0.0 && r <= 1.0 );
        assert( g >= 0.0 && g <= 1.0 );
        assert( b >= 0.0 && b <= 1.0 );
        assert( a >= 0.0 && a <= 1.0 );
    }

    /**
     * @brief Constructor
     *
     * @param aColor is one of KiCad's palette colors.
     * @see EDA_COLOR_T
     */
    COLOR4D( EDA_COLOR_T aColor );

#ifdef WX_COMPATIBILITY
    /**
     * @brief Constructor
     *
     * @param aColor is the color type used by wxWidgets.
     */
    COLOR4D( const wxColour& aColor );

    /**
     * Function SetFromWxString
     * Sets color values by parsing a string using wxColour::Set()
     *
     * @param aColorString is a color string that wxColour can understand
     * @return true if color was set successfully
     */
    bool SetFromWxString( const wxString& aColorString );

    wxString ToWxString( long flags ) const;

    wxColour ToColour() const;

    /**
     * Function LegacyMix()
     * Mixes this COLOR4D with an input COLOR4D using the OR-mixing of legacy canvas.
     *
     * Can be removed once legacy canvas is removed.
     * Depends on wxColour for simplicity, but could be re-written to avoid
     * this dependency if desired.
     *
     * @param aColor The color to mix with this one
     */
    COLOR4D LegacyMix( COLOR4D aColor ) const;

    /**
     * Packs the color into an unsigned int for compatibility with legacy canvas.
     * Note that this is a lossy downsampling and also that the alpha channel is lost.
     */
    unsigned int ToU32() const;

    /**
     * Unpacks from a unsigned int in the legacy EDA_COLOR_T format.
     */
    void FromU32( unsigned int aPackedColor );
#endif /* WX_COMPATIBLITY */


    /**
     * Function ToHSL()
     * Converts current color (stored in RGB) to HSL format.
     *
     * @param aOutHue is the conversion result for hue component, in degrees 0 ... 360.0
     * @param aOutSaturation is the conversion result for saturation component (0 ... 1.0).
     * @param aOutLightness is conversion result for value component (0 ... 1.0).
     * @note saturation is set to 0.0 for black color if r = g = b,
     */
    void ToHSL( double& aOutHue, double& aOutSaturation, double& aOutValue ) const;

    /**
     * Function FromHSL()
     * Changes currently used color to the one given by hue, saturation and lightness parameters.
     *
     * @param aInHue is hue component, in degrees (0.0 - 360.0)
     * @param aInSaturation is saturation component (0.0 - 1.0)
     * @param aInLightness is lightness component (0.0 - 1.0)
     */
    void FromHSL( double aInHue, double aInSaturation, double aInLightness );

    /**
     * Function Brighten
     * Makes the color brighter by a given factor.
     * @param aFactor Specifies how bright the color should become (valid values: 0.0 .. 1.0).
     * @return COLOR4D& Brightened color.
     */
    COLOR4D& Brighten( double aFactor )
    {
        assert( aFactor >= 0.0 && aFactor <= 1.0 );

        r = r * ( 1.0 - aFactor ) + aFactor;
        g = g * ( 1.0 - aFactor ) + aFactor;
        b = b * ( 1.0 - aFactor ) + aFactor;

        return *this;
    }

    /**
     * Function Darken
     * Makes the color darker by a given factor.
     * @param aFactor Specifies how dark the color should become (valid values: 0.0 .. 1.0).
     * @return COLOR4D& Darkened color.
     */
    COLOR4D& Darken( double aFactor )
    {
        assert( aFactor >= 0.0 && aFactor <= 1.0 );

        r = r * ( 1.0 - aFactor );
        g = g * ( 1.0 - aFactor );
        b = b * ( 1.0 - aFactor );

        return *this;
    }

    /**
     * Function Invert
     * Makes the color inverted, alpha remains the same.
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
     * Function Brightened
     * Returns a color that is brighter by a given factor, without modifying object.
     * @param aFactor Specifies how bright the color should become (valid values: 0.0 .. 1.0).
     * @return COLOR4D Highlighted color.
     */
    COLOR4D Brightened( double aFactor ) const
    {
        assert( aFactor >= 0.0 && aFactor <= 1.0 );

        return COLOR4D( r * ( 1.0 - aFactor ) + aFactor,
                        g * ( 1.0 - aFactor ) + aFactor,
                        b * ( 1.0 - aFactor ) + aFactor,
                        a );
    }

    /**
     * Function Darkened
     * Returns a color that is darker by a given factor, without modifying object.
     * @param aFactor Specifies how dark the color should become (valid values: 0.0 .. 1.0).
     * @return COLOR4D Darkened color.
     */
    COLOR4D Darkened( double aFactor ) const
    {
        assert( aFactor >= 0.0 && aFactor <= 1.0 );

        return COLOR4D( r * ( 1.0 - aFactor ),
                        g * ( 1.0 - aFactor ),
                        b * ( 1.0 - aFactor ),
                        a );
    }

    /**
     * Function Mix
     * Returns a color that is mixed with the input by a factor
     * @param aFactor Specifies how much of the original color to keep (valid values: 0.0 .. 1.0).
     * @return COLOR4D Mixed color.
     */
    COLOR4D Mix( const COLOR4D& aColor, double aFactor ) const
    {
        assert( aFactor >= 0.0 && aFactor <= 1.0 );

        return COLOR4D( aColor.r * ( 1.0 - aFactor ) + r * aFactor,
                        aColor.g * ( 1.0 - aFactor ) + g * aFactor,
                        aColor.b * ( 1.0 - aFactor ) + b * aFactor,
                        a );
    }

    /**
     * Function WithAlpha
     * Returns a colour with the same colour, but the given alpha
     * @param aAlpha specifies the alpha of the new color
     * @return COLOR4D color with that alpha
     */
     COLOR4D WithAlpha( double aAlpha ) const
     {
         assert( aAlpha >= 0.0 && aAlpha <= 1.0 );

         return COLOR4D( r, g, b, aAlpha );
     }

    /**
     * Function Inverted
     * Returns an inverted color, alpha remains the same.
     * @return COLOR4D& Inverted color.
     */
    COLOR4D Inverted() const
    {
        return COLOR4D( 1.0 - r, 1.0 - g, 1.0 - b, a );
    }

    /**
     * Function GetBrightness
     * Returns the brightness value of the color ranged from 0.0 to 1.0.
     * @return The brightness value.
     */
    double GetBrightness() const
    {
        // Weighted W3C formula
        return r * 0.299 + g * 0.587 + b * 0.117;
    }

    /**
     * Function ToHSV()
     * Converts current color (stored in RGB) to HSV format.
     *
     * @param aOutHue is the conversion result for hue component, in degrees 0 ... 360.0
     * @param aOutSaturation is the conversion result for saturation component (0 ... 1.0).
     * @param aOutValue is conversion result for value component (0 ... 1.0).
     * @param aAlwaysDefineHue controls the way hue is defined when r = v = b
     * @note saturation is set to 0.0 for black color (r = v = b = 0), and if r = v = b,
     * hue is set to 0.0 if aAlwaysDefineHue = true, and set to NAN if aAlwaysDefineHue = false.
     * this option is usefull to convert a 4D color to a legacy color, because Red has hue = 0,
     * therefore aAlwaysDefineHue = false makes difference between Red and Gray colors.
     */
    void ToHSV( double& aOutHue, double& aOutSaturation, double& aOutValue, bool aAlwaysDefineHue = false ) const;

    /**
     * Function FromHSV()
     * Changes currently used color to the one given by hue, saturation and value parameters.
     *
     * @param aInH is hue component, in degrees.
     * @param aInS is saturation component.
     * @param aInV is value component.
     */
    void FromHSV( double aInH, double aInS, double aInV );

    // Color components: red, green, blue, alpha
    double r; ///< Red component
    double g; ///< Green component
    double b; ///< Blue component
    double a; ///< Alpha component

    /// For legacy support; used as a value to indicate color hasn't been set yet
    static const COLOR4D UNSPECIFIED;

    // Declare a few color shortcuts that are used for comparisons frequently
    static const COLOR4D WHITE;
    static const COLOR4D BLACK;
};

/// @brief Equality operator, are two colors equal
const bool operator==( const COLOR4D& lhs, const COLOR4D& rhs );

/// @brief Not equality operator, are two colors not equal
const bool operator!=( const COLOR4D& lhs, const COLOR4D& rhs );

/// Syntactic sugar for outputting colors to strings
std::ostream &operator<<( std::ostream &aStream, COLOR4D const &aColor );

} // namespace KIGFX

#endif /* COLOR4D_H_ */
