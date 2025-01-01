/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef SPICE_VALUE_H
#define SPICE_VALUE_H

#include <wx/string.h>
#include <wx/valtext.h>


/**
 * A SPICE_VALUE_FORMAT holds precision and range info for formatting values.
 *
 * The Precision field indicates the number of significant digits to show in the result.
 *
 * The range field gives the SI unit prefix for the range or '~' for auto-range, and the
 * units.  For instance "mV" or "~Hz".
 */
struct SPICE_VALUE_FORMAT
{
    void FromString( const wxString& aString );
    wxString ToString() const;

    void UpdateUnits( const wxString& aUnits );

    int      Precision;
    wxString Range;
};


///< Helper class to handle Spice way of expressing values (e.g. 10.5 Meg)
class SPICE_VALUE
{
public:
    enum UNIT_PREFIX
    {
        PFX_FEMTO   = -15,
        PFX_PICO    = -12,
        PFX_NANO    = -9,
        PFX_MICRO   = -6,
        PFX_MILI    = -3,
        PFX_NONE    = 0,
        PFX_KILO    = 3,
        PFX_MEGA    = 6,
        PFX_GIGA    = 9,
        PFX_TERA    = 12
    };

    SPICE_VALUE()
        : m_base( 0 ), m_prefix( PFX_NONE ), m_spiceStr( false )
    {
    }

    ///< Parses the string to create a Spice value (e.g. 100n)
    SPICE_VALUE( const wxString& aString );

    SPICE_VALUE( int aInt, UNIT_PREFIX aPrefix = PFX_NONE )
        : m_base( aInt ), m_prefix( aPrefix ), m_spiceStr( false )
    {
        Normalize();
    }

    SPICE_VALUE( double aDouble, UNIT_PREFIX aPrefix = PFX_NONE )
        : m_base( aDouble ), m_prefix( aPrefix ), m_spiceStr( false )
    {
        Normalize();
    }

    /**
     * Normalize the value. The unit prefix is picked so the base is (0.001 <= base < 1000).
     */
    void Normalize();

    double ToNormalizedDouble( wxString* aPrefix );

    double ToDouble() const;

    /**
     * Return string value as when converting double to string (e.g. 123456.789).
     */
    wxString ToString() const;

    /**
     * Return string value with a particular precision and range.
     * @param aPrecision number of significant digits
     * @param aRange "~" + unit for autorage; otherwise SI prefix + unit
     */
    wxString ToString( const SPICE_VALUE_FORMAT& aFormat );

    /**
     * Return string value in Spice format (e.g. 123.3456789k).
     */
    wxString ToSpiceString() const;

    /**
     * Return either a normal string or Spice format string, depending on the original
     * value format.
     */
    wxString ToOrigString() const
    {
        return m_spiceStr ? ToSpiceString() : ToString();
    }

    /**
     * Return true if the object was initiated with a Spice formatted string value.
     */
    bool IsSpiceString() const
    {
        return m_spiceStr;
    }

    bool operator==( const SPICE_VALUE& aOther ) const
    {
        return ( m_prefix == aOther.m_prefix && m_base == aOther.m_base );
    }

    bool operator>( const SPICE_VALUE& aOther ) const
    {
        return this->ToDouble() > aOther.ToDouble();
    }

    bool operator<( const SPICE_VALUE& aOther ) const
    {
        return this->ToDouble() < aOther.ToDouble();
    }

    bool operator>=( const SPICE_VALUE& aOther ) const
    {
        return ( *this == aOther || *this > aOther );
    }

    bool operator<=( const SPICE_VALUE& aOther ) const
    {
        return ( *this == aOther || *this < aOther );
    }

    SPICE_VALUE operator-( const SPICE_VALUE& aOther ) const;
    SPICE_VALUE operator+( const SPICE_VALUE& aOther ) const;
    SPICE_VALUE operator*( const SPICE_VALUE& aOther ) const;
    SPICE_VALUE operator/( const SPICE_VALUE& aOther ) const;

    ///< Remove redundant zeros from the end of a string.
    static void StripZeros( wxString& aString );

    static UNIT_PREFIX ParseSIPrefix( wxChar c );

private:
    double      m_base;
    UNIT_PREFIX m_prefix;

    ///< Was the value defined using the Spice notation?
    bool        m_spiceStr;
};


///< Helper class to recognize Spice formatted values
class SPICE_VALIDATOR : public wxTextValidator
{
public:
    SPICE_VALIDATOR( bool aEmptyAllowed = false )
        : m_emptyAllowed( aEmptyAllowed )
    {
    }

    wxObject* Clone() const override
    {
        return new SPICE_VALIDATOR( *this );
    }

    bool Validate( wxWindow* aParent ) override;

private:
    ///< Is it valid to get an empty value?
    bool m_emptyAllowed;
};

#endif /* SPICE_VALUE_H */
