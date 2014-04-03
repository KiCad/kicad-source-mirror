/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2014 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <hotkey_grid_table.h>

/*
 *  Reads the hotkey table from its stored format into a format suitable
 *  for a wxGrid.
 */
HOTKEY_EDITOR_GRID_TABLE::HOTKEY_EDITOR_GRID_TABLE( struct EDA_HOTKEY_CONFIG* origin ) :
    wxGridTableBase(), m_hotkeys()
{
    EDA_HOTKEY_CONFIG* section;

    for( section = origin; section->m_HK_InfoList; section++ )
    {
        // Add a dummy hotkey_spec which is a header before each hotkey list
        hotkey_spec spec( *section->m_SectionTag, NULL );
        m_hotkeys.push_back( spec );

        EDA_HOTKEY** hotkey_descr_list;

        // Add hotkeys descr
        for( hotkey_descr_list = section->m_HK_InfoList; *hotkey_descr_list;
             hotkey_descr_list++ )
        {
            EDA_HOTKEY* hotkey_descr = *hotkey_descr_list;
            hotkey_spec    spec( *section->m_SectionTag, new EDA_HOTKEY( hotkey_descr ) );
            m_hotkeys.push_back( spec );
        }
    }
}


HOTKEY_EDITOR_GRID_TABLE::hotkey_spec_vector& HOTKEY_EDITOR_GRID_TABLE::getHotkeys()
{
    return m_hotkeys;
}


int HOTKEY_EDITOR_GRID_TABLE::GetNumberRows()
{
    return m_hotkeys.size();
}


int HOTKEY_EDITOR_GRID_TABLE::GetNumberCols()
{
    return 2;
}


bool HOTKEY_EDITOR_GRID_TABLE::IsEmptyCell( int row, int col )
{
    return col == 1 && m_hotkeys[row].second == NULL;
}


wxString HOTKEY_EDITOR_GRID_TABLE::GetValue( int row, int col )
{
    EDA_HOTKEY* hotkey_descr = m_hotkeys[row].second;

    if( col == 0 )
    {
        if( hotkey_descr == NULL )
        {
            // section header
            return m_hotkeys[row].first;
        }
        else
        {
            return hotkey_descr->m_InfoMsg;
        }
    }
    else
    {
        if( hotkey_descr == NULL )
        {
            // section header
            return wxEmptyString;
        }
        else
        {
            return KeyNameFromKeyCode( hotkey_descr->m_KeyCode );
        }
    }
}


void HOTKEY_EDITOR_GRID_TABLE::SetValue( int row, int col, const wxString& value )
{
}


wxString HOTKEY_EDITOR_GRID_TABLE::GetTypeName( int row, int col )
{
    return wxGRID_VALUE_STRING;
}


bool HOTKEY_EDITOR_GRID_TABLE::CanGetValueAs( int row, int col, const wxString& typeName )
{
    return typeName == wxGRID_VALUE_STRING && col == 2;
}


bool HOTKEY_EDITOR_GRID_TABLE::CanSetValueAs( int row, int col, const wxString& typeName )
{
    return false;
}


long HOTKEY_EDITOR_GRID_TABLE::GetValueAsLong( int row, int col )
{
    return -1L;
}


double HOTKEY_EDITOR_GRID_TABLE::GetValueAsDouble( int row, int col )
{
    return 0.0;
}


bool HOTKEY_EDITOR_GRID_TABLE::GetValueAsBool( int row, int col )
{
    return false;
}


void HOTKEY_EDITOR_GRID_TABLE::SetValueAsLong( int row, int col, long value )
{
}


void HOTKEY_EDITOR_GRID_TABLE::SetValueAsDouble( int row, int col, double value )
{
}


void HOTKEY_EDITOR_GRID_TABLE::SetValueAsBool( int row, int col, bool value )
{
}


void* HOTKEY_EDITOR_GRID_TABLE::GetValueAsCustom( int row, int col )
{
    return 0;
}


void HOTKEY_EDITOR_GRID_TABLE::SetValueAsCustom( int row, int col, void* value )
{
}


wxString HOTKEY_EDITOR_GRID_TABLE::GetColLabelValue( int col )
{
    return col == 0 ? _( "Command" ) : _( "Hotkey" );
}


bool HOTKEY_EDITOR_GRID_TABLE::IsHeader( int row )
{
    return m_hotkeys[row].second == NULL;
}


void HOTKEY_EDITOR_GRID_TABLE::SetKeyCode( int row, long key )
{
    m_hotkeys[row].second->m_KeyCode = key;
}


void HOTKEY_EDITOR_GRID_TABLE::RestoreFrom( struct EDA_HOTKEY_CONFIG* origin )
{
    int row = 0;
    EDA_HOTKEY_CONFIG* section;

    for( section = origin; section->m_HK_InfoList; section++ )
    {
        ++row;      // Skip header
        EDA_HOTKEY** info_ptr;

        for( info_ptr = section->m_HK_InfoList; *info_ptr; info_ptr++ )
        {
            EDA_HOTKEY* info = *info_ptr;
            m_hotkeys[row++].second->m_KeyCode = info->m_KeyCode;
        }
    }
}


HOTKEY_EDITOR_GRID_TABLE::~HOTKEY_EDITOR_GRID_TABLE()
{
    hotkey_spec_vector::iterator i;

    for( i = m_hotkeys.begin(); i != m_hotkeys.end(); ++i )
        delete i->second;
}
