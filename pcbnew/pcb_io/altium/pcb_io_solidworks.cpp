/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/string.h>

#include <font/fontconfig.h>
#include <pcb_io_solidworks.h>
#include <pcb_io_altium_designer.h>
#include <altium_pcb.h>
#include <altium_pcb_compound_file.h>
#include <io/altium/altium_binary_parser.h>
#include <pcb_io/pcb_io.h>
#include <reporter.h>

#include <board.h>

#include <compoundfilereader.h>
#include <utf.h>

PCB_IO_SOLIDWORKS::PCB_IO_SOLIDWORKS() :
        PCB_IO( wxS( "Solidworks PCB" ) )
{
    m_reporter = &WXLOG_REPORTER::GetInstance();
    RegisterCallback( PCB_IO_ALTIUM_DESIGNER::DefaultLayerMappingCallback );
}


PCB_IO_SOLIDWORKS::~PCB_IO_SOLIDWORKS()
{
}


bool PCB_IO_SOLIDWORKS::CanReadBoard( const wxString& aFileName ) const
{
    if( !PCB_IO::CanReadBoard( aFileName ) )
        return false;

    return PCB_IO_ALTIUM_DESIGNER::checkFileHeader( aFileName );
}


BOARD* PCB_IO_SOLIDWORKS::LoadBoard( const wxString& aFileName, BOARD* aAppendToMe,
                                         const std::map<std::string, UTF8>* aProperties, PROJECT* aProject )
{
    m_props = aProperties;

    m_board = aAppendToMe ? aAppendToMe : new BOARD();

    // Collect the font substitution warnings (RAII - automatically reset on scope exit)
    FONTCONFIG_REPORTER_SCOPE fontconfigScope( &LOAD_INFO_REPORTER::GetInstance() );

    // Give the filename to the board if it's new
    if( !aAppendToMe )
        m_board->SetFileName( aFileName );

    // clang-format off
    const std::map<ALTIUM_PCB_DIR, std::string> mapping = {
            { ALTIUM_PCB_DIR::FILE_HEADER, "FileHeader" },
            { ALTIUM_PCB_DIR::ARCS6, "D2864697BB2D411B857EBD69D74447" },
            { ALTIUM_PCB_DIR::BOARD6, "21CE7E3D9BFF41679BACA1184CAF54" },
            { ALTIUM_PCB_DIR::BOARDREGIONS, "67075A4119214CE4AB174F9B1A9A41" },
            { ALTIUM_PCB_DIR::CLASSES6, "1122D4F14A924F9CA5C2060AF370E0" },
            { ALTIUM_PCB_DIR::COMPONENTS6, "208CAE8E44BD43D5B3CCA426D9331B" },
            { ALTIUM_PCB_DIR::COMPONENTBODIES6, "6DDF94E6CB364893BED31C189F9AF3" },
            { ALTIUM_PCB_DIR::DIMENSIONS6, "6148AE8C77B042798B46830E96BB24" },
            { ALTIUM_PCB_DIR::FILLS6, "5944DE0E258C41E2B0B382AC964048" },
            { ALTIUM_PCB_DIR::MODELS, "874F98A7E25A48EDAD394EB891E503" },
            { ALTIUM_PCB_DIR::NETS6, "0201837ACD434D55B34BBC68B75BAB" },
            { ALTIUM_PCB_DIR::PADS6, "E4D0C33E25824886ABC7FEEAE7B521" },
            { ALTIUM_PCB_DIR::POLYGONS6, "7ABD4252549749DD8DB16804819AC3" },
            { ALTIUM_PCB_DIR::REGIONS6, "6B3892541AB94CD999291D590B5C86" }, // probably wrong; in as a placeholder
            { ALTIUM_PCB_DIR::RULES6, "7009830ADF65423FA6CCB73A77E710" },
            { ALTIUM_PCB_DIR::SHAPEBASEDREGIONS6, "91241C66300E4490965070BA56F6F7" },
            { ALTIUM_PCB_DIR::TEXTS6, "4AF3D139533041489C2A57BBF9890D" },
            { ALTIUM_PCB_DIR::TRACKS6, "5D0C6E18E16A4BBFAA256C24B79EAE" },
            { ALTIUM_PCB_DIR::VIAS6, "2AF5387F097242D3A1095B6FAC3397" },
            { ALTIUM_PCB_DIR::WIDESTRINGS6, "9B378679AF85466C8673A41EE46393" }

            /*
             * Understood but not needed:
             * 04A8F96E0E4C478C813AE57CACCD0F - Legacy text storage
             * 01F1BD1AA06E4D6A9D1ABF0BBFF4A4 - Fwd/Back compatibility messages
             *
             * Not yet used by KiCad:
             * 7C01505E39124E67BCCAB1883B8FB7 - Design Rule Checker Options6
             * 63B31A3709B54882BFA96424906BE8 - EmbeddedFonts6
             * 8B83C7E94C1D419B9B2D5505479820 - Pin Swap Options6
             * F78C10230A794F5C93ACB50AC693B2 - Advanced Placer Options6
             *
             * No data yet on:
             * 1C0DB1ED572645BEB65D029D20406C
             * 2AA5C1C72BF14315A47DD931B84A79
             * 6468C28D32CC4867AB374091CC8431
             * 6B3892541AB94CD999291D590B5C86
             * 8675F4105E444E6D9B2BEE6273769D
             * B983FCC2B6DE46E0B94006B6393235
             * D6551D22B6DB44659C3F32F7E1949D
             *
             * Not yet identified:
             * D06DD2E4A51C4A3EA96D9ED8C8F3F3
             * F9C465994DE840579ED19E820C19C2
             *
             * Region-like objects that don't map cleanly (maybe Solidworks sketches?)
             * 84958494F3F54075975C4E199DB8EB
             * 2E731D36D1F049428744F0661F3E44
             */
    };
    // clang-format on

    ALTIUM_PCB_COMPOUND_FILE altiumPcbFile( aFileName );

    try
    {
        // Parse File
        ALTIUM_PCB pcb( m_board, m_progressReporter, m_layer_mapping_handler, m_reporter );
        pcb.Parse( altiumPcbFile, mapping );
    }
    catch( CFB::CFBException& exception )
    {
        THROW_IO_ERROR( exception.what() );
    }

    return m_board;
}
