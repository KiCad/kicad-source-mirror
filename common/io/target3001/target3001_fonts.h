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

#ifndef TARGET3001_FONTS_H
#define TARGET3001_FONTS_H

#include <map>
#include <tuple>

#include <math/vector2d.h>
#include <wx/string.h>

class EDA_TEXT;

namespace KIFONT
{
class OUTLINE_FONT;
}

namespace TARGET3001
{

struct ELEMENT;

/**
 * Maps TARGET 3001! text onto KiCad fonts.
 *
 * The built-in TARGET font is fixed pitch with a cap height equal to the text height, so its width is chosen
 * to give KiCad's stroke font the same string length.  DIN-ISO-ANSI scales its glyphs uniformly by the larger
 * of height and width with a cap height of 0.9 of that size.  Its glyphs are proportional with a fixed gap, so
 * the width is the least squares fit of KiCad's glyph centres to the DIN-ISO-ANSI ones.
 *
 * TrueType fonts scale a fixed master rendering linearly: the em square is 300/222 of the text height
 * (300/241 for Calibri) and the width is the font's average character width.  The average is the
 * classic weighted lowercase average, so it is measured from glyph advances and stays correct when a
 * metric-compatible substitute font is loaded.
 */
class FONTS
{
public:
    /**
     * Give @a aText the font, style, size and stroke width of text @a aElem.
     *
     * The text content must already be set, because fixed-pitch text is sized by its length.
     *
     * @param aUnit is the number of file nanometres per unit of @a aText.
     */
    void Apply( EDA_TEXT* aText, const ELEMENT& aElem, int aUnit );

    /**
     * Offset in text units that moves @a aText so the baseline-left point of its first glyph lies on its
     * position.
     *
     * KiCad offsets text from its position by amounts that depend on the font, while TARGET 3001! anchors
     * the baseline at the pen.  The built-in fonts draw the stem of H at the pen, so stroke text is also
     * aligned horizontally by it.  Only the baseline is aligned for right-justified text.
     */
    static VECTOR2I BaselineShift( const EDA_TEXT& aText );

private:
    bool applyTrueType( EDA_TEXT* aText, const ELEMENT& aElem, int aUnit );

    std::map<std::tuple<wxString, bool, bool>, KIFONT::OUTLINE_FONT*> m_fonts;
    std::map<const KIFONT::OUTLINE_FONT*, double> m_averageWidth;    ///< per unit of text size
};

} // namespace TARGET3001

#endif // TARGET3001_FONTS_H
