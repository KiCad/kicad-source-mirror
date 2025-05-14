#include <tool/group_tool.h>
#include <board_commit.h>
#include <pcb_group.h>

class PCB_GROUP_TOOL : public GROUP_TOOL
{
public:
    /**
     * Invoke the picker tool to select a new member of the group.
     */
    int PickNewMember( const TOOL_EVENT& aEvent ) override;

    ///< Group selected items.
    int Group( const TOOL_EVENT& aEvent ) override;

protected:
    std::shared_ptr<COMMIT> createCommit() override { return std::make_shared<BOARD_COMMIT>( this ); }

    EDA_GROUP* getGroupFromItem( EDA_ITEM* aItem ) override
    {
        if( aItem->Type() == PCB_GROUP_T )
            return static_cast<PCB_GROUP*>( aItem );

        return nullptr;
    }
};
