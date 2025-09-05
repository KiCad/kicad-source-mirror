/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one at
 * http://www.gnu.org/licenses/
 */


#pragma once

#include <optional>

#include <geometry/circle.h>
#include <geometry/seg.h>
#include <math/box2.h>
#include <sch_pin.h>


class SCHEMATIC_SETTINGS;

/**
 * A pin layout helper is a class that manages the layout of the parts of
 * a pin on a schematic symbol:
 *
 * including, extents of:
 * - the pin itself
 * - the pin number, number, type
 * - decorations
 * - alternate mode icons
 *
 * This is useful, because this information is used in multiple places,
 * and regenerating it in multiple places is error-prone. It can also
 * be cached if it's encapsulated in one place.
 */
class PIN_LAYOUT_CACHE
{
public:
    PIN_LAYOUT_CACHE( const SCH_PIN& aPin );

    enum DIRTY_FLAGS
    {
        NAME = 1,
        NUMBER = 2,
        ELEC_TYPE = 4,

        ALL = NAME | NUMBER | ELEC_TYPE,
    };

    /**
     * Recompute all the layout information.
     */
    void MarkDirty( int aFlags );

    void SetRenderParameters( int aNameThickness, int aNumberThickness, bool aShowElectricalType,
                              bool aShowAltIcons );

    /**
     * Get the bounding box of the pin itself.
     */
    BOX2I GetPinBoundingBox( bool aIncludeLabelsOnInvisiblePins, bool aIncludeNameAndNumber,
                             bool aIncludeElectricalType );

    /**
     * Get the bounding box of the pin name, if there is one.
     */
    OPT_BOX2I GetPinNameBBox();

    /**
     * Get the bounding box of the pin number, if there is one.
     */
    OPT_BOX2I GetPinNumberBBox();

    /**
     * Get the box of the alt mode icon, if there is one.
     */
    OPT_BOX2I GetAltIconBBox();

    /**
     * Gets the dangling indicator geometry for this pin, if the
     * pin were to be dangling.
     */
    CIRCLE GetDanglingIndicator() const;

    struct TEXT_INFO
    {
        wxString          m_Text;
        int               m_TextSize;
        int               m_Thickness;
        VECTOR2I          m_TextPosition;
        GR_TEXT_H_ALIGN_T m_HAlign;
        GR_TEXT_V_ALIGN_T m_VAlign;
        EDA_ANGLE         m_Angle;
    };

    /**
     * Get the text info for the pin name.
     *
     * If the pin name is not visible, this will return an empty optional.
     */
    std::optional<TEXT_INFO> GetPinNameInfo( int aShadowWidth );
    std::optional<TEXT_INFO> GetPinNumberInfo( int aShadowWidth );
    std::optional<TEXT_INFO> GetPinElectricalTypeInfo( int aShadowWidth );

private:

    bool isDirty( int aMask ) const
    {
        return m_dirtyFlags & aMask;
    }

    void setClean( int aMask )
    {
        m_dirtyFlags &= ~aMask;
    }

    /**
     * Cached extent of a text item.
     */
    struct TEXT_EXTENTS_CACHE
    {
        KIFONT::FONT* m_Font = nullptr;
        int           m_FontSize = 0;
        VECTOR2I      m_Extents;
    };

    static void recomputeExtentsCache( bool aDefinitelyDirty, KIFONT::FONT* aFont, int aSize,
                                       const wxString& aText, const KIFONT::METRICS& aFontMetrics,
                                       TEXT_EXTENTS_CACHE& aCache );

    /**
     * Recompute all the caches that have become dirty.
     */
    void recomputeCaches();

    /**
     * Transform a box (in-place) to the pin's orientation.
     */
    void transformBoxForPin( BOX2I& aBox ) const;

    /**
     * Transform text info to suit a pin's
     *
     * @param the 'nominal' text info for a PIN_RIGHT pin, which will be adjusted
     */
    void transformTextForPin( TEXT_INFO& aTextInfo ) const;

    /**
     * Get the current pin text offset
     */
    int getPinTextOffset() const;

    /**
     * Get the untransformd text box in the default orientation
     *
     * This will have to be offset and rotated.
     */
    OPT_BOX2I getUntransformedPinNameBox() const;
    OPT_BOX2I getUntransformedPinNumberBox() const;
    OPT_BOX2I getUntransformedPinTypeBox() const;
    OPT_BOX2I getUntransformedAltIconBox() const;

    /// Pin type decoration if any
    OPT_BOX2I getUntransformedDecorationBox() const;

    /// The pin in question
    const SCH_PIN& m_pin;

    // The schematic settings if there are any
    const SCHEMATIC_SETTINGS* m_schSettings;

    int m_dirtyFlags;

    // Cached render parameters
    float m_shadowOffsetAdjust = 1.0f;
    int   m_nameThickness = 0;
    int   m_numberThickness = 0;
    bool  m_showElectricalType = false;
    bool  m_showAltIcons = false;

    // Various cache members
    TEXT_EXTENTS_CACHE m_numExtentsCache;
    TEXT_EXTENTS_CACHE m_nameExtentsCache;
    TEXT_EXTENTS_CACHE m_typeExtentsCache;
};
