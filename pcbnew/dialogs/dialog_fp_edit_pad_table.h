#pragma once

#include "dialog_shim.h"
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

private:
    // Proportional resize support
    std::vector<double> m_colProportions;    // relative widths captured after init
    std::vector<int>    m_minColWidths;      // initial (minimum) widths

    struct PAD_SNAPSHOT
    {
        wxString   number;
        PAD_SHAPE  shape;
        VECTOR2I   position;
        VECTOR2I   size;
        PAD_ATTRIB attribute;
        VECTOR2I   drillSize;
        int        padToDieLength = 0;
        int        padToDieDelay  = 0;
    };

    std::vector<PAD_SNAPSHOT> m_originalPads; // original pad data for cancel rollback
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

