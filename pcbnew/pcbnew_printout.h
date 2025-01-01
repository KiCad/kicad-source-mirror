/*
 * Copyright (C) 2018 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef PCBNEW_PRINTOUT_H
#define PCBNEW_PRINTOUT_H

#include <board_printout.h>
#include <lset.h>
#include <pcb_painter.h>
#include <plotprint_opts.h>

class BOARD;

struct PCBNEW_PRINTOUT_SETTINGS : BOARD_PRINTOUT_SETTINGS
{
    PCBNEW_PRINTOUT_SETTINGS( const PAGE_INFO& aPageInfo );

    enum DRILL_MARKS m_DrillMarks;

    enum PAGINATION_T {
        LAYER_PER_PAGE,
        ALL_LAYERS
    } m_Pagination;

    bool m_PrintEdgeCutsOnAllPages;  ///< Print board outline on each page
    bool m_AsItemCheckboxes;         ///< Honor checkboxes in the Items tab of the Layers Manager

    void Load( APP_SETTINGS_BASE* aConfig ) override;
    void Save( APP_SETTINGS_BASE* aConfig ) override;
};


class PCBNEW_PRINTOUT : public BOARD_PRINTOUT
{
public:
    PCBNEW_PRINTOUT( BOARD* aBoard, const PCBNEW_PRINTOUT_SETTINGS& aParams,
                     const KIGFX::VIEW* aView, const wxString& aTitle );

    bool OnPrintPage( int aPage ) override;

protected:
    int milsToIU( double aMils ) const override;

    void setupViewLayers( KIGFX::VIEW& aView, const LSET& aLayerSet ) override;

    void setupPainter( KIGFX::PAINTER& aPainter ) override;

    void setupGal( KIGFX::GAL* aGal ) override;

    BOX2I getBoundingBox() override;

    std::unique_ptr<KIGFX::PAINTER> getPainter( KIGFX::GAL* aGal ) override;

private:
    BOARD*                   m_board;
    PCBNEW_PRINTOUT_SETTINGS m_pcbnewSettings;
};


namespace KIGFX {

/**
 * Special flavor of PCB_PAINTER that contains modifications to handle printing options.
 */
class PCB_PRINT_PAINTER : public PCB_PAINTER
{
public:
    PCB_PRINT_PAINTER( GAL* aGal );

    /**
     * Set drill marks visibility and options.
     *
     * @param aRealSize when enabled, drill marks represent actual holes. Otherwise aSize
     *                  parameter is used.
     * @param aSize is drill mark size (internal units), valid only when aRealSize == false.
     */
    void SetDrillMarks( bool aRealSize, unsigned int aSize = 0 )
    {
        m_drillMarkReal = aRealSize;
        m_drillMarkSize = aSize;
    }

protected:
    PAD_DRILL_SHAPE getDrillShape( const PAD* aPad ) const override;

    SHAPE_SEGMENT getPadHoleShape( const PAD* aPad ) const override;

    int getViaDrillSize( const PCB_VIA* aVia ) const override;

protected:
    bool m_drillMarkReal;    ///< Actual hole size or user-specified size for drill marks
    int  m_drillMarkSize;    ///< User-specified size (in internal units)
};

}; // namespace KIGFX

#endif /* PCBNEW_PRINTOUT_H */
