/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see change_log.txt for contributors.
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

/**
 * A base class for LIB_SYMBOL and SCH_SYMBOL.
 */
class SYMBOL : public SCH_ITEM
{
public:
    void init()
    {
        m_pinNameOffset = 0;
        m_showPinNames = true;
        m_showPinNumbers = true;
        m_excludedFromSim = false;
        m_excludedFromBOM = false;
        m_excludedFromBoard = false;
    };

    SYMBOL( KICAD_T idType ) :
            SCH_ITEM( nullptr, idType )
    {
        init();
    };

    SYMBOL( EDA_ITEM* aParent, KICAD_T idType ) :
            SCH_ITEM( aParent, idType )
    {
        init();
    };

    SYMBOL( const SYMBOL& base ) :
            SCH_ITEM( base ),
            m_pinNameOffset( base.m_pinNameOffset ),
            m_showPinNames( base.m_showPinNames ),
            m_showPinNumbers( base.m_showPinNumbers ),
            m_excludedFromSim( base.m_excludedFromSim ),
            m_excludedFromBOM( base.m_excludedFromBOM ),
            m_excludedFromBoard( base.m_excludedFromBoard )
    { };

    SYMBOL& operator=( const SYMBOL& aItem )
    {
        SCH_ITEM::operator=( aItem );

        m_pinNameOffset    = aItem.m_pinNameOffset;
        m_showPinNames      = aItem.m_showPinNames;
        m_showPinNumbers    = aItem.m_showPinNumbers;

        m_excludedFromSim   = aItem.m_excludedFromSim;
        m_excludedFromBOM   = aItem.m_excludedFromBOM;
        m_excludedFromBoard = aItem.m_excludedFromBoard;

        return *this;
    };

    virtual ~SYMBOL() { };

    virtual const LIB_ID& GetLibId() const = 0;
    virtual wxString GetDescription() const = 0;
    virtual wxString GetKeyWords() const = 0;

    virtual bool IsPower() const = 0;
    virtual bool IsNormal() const = 0;

    /**
     * Test if symbol has more than one body conversion type (DeMorgan).
     *
     * @return True if symbol has more than one conversion.
     */
    virtual bool HasAlternateBodyStyle() const = 0;

    /**
     * @return true if the symbol has multiple units per symbol.
     */
    virtual bool IsMulti() const = 0;

    /**
     * @return the number of units defined for the symbol.
     */
    virtual int GetUnitCount() const = 0;

    virtual const wxString GetRef( const SCH_SHEET_PATH* aSheet,
                                   bool aIncludeUnit = false ) const = 0;

    virtual const wxString GetValue( bool aResolve, const SCH_SHEET_PATH* aPath,
                                     bool aAllowExtraText ) const = 0;

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
    void SetShowPinNames( bool aShow ) { m_showPinNames = aShow; }
    bool GetShowPinNames() const { return m_showPinNames; }

    /**
     * Set or clear the pin number visibility flag.
     */
    void SetShowPinNumbers( bool aShow ) { m_showPinNumbers = aShow; }
    bool GetShowPinNumbers() const { return m_showPinNumbers; }

    /**
     * Set or clear the exclude from simulation flag.
     */
    void SetExcludedFromSim( bool aExcludeFromSim ) override { m_excludedFromSim = aExcludeFromSim; }
    bool GetExcludedFromSim() const override { return m_excludedFromSim; }

    /**
     * Set or clear the exclude from schematic bill of materials flag.
     */
    void SetExcludedFromBOM( bool aExcludeFromBOM ) { m_excludedFromBOM = aExcludeFromBOM; }
    bool GetExcludedFromBOM() const { return m_excludedFromBOM; }

    /**
     * Set or clear exclude from board netlist flag.
     */
    void SetExcludedFromBoard( bool aExcludeFromBoard ) { m_excludedFromBoard = aExcludeFromBoard; }
    bool GetExcludedFromBoard() const { return m_excludedFromBoard; }

    void ViewGetLayers( int aLayers[], int& aCount ) const override;

protected:
    int           m_pinNameOffset;         ///< The offset in mils to draw the pin name.  Set to
                                           ///<   0 to draw the pin name above the pin.
    bool          m_showPinNames;
    bool          m_showPinNumbers;

    bool          m_excludedFromSim;
    bool          m_excludedFromBOM;
    bool          m_excludedFromBoard;
};

#endif  //  SYMBOL_H
