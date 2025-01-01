/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2011 jean-pierre.charras
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

/**
 * @file class_regulator_data.h
 * Contains structures for storage of regulator data
 */

#ifndef CLASS_REGULATOR_DATA_H
#define CLASS_REGULATOR_DATA_H

#include <string_utils.h>

#include <vector>

#include <wx/arrstr.h>

// Helper class to store parameters for a regulator
class REGULATOR_DATA
{
public:
    REGULATOR_DATA( const wxString& aName, double aVrefMin, double aVrefTyp, double aVrefMax,
                    int aType, double aIadjTyp = 0, double aIadjMax = 0 ) :
            m_Name( aName ),
            m_Type( aType ), m_VrefMin( aVrefMin ), m_VrefTyp( aVrefTyp ), m_VrefMax( aVrefMax ),
            m_IadjTyp( aIadjTyp ), m_IadjMax( aIadjMax )
    {
    }

public:
    wxString m_Name;        // Regulator name
    int m_Type;             // type: with separate sense pin (normal) (=0)
                            // or adjustable 3 pins reg (=1)
    double m_VrefMin; // min Vreference in volt
    double m_VrefTyp; // typ Vreference in volt
    double m_VrefMax; // max Vreference in volt
    double m_IadjTyp; // 3 pin type only: typ I adjust in micro amp
    double m_IadjMax; // 3 pin type only: max I adjust in micro amp
};

// Helper class to store the list of known regulators
class REGULATOR_LIST
{
public:
    REGULATOR_LIST() {}
    ~REGULATOR_LIST()
    {
        for( REGULATOR_DATA* regulator : m_List )
            delete regulator;
    }

    unsigned int GetCount()
    {
        return m_List.size();
    }

    void Add( REGULATOR_DATA* aItem )
    {
        // add new item an try to keep alphabetic order,
        // and because name have numbers inside, use a KiCad compare function
        // that handles number as numbers not ASCII chars
        unsigned ii = 0;

        for( ; ii < m_List.size(); ii++ )
        {
            if( StrNumCmp( aItem->m_Name, m_List[ii]->m_Name, true ) < 0 )
                break;
        }

        m_List.insert( m_List.begin() + ii, aItem );
    }

    REGULATOR_DATA* GetReg( const wxString& aName )
    {
        for( REGULATOR_DATA* regulator : m_List )
        {
            if( aName.CmpNoCase( regulator->m_Name ) == 0 )
                return regulator;
        }
        return nullptr;
    }

    void Remove( const wxString& aRegName )
    {
        for( unsigned ii = 0; ii < m_List.size(); ii++ )
        {
            if( aRegName.CmpNoCase( m_List[ii]->m_Name ) == 0 )
            {
                // Found! remove it
                m_List.erase( m_List.begin() + ii );
                break;
            }
        }
    }

    /**
     * Replace an old REGULATOR_DATA by a new one
     * The old one is deleted
     * the 2 items must have the same name
     */
    void Replace( REGULATOR_DATA* aItem )
    {
        // Search for the old regulator
        for( unsigned ii = 0; ii < m_List.size(); ii++ )
        {
            if( aItem->m_Name.CmpNoCase( m_List[ii]->m_Name ) == 0 )
            {
                // Found! remove it
                delete m_List[ii];
                m_List[ii] = aItem;
                break;
            }
        }
    }

    wxArrayString GetRegList() const
    {
        wxArrayString list;

        for( REGULATOR_DATA* regulator : m_List )
            list.Add( regulator->m_Name );

        return list;
    }

    void Clear() { m_List.clear(); }

    std::vector <REGULATOR_DATA*> m_List;
};

#endif  // CLASS_REGULATOR_DATA_H
