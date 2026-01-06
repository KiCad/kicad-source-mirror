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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef SYMBOL_H
#define SYMBOL_H

#include <lib_id.h>
#include <sch_item.h>
#include <sch_field.h>
#include <sch_pin.h>

// @todo Move this to transform alone with all of the transform manipulation code.
/// enum used in RotationMiroir()
enum SYMBOL_ORIENTATION_T
{
    SYM_NORMAL,                     // Normal orientation, no rotation or mirror
    SYM_ROTATE_CLOCKWISE,           // Rotate -90
    SYM_ROTATE_COUNTERCLOCKWISE,    // Rotate +90
    SYM_ORIENT_0,                   // No rotation and no mirror id SYM_NORMAL
    SYM_ORIENT_90,                  // Rotate 90, no mirror
    SYM_ORIENT_180,                 // Rotate 180, no mirror
    SYM_ORIENT_270,                 // Rotate -90, no mirror
    SYM_MIRROR_X = 0x100,           // Mirror around X axis
    SYM_MIRROR_Y = 0x200            // Mirror around Y axis
};


// Cover for SYMBOL_ORIENTATION_T for property manager (in order to expose only a subset of
// SYMBOL_ORIENTATION_T's values).
enum SYMBOL_ORIENTATION_PROP
{
    SYMBOL_ANGLE_0   = SYMBOL_ORIENTATION_T::SYM_ORIENT_0,
    SYMBOL_ANGLE_90  = SYMBOL_ORIENTATION_T::SYM_ORIENT_90,
    SYMBOL_ANGLE_180 = SYMBOL_ORIENTATION_T::SYM_ORIENT_180,
    SYMBOL_ANGLE_270 = SYMBOL_ORIENTATION_T::SYM_ORIENT_270
};


/**
 * A base class for #LIB_SYMBOL and #SCH_SYMBOL.
 */
class SYMBOL : public SCH_ITEM
{
public:
    SYMBOL( KICAD_T idType ) :
            SYMBOL( nullptr, idType )
    { }

    SYMBOL( EDA_ITEM* aParent, KICAD_T idType ) :
            SCH_ITEM( aParent, idType ),
            m_pinNameOffset( 0 ),
            m_showPinNames( true ),
            m_showPinNumbers( true ),
            m_excludedFromSim( false ),
            m_excludedFromBOM( false ),
            m_excludedFromBoard( false ),
            m_excludedFromPosFiles( false ),
            m_DNP( false )
    { }

    SYMBOL( const SYMBOL& base ) :
            SCH_ITEM( base ),
            m_pinNameOffset( base.m_pinNameOffset ),
            m_showPinNames( base.m_showPinNames ),
            m_showPinNumbers( base.m_showPinNumbers ),
            m_excludedFromSim( base.m_excludedFromSim ),
            m_excludedFromBOM( base.m_excludedFromBOM ),
            m_excludedFromBoard( base.m_excludedFromBoard ),
            m_excludedFromPosFiles( base.m_excludedFromPosFiles ),
            m_DNP( base.m_DNP )
    { }

    SYMBOL& operator=( const SYMBOL& aItem )
    {
        SCH_ITEM::operator=( aItem );

        m_pinNameOffset       = aItem.m_pinNameOffset;
        m_showPinNames        = aItem.m_showPinNames;
        m_showPinNumbers      = aItem.m_showPinNumbers;

        m_excludedFromSim     = aItem.m_excludedFromSim;
        m_excludedFromBOM     = aItem.m_excludedFromBOM;
        m_excludedFromBoard   = aItem.m_excludedFromBoard;
        m_excludedFromPosFiles = aItem.m_excludedFromPosFiles;
        m_DNP                 = aItem.m_DNP;

        return *this;
    };

    ~SYMBOL() override = default;

    virtual const LIB_ID& GetLibId() const = 0;
    virtual wxString GetDescription() const = 0;
    virtual wxString GetShownDescription( int aDepth = 0 ) const = 0;
    virtual wxString GetKeyWords() const = 0;
    virtual wxString GetShownKeyWords( int aDepth = 0 ) const = 0;

    virtual bool IsGlobalPower() const = 0;
    virtual bool IsLocalPower() const = 0;
    virtual bool IsPower() const = 0;
    virtual bool IsNormal() const = 0;

    /**
     * @return true if the symbol has multiple units per symbol.
     */
    virtual bool IsMultiUnit() const = 0;

    /**
     * @return the number of units defined for the symbol.
     */
    virtual int GetUnitCount() const = 0;

    /**
     * @return true if the symbol has multiple body styles available.
     */
    virtual bool IsMultiBodyStyle() const = 0;

    /**
     * @return the number of body styles defined for the symbol.
     */
    virtual int GetBodyStyleCount() const = 0;

    virtual bool HasDeMorganBodyStyles() const = 0;

    virtual const wxString GetRef( const SCH_SHEET_PATH* aSheet,
                                   bool aIncludeUnit = false ) const = 0;

    virtual const wxString GetValue( bool aResolve, const SCH_SHEET_PATH* aPath,
                                     bool aAllowExtraText, const wxString& aVaraintName = wxEmptyString ) const = 0;

