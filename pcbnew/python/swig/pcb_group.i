

%{
#include <eda_group.h>
#include <pcb_group.h>
%}
%include ../common/eda_group.h
%include pcb_group.h

%extend PCB_GROUP
{
    std::deque<BOARD_ITEM*> GetItemsDeque()
    {
        std::deque<BOARD_ITEM*> result;

        const std::unordered_set<BOARD_ITEM*> items = $self->GetBoardItems();

        for(BOARD_ITEM* item: items)
            result.push_back(item);

        return result;
    }

    %pythoncode
    %{
        def GetItems(self):
            return [item.Cast() for item in self.GetItemsDeque()]
    %}
}
