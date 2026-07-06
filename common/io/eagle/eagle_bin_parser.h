/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Binary Eagle parsing logic and the eagle_script[] format table ported from
 * pcb-rnd src_plugins/io_eagle (eagle_bin.c) by Tibor 'Igor2' Palinkas and
 * Erich S. Heinzle.
 *
 *                            COPYRIGHT (pcb-rnd, eagle_bin.c / eagle_bin.h)
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
 *  Copyright (C) 2017 Erich S. Heinzle
 *
 * Copyright (C) 2026 KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef EAGLE_BIN_PARSER_H_
#define EAGLE_BIN_PARSER_H_

#include <cstdint>
#include <map>
#include <memory>
#include <vector>

#include <wx/string.h>

class wxXmlDocument;
class wxXmlNode;
class wxInputStream;

/**
 * Read-only parser for the pre-v6 binary Eagle .brd format.
 *
 * Eagle stored boards in a flat stream of fixed 24-byte records before the
 * XML era (versions 6+). The binary is decoded into a node tree structurally
 * identical to the Eagle XML DOM, then fed to the same XML tree-walker, so
 * KiCad's existing PCB_IO_EAGLE::loadAllSections() can consume the result
 * unchanged.
 *
 * There is no separate binary-to-board mapping. The binary is parsed into a
 * generic intermediate tree (EGB_NODE), rewritten in place by a series of
 * post-processing passes so its element names and attribute names match the
 * XML schema, and finally emitted as a wxXmlDocument.
 */
class EAGLE_BIN_PARSER
{
public:
    /**
     * Probe the first two bytes for the binary magic.
     *
     * @return true for v4/v5 (0x10 0x00) or v3 (0x10 0x80) binary boards.
     */
    static bool IsBinaryEagle( wxInputStream& aStream );

    EAGLE_BIN_PARSER();
    ~EAGLE_BIN_PARSER();

    /**
     * Parse a binary Eagle board into an XML DOM compatible with the XML walker.
     *
     * @param aBytes is the entire file content.
     * @return a document owning the synthesized DOM; throws IO_ERROR on failure.
     */
    std::unique_ptr<wxXmlDocument> Parse( const std::vector<uint8_t>& aBytes );

private:
    /// Lightweight mutable tree node for the intermediate Eagle binary tree.
    struct EGB_NODE
    {
        int                                    id = 0;
        wxString                               name;
        wxString                               content; ///< PCDATA emitted as the XML node's text body
        std::map<wxString, wxString>           props;
        std::vector<std::unique_ptr<EGB_NODE>> children;
        EGB_NODE*                              parent = nullptr;

        EGB_NODE* AddChild( int aId, const wxString& aName );

        /// Move an existing node in as a child, repointing its parent link.
        EGB_NODE* AdoptChild( std::unique_ptr<EGB_NODE> aChild );

        bool      HasProp( const wxString& aKey ) const { return props.count( aKey ) != 0; }
        wxString  Prop( const wxString& aKey ) const;
        long      PropLong( const wxString& aKey ) const;

        /// Format a property doubled in 64-bit so the half-to-full widening
        /// cannot overflow a 32-bit long.
        wxString  PropDoubled( const wxString& aKey ) const;
        EGB_NODE* FindChildById( int aId ) const;
        EGB_NODE* FindChildByName( const wxString& aName ) const;

        /// Apply aFn to this node and every descendant, pre-order.
        template <typename FN>
        void ForEach( FN&& aFn )
        {
            aFn( this );

            for( const auto& child : children )
                child->ForEach( aFn );
        }
    };

    /// DRC values pulled from the trailing 244-byte block (or sane defaults).
    struct DRC_CTX
    {
        long   mdWireWire = 12;
        long   msWidth = 10;
        double rvPadTop = 0.25;
        double rvPadInner = 0.25;
        double rvPadBottom = 0.25;
    };

    // Recursive descent over the 24-byte block stream.
    int readBlock( long& aNumBlocks, EGB_NODE* aParent );

    // Trailing optional sections (v4/v5 only).
    bool readNotes();
    bool readDrc( DRC_CTX& aDrc );

    const wxString& nextLongText();

    // Each 0x7F-marked string field holds a little-endian pointer into the free-text
    // blob; resolve them by offset so the result is independent of traversal order.
    void resolveLongPointers();

