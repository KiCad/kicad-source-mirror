
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 KiCad Developers, see change_log.txt for contributors.

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

#include <wx/string.h>
#include <io_mgr.h>
#include <boost/property_tree/xml_parser.hpp>


class EAGLE_PLUGIN : public PLUGIN
{
public:

    //-----<PUBLIC PLUGIN API>-------------------------------------------------
    const wxString& PluginName();

    BOARD* Load( const wxString& aFileName, BOARD* aAppendToMe,  PROPERTIES* aProperties = NULL );

    void Save( const wxString& aFileName, BOARD* aBoard, PROPERTIES* aProperties = NULL );

    wxArrayString FootprintEnumerate( const wxString& aLibraryPath, PROPERTIES* aProperties = NULL);

    MODULE* FootprintLoad( const wxString& aLibraryPath, const wxString& aFootprintName, PROPERTIES* aProperties = NULL );

    void FootprintSave( const wxString& aLibraryPath, const MODULE* aFootprint, PROPERTIES* aProperties = NULL );

    void FootprintDelete( const wxString& aLibraryPath, const wxString& aFootprintName );

    void FootprintLibCreate( const wxString& aLibraryPath, PROPERTIES* aProperties = NULL );

    void FootprintLibDelete( const wxString& aLibraryPath, PROPERTIES* aProperties = NULL );

    bool IsFootprintLibWritable( const wxString& aLibraryPath );


#if 0

    const wxString& GetFileExtension() const = 0;
#endif

};


const wxString& EAGLE_PLUGIN::PluginName()
{
    static const wxString name = wxT( "Eagle" );
    return name;
}


BOARD* EAGLE_PLUGIN::Load( const wxString& aFileName, BOARD* aAppendToMe,  PROPERTIES* aProperties )
{
    return aAppendToMe;
}


void EAGLE_PLUGIN::Save( const wxString& aFileName, BOARD* aBoard, PROPERTIES* aProperties )
{
}


wxArrayString EAGLE_PLUGIN::FootprintEnumerate( const wxString& aLibraryPath, PROPERTIES* aProperties )
{
    return wxArrayString();
}


MODULE* EAGLE_PLUGIN::FootprintLoad( const wxString& aLibraryPath, const wxString& aFootprintName, PROPERTIES* aProperties )
{
    return NULL;
}


void EAGLE_PLUGIN::FootprintSave( const wxString& aLibraryPath, const MODULE* aFootprint, PROPERTIES* aProperties )
{
}


void EAGLE_PLUGIN::FootprintDelete( const wxString& aLibraryPath, const wxString& aFootprintName )
{
}


void EAGLE_PLUGIN::FootprintLibCreate( const wxString& aLibraryPath, PROPERTIES* aProperties )
{
}


void EAGLE_PLUGIN::FootprintLibDelete( const wxString& aLibraryPath, PROPERTIES* aProperties )
{
}


bool EAGLE_PLUGIN::IsFootprintLibWritable( const wxString& aLibraryPath )
{
    return true;
}



#if 0

    wxXmlDocument xml;

    xml.SetFileEncoding( wxT( "UTF-8" ) );

    if( !xml.Load( dlg.GetFilename() ) )
            return;

    XNODE *macrosNode = (XNODE*) xml.GetRoot()->GetChildren();

    while( macrosNode )
    {
        int number = -1;

        if( macrosNode->GetName() == wxT( "macros" ) )
        {
            number = wxAtoi( macrosNode->GetAttribute( wxT( "number" ), wxT( "-1" ) ) );

            if( number >= 0  && number < 10 )
            {
                m_Macros[number].m_Record.clear();

                XNODE *hotkeyNode = (XNODE*) macrosNode->GetChildren();

                while( hotkeyNode )
                {
                    if( hotkeyNode->GetName() == wxT( "hotkey" ) )
                    {
                        int x = wxAtoi( hotkeyNode->GetAttribute( wxT( "x" ), wxT( "0" ) ) );
                        int y = wxAtoi( hotkeyNode->GetAttribute( wxT( "y" ), wxT( "0" ) ) );
                        int hk = wxAtoi( hotkeyNode->GetAttribute( wxT( "hkcode" ), wxT( "0" ) ) );

                        MACROS_RECORD macros_record;
                        macros_record.m_HotkeyCode = hk;
                        macros_record.m_Position.x = x;
                        macros_record.m_Position.y = y;
                        m_Macros[number].m_Record.push_back( macros_record );
                    }

                    hotkeyNode = (XNODE*) hotkeyNode->GetNext();
                }
            }
        }

        macrosNode = (XNODE*) macrosNode->GetNext();
    }

#endif
