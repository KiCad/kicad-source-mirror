/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Thomas Pointhuber <thomas.pointhuber@gmx.at>
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

#include "altium_pcb.h"
#include "altium_parser_binary.h"

#include <class_board.h>

#include <compoundfilereader.h>
#include <utf.h>


const CFB::COMPOUND_FILE_ENTRY* FindStream(const CFB::CompoundFileReader& reader, const char* streamName)
{
    const CFB::COMPOUND_FILE_ENTRY* ret = nullptr;
    reader.EnumFiles(reader.GetRootEntry(), -1,
                     [&](const CFB::COMPOUND_FILE_ENTRY* entry, const CFB::utf16string& u16dir, int level)->void
                     {
                         if (reader.IsStream(entry))
                         {
                             std::string name = UTF16ToUTF8(entry->name);
                             if (u16dir.length() > 0)
                             {
                                 std::string dir = UTF16ToUTF8(u16dir.c_str());
                                 if (strncmp(streamName, dir.c_str(), dir.length()) == 0 &&
                                     streamName[dir.length()] == '\\' &&
                                     strcmp(streamName + dir.length() + 1, name.c_str()) == 0)
                                 {
                                     ret = entry;
                                 }
                             }
                             else
                             {
                                 if (strcmp(streamName, name.c_str()) == 0)
                                 {
                                     ret = entry;
                                 }
                             }
                         }
                     });
    return ret;
}


ALTIUM_PCB::ALTIUM_PCB(BOARD *aBoard) {
    m_board = aBoard;
}

ALTIUM_PCB::~ALTIUM_PCB() {

}

void ALTIUM_PCB::Parse( const CFB::CompoundFileReader& aReader ) {
    // Parse file header
    const CFB::COMPOUND_FILE_ENTRY* fileHeader = FindStream(aReader, "FileHeader");
    wxASSERT( fileHeader != nullptr );
    if (fileHeader != nullptr)
    {
        ParseFileHeader(aReader, fileHeader);
    }

    // Parse pads
    const CFB::COMPOUND_FILE_ENTRY* pads6 = FindStream(aReader, "Pads6\\Data");
    wxASSERT( pads6 != nullptr );
    if (pads6 != nullptr)
    {
        ParsePads6Data(aReader, pads6);
    }
}

void ALTIUM_PCB::ParseFileHeader( const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry ) {
    ALTIUM_PARSER_BINARY reader(aReader, aEntry);

    reader.skip(4);
    std::cout << "HEADER: " << reader.read_string() << std::endl;  // tells me: PCB 5.0 Binary File
}

void ALTIUM_PCB::ParsePads6Data( const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry ) {
    ALTIUM_PARSER_BINARY reader(aReader, aEntry);

    while( reader.bytes_remaining() > 5 + 147 ) {
        u_int8_t recordtype = reader.read<u_int8_t>();
        wxASSERT( recordtype == 0x02 );

        u_int32_t len = reader.read<u_int32_t>();
        std::string name = reader.read_string();
        wxASSERT( len-1 == name.size() );
        std::cout << "Pad: '" << name << "'" << std::endl;

        reader.skip(30);

        u_int16_t component = reader.read<u_int16_t>();
        std::cout << "  component: " << component << std::endl;

        reader.skip(4);

        wxPoint position = reader.read_point();
        std::cout << "  position: " << position << std::endl;
        wxSize topsize = reader.read_size();
        std::cout << "  topsize: " << topsize << std::endl;
        wxSize midsize = reader.read_size();
        std::cout << "  midsize: " << midsize << std::endl;
        wxSize botsize = reader.read_size();
        std::cout << "  botsize: " << botsize << std::endl;

        MODULE* module = new MODULE(m_board);
        m_board->Add(module);

        module->SetPosition(position);

        D_PAD* pad = new D_PAD(module);
        module->Add(pad);

        pad->SetName(name);
        pad->SetSize(topsize);
        pad->SetLayer(PCB_LAYER_ID::F_Cu);  // TODO?
        pad->SetShape(PAD_SHAPE_T::PAD_SHAPE_RECT);
        pad->SetAttribute(PAD_ATTR_T::PAD_ATTRIB_SMD);

        reader.skip(147-60-8);

        // TODO: in special cases we need to skip more!
    }
}