/********************************************/
/* Definitions for the EESchema program:	*/
/********************************************/

#ifndef CLASS_SCREEN_H
#define CLASS_SCREEN_H

#ifndef eda_global
#define eda_global extern
#endif

#include "macros.h"
#include "base_struct.h"

/* Max number of sheets in a hierarchy project: */
#define NB_MAX_SHEET 500


class SCH_SCREEN : public BASE_SCREEN
{
public:
    int m_RefCount; //how many sheets reference this screen?
                    //delete when it goes to zero.
    int m_ScreenNumber;
    int m_NumberOfScreen;
    SCH_SCREEN( int idtype, KICAD_T aType = SCREEN_STRUCT_TYPE );
    ~SCH_SCREEN();

    virtual wxString GetClass() const
    {
        return wxT( "SCH_SCREEN" );
    }


    void            FreeDrawList(); // Free EESchema drawing list (does not delete the sub hierarchies)

    void Place( WinEDA_DrawFrame* frame, wxDC* DC ) { };
    void            RemoveFromDrawList( SCH_ITEM* DrawStruct ); /* remove DrawStruct from EEDrawList. */
    bool            CheckIfOnDrawList( SCH_ITEM* st );
    void            AddToDrawList( SCH_ITEM* DrawStruct );
    void            ClearUndoORRedoList( EDA_BaseStruct* List );

    bool            SchematicCleanUp( wxDC* DC = NULL );
    SCH_ITEM*       ExtractWires( bool CreateCopy );

    /* full undo redo management : */
    virtual void    ClearUndoRedoList();
    virtual void    AddItemToUndoList( EDA_BaseStruct* item );
    virtual void    AddItemToRedoList( EDA_BaseStruct* item );
};


/********************************************************/
/* Class to handle the list of *screens* in a hierarchy */
/********************************************************/

// screens are unique, and correspond to .sch files.
WX_DEFINE_ARRAY( SCH_SCREEN *, ScreenGrowArray );
class EDA_ScreenList
{
private:
    ScreenGrowArray m_List;
    unsigned int    m_Index;

public:
    EDA_ScreenList();
    ~EDA_ScreenList() { }
    int GetCount() { return m_List.GetCount(); }
    SCH_SCREEN* GetFirst();
    SCH_SCREEN* GetNext();
    SCH_SCREEN* GetScreen( unsigned int index );

private:
    void            AddScreenToList( SCH_SCREEN* testscreen );
    void            BuildScreenList( EDA_BaseStruct* sheet );
};

#endif /* CLASS_SCREEN_H */
