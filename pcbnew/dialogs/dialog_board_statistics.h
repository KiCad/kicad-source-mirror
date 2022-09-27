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


#include <board.h>
#include <footprint.h>
#include <pcb_track.h>
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
    struct LINE_ITEM
    {
        LINE_ITEM<T>( T aAttribute, const wxString& aTitle ) :
                attribute( aAttribute ),
                title( aTitle ),
                qty( 0 )
        {
        }

        T          attribute;
        wxString   title;
        int        qty;
    };

    /**
     * Footprint attributes (such as SMD, THT, Virtual and so on), which will be shown in the
     * dialog. Holds both front and back footprint quantities.
     */
    struct FP_LINE_ITEM
    {
        FP_LINE_ITEM( int aAttributeMask, int aAttributeValue, wxString aTitle ) :
                attribute_mask( aAttributeMask ),
                attribute_value( aAttributeValue ),
                title( aTitle ),
                frontSideQty( 0 ),
                backSideQty( 0 )
        {
        }

        int      attribute_mask;
        int      attribute_value;
        wxString title;
        int      frontSideQty;
        int      backSideQty;
    };

    struct DRILL_LINE_ITEM
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

        DRILL_LINE_ITEM( int aXSize, int aYSize, PAD_DRILL_SHAPE_T aShape, bool aIsPlated,
                         bool aIsPad, PCB_LAYER_ID aStartLayer, PCB_LAYER_ID aStopLayer ) :
                xSize( aXSize ),
                ySize( aYSize ),
                shape( aShape ),
                isPlated( aIsPlated ),
                isPad( aIsPad ),
                startLayer( aStartLayer ),
                stopLayer( aStopLayer ),
                qty( 0 )
        {
        }

        bool operator==( const DRILL_LINE_ITEM& other )
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
            bool operator()( const DRILL_LINE_ITEM& aLeft, const DRILL_LINE_ITEM& aRight )
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
    void printGridToStringAsTable( wxGrid* aGrid, wxString& aStr, bool aUseColLabels,
                                   bool aUseFirstColAsLabel );

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

    bool            m_hasOutline;          ///< Show if board outline properly defined.

    std::deque<FP_LINE_ITEM>          m_fpTypes;
    std::deque<LINE_ITEM<PAD_ATTRIB>> m_padTypes;
    std::deque<LINE_ITEM<VIATYPE>>    m_viaTypes;
    std::deque<DRILL_LINE_ITEM>       m_drillTypes;

    int m_startLayerColInitialSize;        ///< Width of the start layer column as calculated by
                                           ///<    the wxWidgets autosizing algorithm.
    int m_stopLayerColInitialSize;         ///< Width of the stop layer column.
};

#endif // __DIALOG_BOARD_STATISTICS_H
