#ifndef __PCB_TEST_SELECTION_TOOL_H
#define __PCB_TEST_SELECTION_TOOL_H

#include <tool/action_menu.h>
#include <tool/selection_tool.h>
#include <tool/tool_menu.h>
#include <tools/pcb_selection_conditions.h>
#include <tools/pcb_tool_base.h>
#include <tools/pcb_selection.h>

class PCB_TEST_FRAME_BASE;

class PCB_TEST_SELECTION_TOOL : public SELECTION_TOOL
{
public:
    PCB_TEST_SELECTION_TOOL();
    virtual ~PCB_TEST_SELECTION_TOOL();

    /// @copydoc TOOL_BASE::Init()
    bool Init() override;

    /// @copydoc TOOL_BASE::Reset()
    void Reset( RESET_REASON aReason ) override;

    int Main( const TOOL_EVENT& aEvent );

    PCB_SELECTION& GetSelection() { return m_selection; }

    void ClearSelection();

    void SetSelectionHook( std::function<void(PCB_TEST_FRAME_BASE*, PCB_SELECTION*)> aHook )
    {
        m_selectionHook = aHook;
    }

    void SetSelectableItemTypes( const std::vector<KICAD_T> aTypes );

protected:
    const GENERAL_COLLECTORS_GUIDE getCollectorsGuide() const;
    bool                           selectPoint( const VECTOR2I& aWhere );

    void setTransitions() override;

    KIGFX::PCB_VIEW* view() const { return static_cast<KIGFX::PCB_VIEW*>( getView() ); }

    KIGFX::VIEW_CONTROLS* controls() const { return getViewControls(); }

    PCB_TEST_FRAME_BASE* frame() const { return getEditFrame<PCB_TEST_FRAME_BASE>(); }

    BOARD* board() const { return getModel<BOARD>(); }

    PCB_DRAW_PANEL_GAL* canvas() const
    {
        return static_cast<PCB_DRAW_PANEL_GAL*>( frame()->GetPanel().get() );
    }

    SELECTION& selection() override { return m_selection; }


    void highlightInternal( EDA_ITEM* aItem, int aMode, bool aUsingOverlay );
    void unhighlight( EDA_ITEM* aItem, int aMode, SELECTION* aGroup ) override;
    void unhighlightInternal( EDA_ITEM* aItem, int aMode, bool aUsingOverlay );
    void highlight( EDA_ITEM* aItem, int aMode, SELECTION* aGroup ) override;
    void select( EDA_ITEM* aItem ) override;
    void unselect( EDA_ITEM* aItem ) override;


private:
    std::function<void(PCB_TEST_FRAME_BASE*,PCB_SELECTION*)> m_selectionHook;
    PCB_SELECTION m_selection; // Current state of selection
    std::vector<KICAD_T> m_selectableTypes;
};


#endif
