/***************/
/*  netlist.h  */
/***************/

#ifndef _NETLIST_H_
#define _NETLIST_H_


#include "macros.h"

#include "class_libentry.h"
#include "sch_sheet_path.h"


class SCH_COMPONENT;


#define NETLIST_HEAD_STRING "EESchema Netlist Version 1.1"

#define ISBUS 1

/* Max pin number per component and footprint */
#define MAXPIN 5000


/**
 * Class SCH_REFERENCE
 * is used as a helper to define a component's reference designator in a schematic.  This
 * helper is required in a complex hierarchy because a component can be used more than
 * once and its reference depends on the sheet path.  This class is used to flatten the
 * schematic hierarchy for annotation, net list generation, and bill of material
 * generation.
 */
class SCH_REFERENCE
{
private:
    /// Component reference prefix, without number (for IC1, this is IC) )
    std::string    m_Ref;               // it's private, use the accessors please


public:
    SCH_COMPONENT* m_RootCmp;           // the component in schematic
    LIB_COMPONENT* m_Entry;             // the source component in library
    int            m_Unit;              /* Selected part (For multi parts per
                                        * package) depending on sheet path */
    SCH_SHEET_PATH m_SheetPath;         /* the sheet path for this component */
    unsigned long  m_TimeStamp;         /* unique identification number
                                         * depending on sheet path */
    bool           m_IsNew;             /* true for not yet annotated
                                         * components */
    wxString*      m_Value;             /* Component value (same for all
                                         * instances) */
    int            m_NumRef;            /* Reference number (for IC1, this is
                                         * 1) ) depending on sheet path*/
    int            m_Flag;              /* flag for computations */

public:

    SCH_REFERENCE()
    {
        m_RootCmp      = NULL;
        m_Entry        = NULL;
        m_Unit         = 0;
        m_TimeStamp    = 0;
        m_IsNew        = false;
        m_Value        = NULL;
        m_NumRef       = 0;
        m_Flag         = 0;
    }

    SCH_REFERENCE( SCH_COMPONENT* aComponent, LIB_COMPONENT* aLibComponent,
                   SCH_SHEET_PATH& aSheetPath );

    /**
     * Function Annotate
     * updates the annotation of the component according the the current object state.
     */
    void Annotate();

    /**
     * Function Split
     * attempts to split the reference designator into a name (U) and number (1).  If the
     * last character is '?' or not a digit, the reference is tagged as not annotated.
     * For components with multiple parts per package that are not already annotated, set
     * m_Unit to a max value (0x7FFFFFFF).
     */
    void Split();

    /*  Some accessors which hide the strategy of how the reference is stored,
        thereby making it easy to change that strategy.
    */


    void SetRef( const wxString& aReference )
    {
        m_Ref =  CONV_TO_UTF8( aReference );
    }
    wxString GetRef() const
    {
        return CONV_FROM_UTF8( m_Ref.c_str() );
    }
    void SetRefStr( const std::string& aReference )
    {
        m_Ref = aReference;
    }
    const char* GetRefStr() const
    {
        return m_Ref.c_str();
    }


    int CompareValue( const SCH_REFERENCE& item ) const
    {
        return m_Value->CmpNoCase( *item.m_Value );
    }


    int CompareRef( const SCH_REFERENCE& item ) const
    {
        return m_Ref.compare( item.m_Ref );
    }


    bool IsPartsLocked()
    {
        return m_Entry->UnitsLocked();
    }
};


#endif