    virtual void GetFields( std::vector<SCH_FIELD*>& aVector, bool aVisibleOnly ) const = 0;

    virtual std::vector<SCH_PIN*> GetPins() const = 0;

    /**
     * Set the offset in mils of the pin name text from the pin symbol.
     *
     * Set the offset to 0 to draw the pin name above the pin symbol.
     *
     * @param aOffset - The offset in mils.
     */
    void SetPinNameOffset( int aOffset ) { m_pinNameOffset = aOffset; }
    int GetPinNameOffset() const { return m_pinNameOffset; }

    /**
     * Set or clear the pin name visibility flag.
     */
    virtual void SetShowPinNames( bool aShow ) { m_showPinNames = aShow; }
    virtual bool GetShowPinNames() const { return m_showPinNames; }

    /**
     * Set or clear the pin number visibility flag.
     */
    virtual void SetShowPinNumbers( bool aShow ) { m_showPinNumbers = aShow; }
    virtual bool GetShowPinNumbers() const { return m_showPinNumbers; }

    /**
     * Set or clear the exclude from simulation flag.
     */
    virtual void SetExcludedFromSim( bool aExcludeFromSim, const SCH_SHEET_PATH* aInstance = nullptr,
                                     const wxString& aVariantName = wxEmptyString ) override
    {
        m_excludedFromSim = aExcludeFromSim;
    }

    virtual bool GetExcludedFromSim( const SCH_SHEET_PATH* aInstance = nullptr,
                                     const wxString& aVariantName = wxEmptyString ) const override
    {
        return m_excludedFromSim;
    }

    /**
     * Set or clear the exclude from schematic bill of materials flag.
     */
    virtual void SetExcludedFromBOM( bool aExcludeFromBOM, const SCH_SHEET_PATH* aInstance = nullptr,
                                     const wxString& aVariantName = wxEmptyString ) override
    {
        m_excludedFromBOM = aExcludeFromBOM;
    }

    virtual bool GetExcludedFromBOM( const SCH_SHEET_PATH* aInstance = nullptr,
                                     const wxString& aVariantName = wxEmptyString ) const override
    {
        return m_excludedFromBOM;
    }

    /**
     * Set or clear exclude from board netlist flag.
     */
    void SetExcludedFromBoard( bool aExclude, const SCH_SHEET_PATH* aInstance = nullptr,
                               const wxString& aVariantName = wxEmptyString ) override
    {
        m_excludedFromBoard = aExclude;
    }

    bool GetExcludedFromBoard( const SCH_SHEET_PATH* aInstance = nullptr,
                               const wxString& aVariantName = wxEmptyString ) const override
    {
        return m_excludedFromBoard;
    }

    /**
     * Set or clear exclude from position files flag.
     */
    void SetExcludedFromPosFiles( bool aExclude, const SCH_SHEET_PATH* aInstance = nullptr,
                                  const wxString& aVariantName = wxEmptyString ) override
    {
        m_excludedFromPosFiles = aExclude;
    }

    bool GetExcludedFromPosFiles( const SCH_SHEET_PATH* aInstance = nullptr,
                                  const wxString& aVariantName = wxEmptyString ) const override
    {
        return m_excludedFromPosFiles;
    }

    /**
     * Set or clear the 'Do Not Populate' flag.
     */
    virtual bool GetDNP( const SCH_SHEET_PATH* aInstance = nullptr,
                         const wxString& aVariantName = wxEmptyString ) const override { return m_DNP; }
    virtual void SetDNP( bool aDNP, const SCH_SHEET_PATH* aInstance = nullptr,
                         const wxString& aVariantName = wxEmptyString ) override { m_DNP = aDNP; }

    virtual int GetOrientation() const { return SYM_NORMAL; }

    const TRANSFORM& GetTransform() const { return m_transform; }
    TRANSFORM& GetTransform() { return m_transform; }
    void SetTransform( const TRANSFORM& aTransform ) { m_transform = aTransform; }

    void SetPreviewUnit( int aUnit ) { m_previewUnit = aUnit; }
    void SetPreviewBodyStyle( int aBodyStyle ) { m_previewBodyStyle = aBodyStyle; }

    /**
     * Return a bounding box for the symbol body but not the pins or fields.
     */
    virtual BOX2I GetBodyBoundingBox() const = 0;

    /**
     * Return a bounding box for the symbol body and pins but not the fields.
     */
    virtual BOX2I GetBodyAndPinsBoundingBox() const = 0;

    std::vector<int> ViewGetLayers() const override;

protected:
    TRANSFORM     m_transform;             ///< The rotation/mirror transformation.

    int           m_pinNameOffset;         ///< The offset in mils to draw the pin name.  Set to
                                           ///<   0 to draw the pin name above the pin.
    bool          m_showPinNames;
    bool          m_showPinNumbers;

    bool          m_excludedFromSim;
    bool          m_excludedFromBOM;
    bool          m_excludedFromBoard;
    bool          m_excludedFromPosFiles;
    bool          m_DNP;                   ///< True if symbol is set to 'Do Not Populate'.

    int           m_previewUnit = 1;
    int           m_previewBodyStyle = 1;
};

#endif  //  SYMBOL_H
