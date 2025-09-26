/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Author: SYSUEric <jzzhuang666@gmail.com>.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef _ODB_COMPONENT_H_
#define _ODB_COMPONENT_H_

#include "odb_util.h"
#include <list>
#include <set>
#include <wx/string.h>
#include "odb_attribute.h"
#include "odb_eda_data.h"

class ODB_COMPONENT;
class COMPONENTS_MANAGER : public ATTR_MANAGER
{
public:
    COMPONENTS_MANAGER() = default;

    virtual ~COMPONENTS_MANAGER() { m_compList.clear(); }

    ODB_COMPONENT& AddComponent( const FOOTPRINT* aFp, const EDA_DATA::PACKAGE& aPkg );

    void Write( std::ostream& ost ) const;

private:
    std::list<ODB_COMPONENT> m_compList;
    std::set<wxString>       m_usedCompNames;
};


class ODB_COMPONENT : public ATTR_RECORD_WRITER
{
public:
    ODB_COMPONENT( size_t aIndex, size_t r ) : m_index( aIndex ), m_pkg_ref( r ) {}

    const size_t m_index;   ///<! CMP index number on board to be used in SNT(TOP), 0~n-1
    size_t       m_pkg_ref; ///<! package ref number from PKG in eda/data file, 0~n-1
    std::pair<wxString, wxString> m_center;
    wxString                      m_rot = wxT( "0" );
    wxString                      m_mirror = wxT( "N" );

    wxString m_comp_name; ///<! Unique reference designator (component name)

    wxString
            m_part_name; ///<! Part identification is a single string of ASCII characters without spaces

    std::map<wxString, wxString> m_prp; // !< Component Property Record

    struct TOEPRINT
    {
    public:
        TOEPRINT( const EDA_DATA::PIN& pin ) :
                m_pin_num( pin.m_index ), m_toeprint_name( pin.m_name )
        {
        }

        const size_t m_pin_num; ///<! index of PIN record in the eda/data file, 0~n-1.

        std::pair<wxString, wxString> m_center; ///<! Board location of the pin.

        wxString m_rot; ///<! Rotation, clockwise, it equals to the actual PAD rotation,
                        ///<! not CMP m_rot.

        wxString m_mirror; ///<! equal to CMP m_mirror.

        size_t m_net_num = 0; ///<! Number of NET record in the eda/data file.

        size_t m_subnet_num = 0; ///<! Number of subnet (SNT record TOP) in the referenced net

        wxString m_toeprint_name; ///<! Name of the pad in PIN record

        void Write( std::ostream& ost ) const;
    };


    std::list<TOEPRINT> m_toeprints;

    void Write( std::ostream& ost ) const;
};


#endif // _ODB_COMPONENT_H_