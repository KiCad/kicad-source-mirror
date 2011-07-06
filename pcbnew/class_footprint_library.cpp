/**
 * @file class_footprint_library.cpp
 * Helper class to read/write footprint libraries.
 */

#include "fctsys.h"
#include "kicad_string.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "richio.h"
#include "filter_reader.h"

#include "class_footprint_library.h"

/*
 * Module library header format:
 * LIBRARY HEADER-datetime
 * $INDEX
 * List of modules names (1 name per line)
 * $EndIndex
 * List of descriptions of Modules
 * $EndLIBRARY
 */

FOOTPRINT_LIBRARY::FOOTPRINT_LIBRARY( FILE * aFile, FILTER_READER * aReader )
{
    wxASSERT( m_reader || m_file );

    m_file = aFile;
    m_reader = aReader;
    m_LineNum = 0;
}

/* function IsLibrary
 * Read the library file Header
 * return > 0 if this file is a footprint lib
 * (currentlu return 1 but could be a value > 1 for future file formats
 */
int FOOTPRINT_LIBRARY::IsLibrary( )
{
    char *line;
    char buffer[1024];
    if( m_reader )
    {
        m_reader->ReadLine();
        line = m_reader->Line();
    }
    else
    {
        line = buffer;
        GetLine( m_file, line, &m_LineNum );
    }

    StrPurge( line );
    if( strnicmp( line, ENTETE_LIBRAIRIE, L_ENTETE_LIB ) == 0 )
        return 1;

    return 0;
}


/*
 * function RebuildIndex
 * Read the full library file and build the list od footprints found
 * and do not use the $INDEX ... $EndINDEX section
 */
bool FOOTPRINT_LIBRARY::RebuildIndex()
{
    m_List.Clear();

    char  name[1024];

    if( m_reader )
    {
        while( m_reader->ReadLine() )
        {
            char * line = m_reader->Line();
            StrPurge( line );
            if( strnicmp( line, "$MODULE", 7 ) == 0 )
            {
                sscanf( line + 7, " %s", name );
                m_List.Add( FROM_UTF8( name ) );
            }
        }
    }
    else
    {
        char line[1024];
        while( GetLine( m_file, line, &m_LineNum ) )
        {
            if( strnicmp( line, "$MODULE", 7 ) == 0 )
            {
                sscanf( line + 7, " %s", name );
                m_List.Add( FROM_UTF8( name ) );
            }
        }
    }

    return true;
}

/* function ReadSectionIndex
 * Read the $INDEX ... $EndINDEX section
 * list of footprints is stored in m_List
 */
bool FOOTPRINT_LIBRARY::ReadSectionIndex()
{
    // Some broken INDEX sections have more than one section
    // So we must read the next line after $EndINDEX tag,
    // to see if this is not a new $INDEX tag.
    bool exit = false;
    if( m_reader )
    {
        while( m_reader->ReadLine() )
        {
            char * line = m_reader->Line();
            StrPurge( line );
            if( strnicmp( line, "$INDEX", 6 ) == 0 )
            {
                exit = false;
                while( m_reader->ReadLine() )
                {
                    StrPurge( line );
                    m_List.Add( FROM_UTF8( line ) );
                    if( strnicmp( line, "$EndINDEX", 9 ) == 0 )
                    {
                        exit = true;
                        break;
                    }
                }
            }
            else if( exit )
                break;
        }
    }
    else
    {
        char line[1024];
        while( GetLine( m_file, line, &m_LineNum ) )
        {
            if( strnicmp( line, "$INDEX", 6 ) == 0 )
            {
                exit = false;
                while( GetLine( m_file, line, &m_LineNum ) )
                {
                    StrPurge( line );
                    m_List.Add( FROM_UTF8( line ) );
                    if( strnicmp( line, "$EndINDEX", 9 ) == 0 )
                    {
                        exit = true;
                        break;
                    }
                }
            }
            else if( exit )
                break;
        }
    }

    return true;
}

/* Function WriteHeader
 * Write the library header
 */
bool FOOTPRINT_LIBRARY::WriteHeader()
{
    char line[256];
    fprintf( m_file, "%s  %s\n", ENTETE_LIBRAIRIE, DateAndTime( line ) );
    fprintf( m_file, "# encoding utf-8\n");
    return true;
}

/* Function WriteSectionIndex
 * Write the $INDEX ... $EndINDEX section.
 * This section is filled by names in m_List
 */
bool FOOTPRINT_LIBRARY::WriteSectionIndex()
{
    fputs( "$INDEX\n", m_file );
    for( unsigned ii = 0; ii < m_List.GetCount(); ii++ )
    {
        fprintf( m_file, "%s\n", TO_UTF8( m_List[ii] ) );
    }
    fputs( "$EndINDEX\n", m_file );
    return true;
}

/* Function WriteEndOfFile
 * Write the last line section.
 */
bool FOOTPRINT_LIBRARY::WriteEndOfFile()
{
    fputs( "$EndLIBRARY\n", m_file );
    return true;
}

/*
 * Function FindInList
 * Search for aName int m_List and return true if found
 */
bool FOOTPRINT_LIBRARY::FindInList( const wxString & aName )
{
    for( unsigned ii = 0; ii < m_List.GetCount(); ii++ )
    {
        if( m_List[ii].CmpNoCase( aName ) == 0 )
            return true;
    }

    return false;
}

/**
 * Function RemoveFromList
 * Search for aName int m_List and remove it
 * @return true if found and removed
 */
bool FOOTPRINT_LIBRARY::RemoveFromList( const wxString & aName )
{
    for( unsigned ii = 0; ii < m_List.GetCount(); ii++ )
    {
        if( m_List[ii].CmpNoCase( aName ) == 0 )
        {
            m_List.RemoveAt(ii);
            return true;
        }
    }

    return false;
}

/**
 * Function SortList
 * Sort m_List in alphabetic order
 */
void FOOTPRINT_LIBRARY::SortList()
{
    m_List.Sort();
}
