#pragma once

#include "dialog_shim.h"
#include "string_utils.h"

#include <memory>
#include <vector>
#include <pad.h>

class PCB_BASE_FRAME;
class FOOTPRINT;
class WX_GRID;
class wxStaticText;
class UNITS_PROVIDER;

class DIALOG_FP_EDIT_PAD_TABLE : public DIALOG_SHIM
{
public:
    // Column indices (after adding Type column)
    enum COLS {
        COL_NUMBER = 0,
        COL_TYPE,
        COL_SHAPE,
        COL_POS_X,
        COL_POS_Y,
        COL_SIZE_X,
        COL_SIZE_Y,
        COL_DRILL_X,
        COL_DRILL_Y,
        COL_P2D_LENGTH,
        COL_P2D_DELAY
    };

    DIALOG_FP_EDIT_PAD_TABLE( PCB_BASE_FRAME* aParent, FOOTPRINT* aFootprint );
    ~DIALOG_FP_EDIT_PAD_TABLE() override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;
    void CaptureOriginalPadState();
    void RestoreOriginalPadState();

private:
    void OnSize( wxSizeEvent& aEvent );
    void OnCharHook( wxKeyEvent& aEvent ) override;
    void OnCellChanged( wxGridEvent& aEvent );
    void OnSelectCell( wxGridEvent& aEvent );
    void OnUpdateUI( wxUpdateUIEvent& aEvent );

    void InitColumnProportions();

    void updateSummary();
    void setRowNullableEditors( int aRowId ) const;

    PAD* getPadForRow( int aRowId ) const;

private:
    PCB_BASE_FRAME*     m_frame;

    // Proportional resize support
    std::vector<double> m_colProportions;    // relative widths captured after init
    std::vector<int>    m_minColWidths;      // initial (minimum) widths

    struct PAD_SNAPSHOT
    {
        explicit PAD_SNAPSHOT( PAD* aPad ) :
                padstack( aPad )
        {
        }

        wxString   number;
        PAD_SHAPE  shape{ PAD_SHAPE::CHAMFERED_RECT };
        PADSTACK   padstack;
        VECTOR2I   position;
        VECTOR2I   size;
        PAD_ATTRIB attribute{ PAD_ATTRIB::PTH };
        int        padToDieLength{ 0 };
        int        padToDieDelay{ 0 };
    };

    // Comparison function to order the pads in the map
    struct PAD_SNAPSHOT_COMPARE
    {
        bool operator()( const PAD* a, const PAD* b ) const
        {
            const int cmpVal = StrNumCmp( a->GetNumber(), b->GetNumber() );

            // First sort by alphanumeric ordering
            if( cmpVal < 0 )
                return true;

            if( cmpVal > 0 )
                return false;

            // Sort by x and then y
            if( a->GetCenter().x < b->GetCenter().x )
                return true;

            if( a->GetCenter().x > b->GetCenter().x )
                return false;

            if( a->GetCenter().y < b->GetCenter().y )
                return true;

            if( a->GetCenter().y > b->GetCenter().y )
                return false;

            // For degenerate pads, sort by raw pointer value
            return a < b;
        }
    };

    std::map<PAD*, PAD_SNAPSHOT, PAD_SNAPSHOT_COMPARE> m_originalPads;      // original pad data for cancel rollback
    bool                      m_cancelled = false; // set if user hit cancel

    wxStaticText*                     m_staticTextPinNumbers;
    wxStaticText*                     m_pin_numbers_summary;
    wxStaticText*                     m_staticTextPinCount;
    wxStaticText*                     m_pin_count;
    wxStaticText*                     m_staticTextDuplicatePins;
    wxStaticText*                     m_duplicate_pins;
    WX_GRID*                          m_grid;
    FOOTPRINT*                        m_footprint;
    std::unique_ptr<UNITS_PROVIDER>   m_unitsProvider;
    bool                              m_summaryDirty;
};