    void postProcess( EGB_NODE* aRoot, const DRC_CTX& aDrc );

    // Schematic assembly, split into ordered steps. resolveSchLibraries returns the
    // library list that resegmentSchSheets needs to resolve part references.
    void postProcessSchematic( EGB_NODE* aRoot );
    void postprocRequiredAttrs( EGB_NODE* aRoot );
    void postprocSchAttrs( EGB_NODE* aRoot );
    void renameSchSections( EGB_NODE* aSchematic );
    std::vector<EGB_NODE*> resolveSchLibraries( EGB_NODE* aSchematic );
    void resegmentSchSheets( EGB_NODE* aSchematic, const std::vector<EGB_NODE*>& aLibList );

    static std::vector<EGB_NODE*> childrenById( EGB_NODE* aParent, int aChildId );
    static wxString               nameByOrdinal( const std::vector<EGB_NODE*>& aList, long aIdx );
    void postprocLayers( EGB_NODE* aDrawing, EGB_NODE* aLayers );
    void postprocDrc( EGB_NODE* aDrcNode, const DRC_CTX& aDrc );
    void postprocLibs( EGB_NODE* aLibraries );
    void postprocElements( EGB_NODE* aElements );
    void postprocAttributes( EGB_NODE* aRoot );
    void postprocNames( EGB_NODE* aLibraries, EGB_NODE* aElements );
    void postprocSignals( EGB_NODE* aSignals );
    void postprocContactRefs( EGB_NODE* aSignals, EGB_NODE* aElements, EGB_NODE* aLibraries );
    void postprocWires( EGB_NODE* aRoot );
    void postprocArcs( EGB_NODE* aRoot );
    void postprocPolygons( EGB_NODE* aRoot );
    void postprocVias( EGB_NODE* aRoot );
    void postprocUnits( EGB_NODE* aRoot );
    void postprocCircles( EGB_NODE* aRoot );
    void postprocSmd( EGB_NODE* aRoot );
    void postprocDimensions( EGB_NODE* aRoot );
    void postprocFreeText( EGB_NODE* aRoot );
    void postprocLongText( EGB_NODE* aRoot );
    void postprocTextContent( EGB_NODE* aRoot );
    void postprocRotation( EGB_NODE* aRoot );
    void postprocPins( EGB_NODE* aRoot );
    bool isRotatable( int aId ) const;
    bool isLongTextHost( int aId ) const;

    void arcDecode( EGB_NODE* aElem, int aArcType, int aLineType );
    void fixLongText( EGB_NODE* aNode, const wxString& aField );

    // Convert the finished intermediate tree into wxXmlNode form.
    wxXmlNode* toXml( const EGB_NODE* aNode ) const;

    // Throw IO_ERROR unless [aOffs, aOffs + aLen) lies within m_buf.
    void requireBytes( size_t aOffs, size_t aLen ) const;

    // Little-endian field extractors over m_buf.
    uint32_t loadU32( size_t aOffs, unsigned aLen ) const;
    int32_t  loadS32( size_t aOffs, unsigned aLen ) const;
    bool     loadBmb( size_t aOffs, uint32_t aMask ) const;
    uint32_t loadUbf( size_t aOffs, uint32_t aField ) const;
    wxString loadStr( size_t aOffs, unsigned aLen ) const;
    double   loadDouble( size_t aOffs ) const;

    const std::vector<uint8_t>* m_buf = nullptr; ///< file contents, not owned
    size_t                      m_pos = 0;       ///< current read cursor
    int                         m_majorVer = 5;  ///< format major version from the drawing header

    std::unique_ptr<EGB_NODE> m_root;

    std::vector<wxString> m_freeText; ///< NUL-delimited notes strings
    size_t                m_freeTextCursor = 0;
    wxString              m_invalidText; ///< returned when out of strings

    /// Free-text strings keyed by their byte offset within the blob, for pointer
    /// (0x7F-reference) resolution.
    std::map<size_t, wxString> m_freeTextByOffset;

    /// A deferred string field and the absolute blob pointer it resolves to,
    /// recorded by readBlock() for resolveLongPointers().
    struct LONG_REF
    {
        EGB_NODE* node;
        wxString  field;
        uint32_t  ptr;
    };

    std::vector<LONG_REF> m_longRefs;
};

#endif // EAGLE_BIN_PARSER_H_
