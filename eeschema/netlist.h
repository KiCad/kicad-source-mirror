/***************/
/*  netlist.h  */
/***************/

#ifndef _NETLIST_H_
#define _NETLIST_H_


#include "macros.h"

#include "class_libentry.h"


class SCH_COMPONENT;


#define NETLIST_HEAD_STRING "EESchema Netlist Version 1.1"

#define ISBUS 1

#define CUSTOMPANEL_COUNTMAX 8  // Max number of netlist plugins

#include "class_netlist_object.h"

/* Id to select netlist type */
enum  TypeNetForm {
    NET_TYPE_UNINIT = 0,
    NET_TYPE_PCBNEW,
    NET_TYPE_ORCADPCB2,
    NET_TYPE_CADSTAR,
    NET_TYPE_SPICE,
    NET_TYPE_CUSTOM1,   /* NET_TYPE_CUSTOM1
                         * is the first id for user netlist format
                         * NET_TYPE_CUSTOM1+CUSTOMPANEL_COUNTMAX-1
                         * is the last id for user netlist format
                         */
    NET_TYPE_CUSTOM_MAX = NET_TYPE_CUSTOM1 + CUSTOMPANEL_COUNTMAX - 1
};


/* Max pin number per component and footprint */
#define MAXPIN 5000


/* object used in annotation to handle a list of components in schematic
 * because in a complex hierarchy, a component is used more than once,
 * and its reference is depending on the sheet path
 * for the same component, we must create a flat list of components
 * used in nelist generation, BOM generation and annotation
 */
class OBJ_CMP_TO_LIST
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

    OBJ_CMP_TO_LIST()
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


    int CompareValue( const OBJ_CMP_TO_LIST& item ) const
    {
        return m_Value->CmpNoCase( *item.m_Value );
    }


    int CompareRef( const OBJ_CMP_TO_LIST& item ) const
    {
        return m_Ref.compare( item.m_Ref );
    }


    bool IsPartsLocked()
    {
        return m_Entry->UnitsLocked();
    }
};


/* Global Variables */

// Buffer to build the list of items used in netlist and erc calculations
typedef std::vector <NETLIST_OBJECT*> NETLIST_OBJECT_LIST;
extern NETLIST_OBJECT_LIST g_NetObjectslist;

extern bool g_OptNetListUseNames;   /* TRUE to use names rather than
                                     * net numbers. SPICE netlist only
                                     */

/* Prototypes: */
void     FreeNetObjectsList( std::vector <NETLIST_OBJECT*>& aNetObjectslist );

/**
 * Function ReturnUserNetlistTypeName
 * to retrieve user netlist type names
 * @param first_item = true: return first name of the list, false = return next
 * @return a wxString : name of the type netlist or empty string
 * this function must be called first with "first_item" = true
 * and after with "first_item" = false to get all the other existing netlist
 * names
 */
wxString ReturnUserNetlistTypeName( bool first_item );


#endif
