
// Rename Add(), Remove() and Delete() to {Add,Remove,Delete}Native() and
// then implement Add() Remove() and Delete() in python so we can manage
// the ownership flag: thisown.
%rename(AddNative)       BOARD_ITEM_CONTAINER::Add;
%rename(RemoveNative)    BOARD_ITEM_CONTAINER::Remove;
%rename(DeleteNative)    BOARD_ITEM_CONTAINER::Delete;

%include board_item_container.h
%{
#include <board_item_container.h>
%}

%extend BOARD_ITEM_CONTAINER
{
    %pythoncode
    %{
    def Add(self,item):
        """
        Add a BOARD_ITEM to this BOARD_ITEM_CONTAINER, clear the thisown to prevent
        python from deleting the object in the garbage collector
        Add(BOARD_ITEM_CONTAINER self, BOARD_ITEM aItem, BOARD_ADD_MODE aMode=BOARD_ADD_MODE::INSERT)
        Add(BOARD_ITEM_CONTAINER self, BOARD_ITEM aItem)
        """
        item.thisown=0
        self.AddNative(item)

    def Remove(self,item):
        """
        Remove a BOARD_ITEM from this BOARD_ITEM_CONTAINER, set the thisdown flag so that
        the python wrapper owns the C++ BOARD_ITEM
        Remove(self, BOARD_ITEM)
        """
        self.RemoveNative(item)
        if (not IsActionRunning()):
            item.thisown=1

    def Delete(self,item):
        """
        Remove a BOARD_ITEM from this BOARD_ITEM_CONTAINER, set the thisdown flag so that
        the python wrapper does not owns the C++ BOARD_ITEM
        Delete(self, BOARD_ITEM)
        """
        item.thisown=0          # C++'s BOARD_ITEM_CONTAINER::Delete() will delete
        self.DeleteNative(item)
        item.this = None
    %}
}
