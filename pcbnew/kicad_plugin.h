/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) CERN.
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

#ifndef KICAD_PLUGIN_H_
#define KICAD_PLUGIN_H_

#include <io_mgr.h>
#include <string>

class BOARD;
class BOARD_ITEM;


/** Current s-expression file format version.  2 was the last legacy format version. */
#define SEXPR_BOARD_FILE_VERSION   3

/** Format output for the clipboard instead of a file. */
#define CTL_CLIPBOARD              (1 << 0)


/**
 * Class PCB_IO
 * is a PLUGIN derivation for saving and loading Pcbnew s-expression formatted files.
 *
 * @note This class is not thread safe, but it is re-entrant multiple times in sequence.
 */
class PCB_IO : public PLUGIN
{

public:
    const wxString& PluginName() const
    {
        static const wxString name = wxT( "KiCad" );
        return name;
    }

    const wxString& GetFileExtension() const
    {
        static const wxString extension = wxT( "kicad_pcb" );
        return extension;
    }

    void Save( const wxString& aFileName, BOARD* aBoard,
               PROPERTIES* aProperties = NULL );          // overload

    /**
     * Function Format
     * outputs \a aItem to \a aFormatter in s-expression format.
     *
     * @param aItem A pointer the an #BOARD_ITEM object to format.
     * @param aFormatter The #OUTPUTFORMATTER object to write to.
     * @param aNestLevel The indentation nest level.
     * @param aControlBits The control bit definition for object specific formatting.
     * @throw IO_ERROR on write error.
     */
    void Format( BOARD_ITEM* aItem, OUTPUTFORMATTER* aFormatter, int aNestLevel,
                 int aControlBits ) const
        throw( IO_ERROR );

protected:

    wxString        m_error;        ///< for throwing exceptions
    BOARD*          m_board;        ///< which BOARD, no ownership here
    PROPERTIES*     m_props;        ///< passed via Save() or Load(), no ownership, may be NULL.

    LINE_READER*    m_reader;       ///< no ownership here.
    wxString        m_filename;     ///< for saves only, name is in m_reader for loads

    int             m_loading_format_version;   ///< which BOARD_FORMAT_VERSION am I Load()ing?

private:
    void format( BOARD* aBoard, OUTPUTFORMATTER* aFormatter, int aNestLevel,
                 int aControlBits ) const
        throw( IO_ERROR );

    void format( DIMENSION* aDimension, OUTPUTFORMATTER* aFormatter, int aNestLevel,
                 int aControlBits ) const
        throw( IO_ERROR );

    void format( EDGE_MODULE* aModuleDrawing, OUTPUTFORMATTER* aFormatter, int aNestLevel,
                 int aControlBits ) const
        throw( IO_ERROR );

    void format( DRAWSEGMENT* aSegment, OUTPUTFORMATTER* aFormatter, int aNestLevel,
                 int aControlBits ) const
        throw( IO_ERROR );

    void format( PCB_TARGET* aTarget, OUTPUTFORMATTER* aFormatter, int aNestLevel,
                         int aControlBits ) const
        throw( IO_ERROR );

    void format( MODULE* aModule, OUTPUTFORMATTER* aFormatter, int aNestLevel,
                 int aControlBits ) const
        throw( IO_ERROR );

    void format( D_PAD* aPad, OUTPUTFORMATTER* aFormatter, int aNestLevel,
                 int aControlBits ) const
        throw( IO_ERROR );

    void format( TEXTE_PCB* aText, OUTPUTFORMATTER* aFormatter, int aNestLevel,
                 int aControlBits ) const
        throw( IO_ERROR );

    void format( TEXTE_MODULE* aText, OUTPUTFORMATTER* aFormatter, int aNestLevel,
                 int aControlBits ) const
        throw( IO_ERROR );

    void format( TRACK* aTrack, OUTPUTFORMATTER* aFormatter, int aNestLevel,
                 int aControlBits ) const
        throw( IO_ERROR );

    void format( ZONE_CONTAINER* aZone, OUTPUTFORMATTER* aFormatter, int aNestLevel,
                 int aControlBits ) const
        throw( IO_ERROR );
};

#endif  // KICAD_PLUGIN_H_
