#ifndef SCH_VIEW_H_
#define SCH_VIEW_H_

#include <memory>
#include <view/view.h>
#include <math/box2.h>

#include <view/wx_view_controls.h>
#include <worksheet_viewitem.h>
#include <layers_id_colors_and_visibility.h>

class SCH_SHEET;
class SCH_SCREEN;
class LIB_PART;

namespace KIGFX
{
    class VIEW_GROUP;
    class WORKSHEET_VIEWITEM;

    namespace PREVIEW
    {
        class SELECTION_AREA;
    };

class SCH_VIEW : public KIGFX::VIEW
{
public:
    SCH_VIEW( bool aIsDynamic );
    ~SCH_VIEW();

    virtual void Add( VIEW_ITEM* aItem, int aDrawPriority = -1 ) override;
    virtual void Remove( VIEW_ITEM* aItem ) override;

    void DisplaySheet( SCH_SHEET *aSheet );
    void DisplaySheet( SCH_SCREEN *aSheet );
    void DisplayComponent( LIB_PART *aPart );

    KIGFX::PREVIEW::SELECTION_AREA* GetSelectionArea() const { return m_selectionArea.get(); }

    KIGFX::VIEW_GROUP* GetPreview() const { return m_preview.get(); }

    void ClearPreview();
    void AddToPreview( EDA_ITEM *aItem, bool makeCopy = true );

    void ShowSelectionArea( bool aShow = true );
    void ShowPreview( bool aShow = true );

    void ClearHiddenFlags();
    void HideWorksheet();

    virtual void Redraw() override;

private:
    std::unique_ptr<WORKSHEET_VIEWITEM> m_worksheet;
    std::unique_ptr<KIGFX::PREVIEW::SELECTION_AREA> m_selectionArea;
    std::unique_ptr<KIGFX::VIEW_GROUP> m_preview;
    std::vector<EDA_ITEM *> m_previewItems;
    std::unordered_map<VIEW_ITEM*, BOX2I> m_cachedBBoxes;
};

}; // namespace

#endif
