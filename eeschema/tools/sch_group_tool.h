#include <tool/group_tool.h>
#include <sch_commit.h>
#include <sch_group.h>

class SCH_GROUP_TOOL : public GROUP_TOOL
{
public:
    /**
     * Invoke the picker tool to select a new member of the group.
     */
    int PickNewMember( const TOOL_EVENT& aEvent ) override;

    ///< Group selected items.
    int Group( const TOOL_EVENT& aEvent ) override;

protected:
    std::shared_ptr<COMMIT> createCommit() override { return std::make_shared<SCH_COMMIT>( m_toolMgr ); }

    EDA_GROUP* getGroupFromItem( EDA_ITEM* aItem ) override
    {
        if( aItem->Type() == SCH_GROUP_T )
            return static_cast<SCH_GROUP*>( aItem );

        return nullptr;
    }
};
