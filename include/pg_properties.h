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

class PROPERTY_BASE;
class REGEX_VALIDATOR;

wxPGProperty* PGPropertyFactory( const PROPERTY_BASE* aProperty );

///> Customized abstract wxPGProperty class to handle coordinate/size units
class PGPROPERTY_DISTANCE
{
public:
    PGPROPERTY_DISTANCE( const wxString& aRegEx );
    virtual ~PGPROPERTY_DISTANCE() = 0;

protected:
    bool StringToDistance( wxVariant& aVariant, const wxString& aText, int aArgFlags = 0 ) const;
    wxString DistanceToString( wxVariant& aVariant, int aArgFlags = 0 ) const;

    std::unique_ptr<REGEX_VALIDATOR> m_regExValidator;
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

#endif /* PG_PROPERTIES_H */
