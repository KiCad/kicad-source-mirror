/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PG_PROPERTIES_H
#define PG_PROPERTIES_H

#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/property.h>
#include <wx/propgrid/props.h>
#include <wx/propgrid/advprops.h>
#include <common.h>
#include <gal/color4d.h>
#include <origin_transforms.h>

class PROPERTY_BASE;
class REGEX_VALIDATOR;
class EDA_DRAW_FRAME;

wxPGProperty* PGPropertyFactory( const PROPERTY_BASE* aProperty, EDA_DRAW_FRAME* aFrame );

///< Customized abstract wxPGProperty class to handle coordinate/size units
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

    bool StringToValue( wxVariant& aVariant, const wxString& aText,
                        int aArgFlags = 0 ) const override
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

    bool StringToValue( wxVariant& aVariant, const wxString& aText,
                        int aArgFlags = 0 ) const override
    {
        return StringToDistance( aVariant, aText, aArgFlags );
    }

    wxString ValueToString( wxVariant& aVariant, int aArgFlags = 0 ) const override
    {
        return DistanceToString( aVariant, aArgFlags );
    }

    wxValidator* DoGetValidator() const override;
};


///< Customized wxPGProperty class to handle angles
class PGPROPERTY_ANGLE : public wxFloatProperty
{
public:
    PGPROPERTY_ANGLE( const wxString& aLabel = wxPG_LABEL, const wxString& aName = wxPG_LABEL,
            double aValue = 0 )
        : wxFloatProperty( aLabel, aName, aValue ), m_scale( 1.0 )
    {
    }

    bool StringToValue( wxVariant& aVariant, const wxString& aText,
                        int aArgFlags = 0 ) const override;
    wxString ValueToString( wxVariant& aVariant, int aArgFlags = 0 ) const override;

    void SetScale( double aScale )
    {
        m_scale = aScale;
    }

    wxValidator* DoGetValidator() const override;

    ///< Do not perform PG validation; the UX is not what we want.
    bool ValidateValue( wxVariant&, wxPGValidationInfo& ) const override { return true; }

protected:
    ///< Scale factor to convert between raw and displayed value
    double m_scale;
};


///< A wxEnumProperty that displays a color next to the enum value
class PGPROPERTY_COLORENUM : public wxEnumProperty
{
public:
    PGPROPERTY_COLORENUM( const wxString& aLabel, const wxString& aName, wxPGChoices* aChoices,
                          int aValue = 0 ) :
            wxEnumProperty( aLabel, aName, *aChoices, aValue ),
            m_colorFunc( []( int aDummy ) { return wxNullColour; } )
    {
        SetFlag( wxPG_PROP_CUSTOMIMAGE );
    }

    wxSize OnMeasureImage( int aItem = -1 ) const override;

    void OnCustomPaint( wxDC& aDC, const wxRect& aRect, wxPGPaintData& aPaintData ) override;

    void SetColorFunc( std::function<wxColour( int aValue )> aFunc )
    {
        m_colorFunc = aFunc;
    }

    wxColour GetColor( int aValue )
    {
        return m_colorFunc( aValue );
    }

protected:
    std::function<wxColour( int aValue )> m_colorFunc;
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


class PGPROPERTY_BOOL : public wxBoolProperty
{
public:
    PGPROPERTY_BOOL( const wxString& aLabel = wxPG_LABEL, const wxString& aName = wxPG_LABEL,
                     bool aValue = false );

    virtual ~PGPROPERTY_BOOL() = default;

    const wxPGEditor* DoGetEditorClass() const override;
};


class PGPROPERTY_COLOR4D : public wxStringProperty
{
public:
    PGPROPERTY_COLOR4D( const wxString& aLabel = wxPG_LABEL, const wxString& aName = wxPG_LABEL,
                        KIGFX::COLOR4D aValue = KIGFX::COLOR4D::UNSPECIFIED,
                        KIGFX::COLOR4D aBackground = KIGFX::COLOR4D::UNSPECIFIED );

    virtual ~PGPROPERTY_COLOR4D() = default;

    wxString ValueToString( wxVariant& aValue, int aFlags = 0 ) const override;

     bool StringToValue( wxVariant &aVariant, const wxString &aText,
                        int aFlags = 0 ) const override;

     void SetBackgroundColor( const KIGFX::COLOR4D& aColor ) { m_backgroundColor = aColor; }
     const KIGFX::COLOR4D& GetBackgroundColor() const { return m_backgroundColor; }

 private:
     /// Used for rendering colors with opacity
     KIGFX::COLOR4D m_backgroundColor;
};

#endif /* PG_PROPERTIES_H */
