/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Alexander Shuklin, jasuramme@gmail.com
 * Copyright (C) 2019-2021 KiCad Developers, see AUTHORS.txt for contributors.
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


#ifndef _DIALOG_BOARD_STATISTICS_H
#define _DIALOG_BOARD_STATISTICS_H


#include <base_units.h>
#include <board.h>
#include <track.h>
#include <confirm.h>
#include <dialog_board_statistics_base.h>
#include <pad_shapes.h>
#include <pcb_base_frame.h>
#include <pcb_edit_frame.h>
#include <project.h>
#include <wx/datetime.h>

/**
 * Dialog to show common board info.
 */
class DIALOG_BOARD_STATISTICS : public DIALOG_BOARD_STATISTICS_BASE
{
public:
    /**
     * Type information, which will be shown in dialog.
     */
    template <typename T>
    struct typeContainer_t
    {
        typeContainer_t<T>( T aAttribute, wxString aTitle )
                : attribute( aAttribute ),
                  title( aTitle ),
                  qty( 0 )
        {
        }

        T          attribute;
        wxString   title;
        int        qty;
    };

    using padsType_t = typeContainer_t<PAD_ATTRIB>;
    using viasType_t = typeContainer_t<VIATYPE>;

    /**
     * Footprint attributes (such as SMD, THT, Virtual and so on), which will be shown in the
     * dialog. Holds both front and bottom components quantities.
     */
    struct componentsType_t
    {
        componentsType_t( FOOTPRINT_ATTR_T aAttribute, wxString aTitle )
                : attribute( aAttribute ),
                  title( aTitle ),
                  frontSideQty( 0 ),
                  backSideQty( 0 )
        {
        }

        FOOTPRINT_ATTR_T attribute;
        wxString         title;
        int              frontSideQty;
        int              backSideQty;
    };

    struct drillType_t
    {
        enum COL_ID
        {
            COL_COUNT,
            COL_SHAPE,
            COL_X_SIZE,
            COL_Y_SIZE,
            COL_PLATED,
            COL_VIA_PAD,
            COL_START_LAYER,
            COL_STOP_LAYER
        };

        drillType_t( int aXSize, int aYSize, PAD_DRILL_SHAPE_T aShape, bool aIsPlated, bool aIsPad,
                PCB_LAYER_ID aStartLayer, PCB_LAYER_ID aStopLayer, int aQty = 0 )
                : xSize( aXSize ),
                  ySize( aYSize ),
                  shape( aShape ),
                  isPlated( aIsPlated ),
                  isPad( aIsPad ),
                  startLayer( aStartLayer ),
                  stopLayer( aStopLayer ),
                  qty( aQty )
        {
        }

        bool operator==( const drillType_t& other )
        {
            return xSize == other.xSize && ySize == other.ySize && shape == other.shape
                   && isPlated == other.isPlated && isPad == other.isPad
                   && startLayer == other.startLayer && stopLayer == other.stopLayer;
        }

        struct COMPARE
        {
            COMPARE( COL_ID aColId, bool aAscending ) : colId( aColId ), ascending( aAscending )
            {
            }
            bool operator()( const drillType_t& aLeft, const drillType_t& aRight )
            {
                switch( colId )
                {
                case COL_COUNT:
                    return compareDrillParameters( aLeft.qty, aRight.qty );
                case COL_SHAPE:
                    return compareDrillParameters( aLeft.shape, aRight.shape );
                case COL_X_SIZE:
                    return compareDrillParameters( aLeft.xSize, aRight.xSize );
                case COL_Y_SIZE:
                    return compareDrillParameters( aLeft.ySize, aRight.ySize );
                case COL_PLATED:
                    return ascending ? aLeft.isPlated : aRight.isPlated;
                case COL_VIA_PAD:
                    return ascending ? aLeft.isPad : aRight.isPad;
                case COL_START_LAYER:
                    return compareDrillParameters( aLeft.startLayer, aRight.startLayer );
                case COL_STOP_LAYER:
                    return compareDrillParameters( aLeft.stopLayer, aRight.stopLayer );
                }

                return false;
            }

            bool compareDrillParameters( int aLeft, int aRight )
            {
                return ascending ? aLeft < aRight : aLeft > aRight;
            }

            COL_ID colId;
            bool   ascending;
        };

        int               xSize;
        int               ySize;
        PAD_DRILL_SHAPE_T shape;
        bool              isPlated;
        bool              isPad;
        PCB_LAYER_ID      startLayer;
        PCB_LAYER_ID      stopLayer;
        int               qty;
    };

    using componentsTypeList_t = std::deque<componentsType_t>;
    using padsTypeList_t = std::deque<padsType_t>;
    using viasTypeList_t = std::deque<viasType_t>;
    using drillTypeList_t = std::deque<drillType_t>;

    DIALOG_BOARD_STATISTICS( PCB_EDIT_FRAME* aParentFrame );
    ~DIALOG_BOARD_STATISTICS();

    ///< Get data from the PCB board and print it to dialog
    bool TransferDataToWindow() override;

private:

    ///< Function to fill up all items types to be shown in the dialog.
    void refreshItemsTypes();

    ///< Get data from board.
    void getDataFromPCB();

    ///< Apply data to dialog widgets.
    void updateWidets();

    ///< Update drills grid.
    void updateDrillGrid();

    ///< Print grid to string in tabular format.
    void printGridToStringAsTable( wxGrid* aGrid, wxString& aStr, bool aUseRowLabels,
            bool aUseColLabels, bool aUseFirstColAsLabel );

    void adjustDrillGridColumns();

    void checkboxClicked( wxCommandEvent& aEvent ) override;

    ///< Save board statistics to a file
    void saveReportClicked( wxCommandEvent& aEvent ) override;

    void drillGridSize( wxSizeEvent& aEvent ) override;

    void drillGridSort( wxGridEvent& aEvent );

    PCB_EDIT_FRAME* m_parentFrame;

    int             m_boardWidth;
    int             m_boardHeight;
    double          m_boardArea;

    ///< Show if board outline properly defined.
    bool m_hasOutline;

    ///< Hold all components types to be shown in the dialog.
    componentsTypeList_t m_componentsTypes;

    ///< Hold all pads types to be shown in the dialog.
    padsTypeList_t  m_padsTypes;

    ///< Hold all vias types to be shown in the dialog.
    viasTypeList_t m_viasTypes;

    ///< Hold all drill hole types to be shown in the dialog.
    drillTypeList_t m_drillTypes;
};

#endif // __DIALOG_BOARD_STATISTICS_H
