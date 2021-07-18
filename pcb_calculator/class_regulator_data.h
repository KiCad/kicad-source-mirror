/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2011 jean-pierre.charras
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <refdes_utils.h>

#include <vector>

#include <wx/arrstr.h>

// Helper class to store parameters for a regulator
class REGULATOR_DATA
{
public:
    REGULATOR_DATA( const wxString& aName, double aVref, int aType, double aIadj = 0)
    {
        m_Type = aType;
        m_Vref = aVref;
        m_Name = aName;
        m_Iadj = aIadj;
    }

public:
    wxString m_Name;        // Regulator name
    int m_Type;             // type: with separate sense pin (normal) (=0)
                            // or adjustable 3 pins reg (=1)
    double m_Vref;          // Vreference in volt
    double m_Iadj;          // 3 pin type only: I adjust in micro amp
};

// Helper class to store the list of known regulators
class REGULATOR_LIST
{
public:
    REGULATOR_LIST() {};
    ~REGULATOR_LIST()
    {
        for( unsigned ii = 0; ii < m_List.size(); ii++ )
            delete m_List[ii];
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
            if( UTIL::RefDesStringCompare( aItem->m_Name, m_List[ii]->m_Name ) < 0 )
                break;
        }

        m_List.insert( m_List.begin() + ii, aItem );
    }

    REGULATOR_DATA* GetReg( const wxString& aName )
    {
        for( unsigned ii = 0; ii < m_List.size(); ii++ )
        {
            if( aName.CmpNoCase( m_List[ii]->m_Name ) == 0 )
            {
                return  m_List[ii];
            }
        }
        return nullptr;
    }

    void Remove( const wxString & aRegName )
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
        for( unsigned ii = 0; ii < m_List.size(); ii++ )
            list.Add( m_List[ii]->m_Name );

        return list;
    }

    std::vector <REGULATOR_DATA*> m_List;
};

#endif  // CLASS_REGULATOR_DATA_H
