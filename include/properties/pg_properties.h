/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PG_PROPERTIES_H
#define PG_PROPERTIES_H

#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/property.h>
#include <wx/propgrid/props.h>
#include <common.h>
#include <origin_transforms.h>

class PROPERTY_BASE;
class REGEX_VALIDATOR;

wxPGProperty* PGPropertyFactory( const PROPERTY_BASE* aProperty );

///> Customized abstract wxPGProperty class to handle coordinate/size units
class PGPROPERTY_DISTANCE
{
public:
    PGPROPERTY_DISTANCE( const wxString& aRegEx,
            ORIGIN_TRANSFORMS::COORD_TYPES_T aCoordType = ORIGIN_TRANSFORMS::NOT_A_COORD );
    virtual ~PGPROPERTY_DISTANCE() = 0;

    void SetCoordType( ORIGIN_TRANSFORMS::COORD_TYPES_T aType ) { m_coordType = aType; }
    ORIGIN_TRANSFORMS::COORD_TYPES_T CoordType() const { return m_coordType; }

protected:
    bool StringToDistance( wxVariant& aVariant, const wxString& aText, int aArgFlags = 0 ) const;
    wxString DistanceToString( wxVariant& aVariant, int aArgFlags = 0 ) const;

    std::unique_ptr<REGEX_VALIDATOR> m_regExValidator;
    ORIGIN_TRANSFORMS::COORD_TYPES_T m_coordType;
};


class PGPROPERTY_SIZE : public wxUIntProperty, public PGPROPERTY_DISTANCE
{
public:
    PGPROPERTY_SIZE( const wxString& aLabel = wxPG_LABEL, const wxString& aName = wxPG_LABEL,
            long aValue = 0 );

    bool StringToValue( wxVariant& aVariant, const wxString& aText, int aArgFlags = 0 ) const override
    {
        return StringToDistance( aVariant, aText, aArgFlags );
    }

    wxString ValueToString( wxVariant& aVariant, int aArgFlags = 0 ) const override
    {
        return DistanceToString( aVariant, aArgFlags );
    }

    wxValidator* DoGetValidator() const override;
};


class PGPROPERTY_COORD : public wxIntProperty, public PGPROPERTY_DISTANCE
{
public:
    PGPROPERTY_COORD( const wxString& aLabel = wxPG_LABEL, const wxString& aName = wxPG_LABEL,
                      long aValue = 0,
                      ORIGIN_TRANSFORMS::COORD_TYPES_T aCoordType = ORIGIN_TRANSFORMS::NOT_A_COORD );

    bool StringToValue( wxVariant& aVariant, const wxString& aText, int aArgFlags = 0 ) const override
    {
        return StringToDistance( aVariant, aText, aArgFlags );
    }

    wxString ValueToString( wxVariant& aVariant, int aArgFlags = 0 ) const override
    {
        return DistanceToString( aVariant, aArgFlags );
    }

    wxValidator* DoGetValidator() const override;
};


///> Customized wxPGProperty class to handle angles
class PGPROPERTY_ANGLE : public wxFloatProperty
{
public:
    PGPROPERTY_ANGLE( const wxString& aLabel = wxPG_LABEL, const wxString& aName = wxPG_LABEL,
            double aValue = 0 )
        : wxFloatProperty( aLabel, aName, aValue ), m_scale( 1.0 )
    {
    }

    bool StringToValue( wxVariant& aVariant, const wxString& aText, int aArgFlags = 0 ) const override;
    wxString ValueToString( wxVariant& aVariant, int aArgFlags = 0 ) const override;

    void SetScale( double aScale )
    {
        m_scale = aScale;
    }

protected:
    ///> Scale factor to convert between raw and displayed value
    double m_scale;
};


///> A wxEnumProperty that displays a color next to the enum value
class PGPROPERTY_COLORENUM : public wxEnumProperty
{
public:
    PGPROPERTY_COLORENUM( const wxString& aLabel, wxString& aName, const wxPGChoices& aChoices,
                          int aValue = 0 ) :
            wxEnumProperty( aLabel, aName, const_cast<wxPGChoices&>( aChoices ), aValue ),
            m_colorFunc( []( const wxString& aChoice ) { return wxNullColour; } )
    {
        SetFlag( wxPG_PROP_CUSTOMIMAGE );
    }

    wxSize OnMeasureImage( int aItem = -1 ) const override;

    void OnCustomPaint( wxDC& aDC, const wxRect& aRect, wxPGPaintData& aPaintData ) override;

    void SetColorFunc( std::function<wxColour( const wxString& aChoice )> aFunc )
    {
        m_colorFunc = aFunc;
    }

    wxColour GetColor( const wxString& aChoice )
    {
        return m_colorFunc( aChoice );
    }

protected:
    std::function<wxColour( const wxString& aChoice )> m_colorFunc;
};


class PGPROPERTY_STRING : public wxStringProperty
{
public:
    PGPROPERTY_STRING( const wxString& aLabel = wxPG_LABEL, const wxString& aName = wxPG_LABEL,
                       const wxString& aValue = wxEmptyString ) :
            wxStringProperty( aLabel, aName, aValue )
    {}

    virtual ~PGPROPERTY_STRING() = default;

    wxString ValueToString( wxVariant& aValue, int aFlags = 0 ) const override;

    bool StringToValue( wxVariant& aVariant, const wxString& aString,
                        int aFlags = 0 ) const override;
};

#endif /* PG_PROPERTIES_H */
