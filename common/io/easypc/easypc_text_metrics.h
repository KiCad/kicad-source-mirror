/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file easypc_text_metrics.h
 * @brief Easy-PC's text measurement, so text lands where Easy-PC draws it.
 *
 * Values are design units, Y up.  The first line's baseline cell starts at the anchor and each later line is one
 * interline pitch lower; alignment shifts each line by its own width; mirroring reflects about the anchor's X before
 * rotating about the anchor. Fixed-pitch stroke characters are one text height wide, which is exact. Proportional
 * stroke widths are absent from the file, so KiCad's stroke font estimates them. TrueType text cannot
 * be measured.
 */

#ifndef EASYPC_TEXT_METRICS_H
#define EASYPC_TEXT_METRICS_H

#include <cstdint>
#include <string>

#include <math/box2.h>
#include <math/vector2d.h>
#include <wx/string.h>


namespace EASYPC
{

struct SOURCE_DESIGN;
struct SOURCE_TEXT_STYLE;


/// The source text position's alignment value
enum TEXT_H_ALIGN : int32_t
{
    TEXT_H_LEFT = 0,
    TEXT_H_RIGHT = 1,
    TEXT_H_CENTRE = 2
};


/// Source text positions always pass BASELINE
enum TEXT_V_ALIGN : int32_t
{
    TEXT_V_BASELINE = 0,
    TEXT_V_TOP = 1,
    TEXT_V_MIDDLE = 2
};


class TEXT_METRICS
{
public:
    explicit TEXT_METRICS( const SOURCE_TEXT_STYLE& aStyle );

    /// False for a TrueType style; the measuring calls throw IO_ERROR for one
    bool CanMeasure() const { return !m_typeFont; }

    int32_t Height() const { return m_height; }

    /// Width of bytes [aBegin, aEnd); a run that measures 0 is one height wide
    int32_t RunWidth( const std::string& aText, size_t aBegin, size_t aEnd ) const;

    /**
     * Every line's cell, inflated by half the pen width, mirrored about the anchor and bounded after rotation.  An
     * empty string gives an empty box.
     */
    BOX2I TextBox( const wxString& aText, const VECTOR2I& aAnchor, int32_t aAngle, bool aMirrored, int aHAlign,
                   int aVAlign = TEXT_V_BASELINE ) const;

    /**
     * When aKeepUpright and UprightRotation turns the text, the anchor moves to the
     * far end of the first line one height up, then is mirrored and rotated with it; otherwise it is unchanged.
     */
    VECTOR2I UprightAnchor( const wxString& aText, const VECTOR2I& aAnchor, int32_t aAngle, bool aMirrored, int aHAlign,
                            bool aKeepUpright ) const;

    /// The CP1252 bytes of aText; characters CP1252 lacks become 0
    static std::string Encode( const wxString& aText );

private:
    void    checkMeasurable() const;
    int32_t linePitch() const;
    int32_t charWidth( char aChar ) const;

    int32_t m_height = 0;
    int32_t m_penWidth = 0;
    int32_t m_interlinePercent = 120;
    int32_t m_charWidthPercent = 100;
    bool    m_proportional = false;
    bool    m_typeFont = false;
};


/**
 * Whether aDesign's text is drawn upright: the design's flag, which old schematics imply, or a schematic from
 * DesignSpark Creator, Pro or ProtoPCB, which are always upright.
 */
bool TextStaysUpright( const SOURCE_DESIGN& aDesign, bool aSchematic );


/// With text kept upright, an angle above 135 and up to 305 degrees turns by 180
int32_t UprightRotation( int32_t aAngle, bool aKeepUpright );


/**
 * A displayed string in KiCad's markup.  A doubled underscore toggles an overbar and a line break ends
 * it, so "__X__Y" becomes "~{X}Y"; a literal "~{" is escaped.
 */
wxString ToKiCadMarkup( const wxString& aText );

} // namespace EASYPC

#endif // EASYPC_TEXT_METRICS_H
