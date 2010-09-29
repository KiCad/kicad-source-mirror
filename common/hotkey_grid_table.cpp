#include "hotkey_grid_table.h"

/*
 *  Reads the hotkey table from its stored format into a format suitable
 *  for a wxGrid.
 */
HotkeyGridTable::HotkeyGridTable( struct
                                  Ki_HotkeyInfoSectionDescriptor* origin ) :
    wxGridTableBase(),
    m_hotkeys()
{
    Ki_HotkeyInfoSectionDescriptor* section;

    for( section = origin; section->m_HK_InfoList; section++ )
    {
        hotkey_spec     spec( *section->m_SectionTag, new Ki_HotkeyInfo( NULL, 0, 0 ) );
        m_hotkeys.push_back( spec );

        Ki_HotkeyInfo** info_ptr;
        for( info_ptr = section->m_HK_InfoList; *info_ptr; info_ptr++ )
        {
            Ki_HotkeyInfo* info = *info_ptr;
            hotkey_spec    spec( *section->m_SectionTag,
                                new Ki_HotkeyInfo( info ) );
            m_hotkeys.push_back( spec );
        }
    }
}


HotkeyGridTable::hotkey_spec_vector& HotkeyGridTable::getHotkeys()
{
    return m_hotkeys;
}


int HotkeyGridTable::GetNumberRows()
{
    return m_hotkeys.size();
}


int HotkeyGridTable::GetNumberCols()
{
    return 2;
}


bool HotkeyGridTable::IsEmptyCell( int row, int col )
{
    return col == 1 && m_hotkeys[row].second == 0;
}


wxString HotkeyGridTable::GetValue( int row, int col )
{
    if( col == 0 )
    {
        if( m_hotkeys[row].second == 0 )
        {
            // section header
            return m_hotkeys[row].first;
        }
        else
        {
            return m_hotkeys[row].second->m_InfoMsg;
        }
    }
    else
    {
        if( m_hotkeys[row].second == 0 )
        {
            return wxString();
        }
        else
        {
            return ReturnKeyNameFromKeyCode( m_hotkeys[row].second->m_KeyCode );
        }
    }
}


void HotkeyGridTable::SetValue( int row, int col, const wxString& value )
{
}


wxString HotkeyGridTable::GetTypeName( int row, int col )
{
    return wxGRID_VALUE_STRING;
}


bool HotkeyGridTable::CanGetValueAs( int row, int col, const wxString& typeName )
{
    return typeName == wxGRID_VALUE_STRING && col == 2;
}


bool HotkeyGridTable::CanSetValueAs( int row, int col, const wxString& typeName )
{
    return false;
}


long HotkeyGridTable::GetValueAsLong( int row, int col )
{
    return -1L;
}


double HotkeyGridTable::GetValueAsDouble( int row, int col )
{
    return 0.0;
}


bool HotkeyGridTable::GetValueAsBool( int row, int col )
{
    return false;
}


void HotkeyGridTable::SetValueAsLong( int row, int col, long value )
{
}


void HotkeyGridTable::SetValueAsDouble( int row, int col, double value )
{
}


void HotkeyGridTable::SetValueAsBool( int row, int col, bool value )
{
}


void* HotkeyGridTable::GetValueAsCustom( int row, int col )
{
    return 0;
}


void HotkeyGridTable::SetValueAsCustom( int row, int col, void* value )
{
}


wxString HotkeyGridTable::GetColLabelValue( int col )
{
    return col == 0 ? _( "Command" ) : _( "Hotkey" );
}


bool HotkeyGridTable::isHeader( int row )
{
    return m_hotkeys[row].second == 0;
}


void HotkeyGridTable::SetKeyCode( int row, long key )
{
    m_hotkeys[row].second->m_KeyCode = key;
}


void HotkeyGridTable::RestoreFrom( struct
                                   Ki_HotkeyInfoSectionDescriptor* origin )
{
    int row = 0;
    Ki_HotkeyInfoSectionDescriptor* section;

    for( section = origin; section->m_HK_InfoList; section++ )
    {
        ++row;
        Ki_HotkeyInfo** info_ptr;
        for( info_ptr = section->m_HK_InfoList; *info_ptr; info_ptr++ )
        {
            Ki_HotkeyInfo* info = *info_ptr;
            m_hotkeys[row++].second->m_KeyCode = info->m_KeyCode;
        }
    }
}


HotkeyGridTable::~HotkeyGridTable()
{
    hotkey_spec_vector::iterator i;

    for( i = m_hotkeys.begin(); i != m_hotkeys.end(); ++i )
    {
        if( i->second )
        {
            delete i->second;
        }
    }
}
