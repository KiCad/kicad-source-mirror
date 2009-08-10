/*************************************/
/* class to handle Net Classes */
/**************************************/

#include "fctsys.h"

#include "common.h"
#include "kicad_string.h"
#include "pcbnew.h"


NET_DESIGN_PARAMS::NET_DESIGN_PARAMS()
{
    m_TracksWidth    = 170;             // "Default" value for tracks thickness used to route this net
    m_TracksMinWidth = 150;             // Minimum value for tracks thickness (used in DRC)
    m_ViasSize     = 550;               // "Default" value for vias sizes used to route this net
    m_ViasMinSize  = 400;               // Minimum value for  vias sizes (used in DRC)
    m_Clearance    = 140;               // "Default" clearance when routing
    m_MinClearance = 110;               // Minimum value for clearance (used in DRC)
}


/* A NETCLASS handles a list of nets and the parameters used to route or test (in DRC) these nets
 * This is mainly used in autotouters like Freeroute, but this can be also used in manual routing
 */

NETCLASS::NETCLASS( BOARD* aParent, const wxString& aName )
{
    m_Parent = aParent;
    m_Name   = aName;                   // Name of the net class
}


NETCLASS::~NETCLASS()
{
}


NETCLASS_LIST::NETCLASS_LIST( BOARD* aParent )
{
    m_Parent = aParent;
    std::vector <NETCLASS*> m_Netclass_List;
}


NETCLASS_LIST::~NETCLASS_LIST()
{
    ClearList();
}


void NETCLASS_LIST::ClearList()
{
    // the NETCLASS_LIST is owner of its items, so delete them
    for( unsigned ii = 0; ii < m_Netclass_List.size(); ii++ )
        delete m_Netclass_List[ii];

    m_Netclass_List.clear();
}


/** Function AddNetclass()
 * @param aNetclass = a pointer to the netclass to add
 * @return true if Ok, false if cannot be added (mainly because a netclass with the same name exists)
 */
bool NETCLASS_LIST::AddNetclass( NETCLASS* aNetclass )
{
    // Test for an existing netclass:
    for( unsigned ii = 0; ii < m_Netclass_List.size(); ii++ )
    {
        if( m_Netclass_List[ii]->m_Name.CmpNoCase( aNetclass->m_Name ) == 0 )
            return false;       // this netclass already exists
    }

    m_Netclass_List.push_back( aNetclass );
    return true;
}


void BOARD::TransfertDesignRulesToNets()
{
    // Clear .m_Flag member of nets (used to detect not in netclass list nets)
    for( unsigned ii = 1; ; ii++ )
    {
        NETINFO_ITEM* net = FindNet( ii );
        if( net == NULL )
            break;

        net->m_Flag = 0;
    }

    for( unsigned ii = 0; ii < m_NetClassesList.GetNetClassCount();  ii++ )
    {
        // Transfert rules and netclass name to nets:
        NETCLASS* netclass = m_NetClassesList.GetNetClass( ii );

        for( unsigned jj = 0; jj < netclass->GetMembersCount(); jj++ )
        {
            wxString      netname = netclass->GetMemberName( jj );
            NETINFO_ITEM* net     = FindNet( netname );

            if( net == NULL )    // This net does not exists: remove it
            {
                netclass->m_MembersNetNames.RemoveAt( jj );
                jj--;
            }
            else
            {
                net->SetClass( *netclass );
                net->m_Flag = 1;
            }
        }
    }

    // Now, set nets that do not have yet a netclass to default netclass
    NETCLASS* defaultnetclass = m_NetClassesList.GetNetClass( 0 );

    for( unsigned ii = 1; ; ii++ )
    {
        NETINFO_ITEM* net = FindNet( ii );

        if( net == NULL )
            break;

        if( net->m_Flag == 0 )
        {
            net->SetClass( *defaultnetclass );
            defaultnetclass->AddMember( net->GetNetname() );
        }
    }
}


/**
 * Function Save
 * writes the data structures for this object out to a FILE in "*.brd" format.
 * @param aFile The FILE to write to.
 * @return bool - true if success writing else false.
 */
bool NETCLASS_LIST::Save( FILE* aFile ) const
{
    bool success = true;

    for( unsigned ii = 0; ii < m_Netclass_List.size(); ii++ )
    {
        success = m_Netclass_List[ii]->Save( aFile );
        if( !success )
            break;
    }

    return success;
}


/**
 * Function Save
 * writes the data structures for this object out to a FILE in "*.brd" format.
 * @param aFile The FILE to write to.
 * @return bool - true if success writing else false.
 */
bool NETCLASS::Save( FILE* aFile ) const
{
    bool success = true;

    fprintf( aFile, "$NETCLASS\n" );
    fprintf( aFile, "Name \"%s\"\n", CONV_TO_UTF8( m_Name ) );

    // Write parameters
    success = m_NetParams.Save( aFile );

    // Write members list:
    for( unsigned ii = 0; ii < GetMembersCount(); ii++ )
        fprintf( aFile, "AddNet \"%s\"\n", CONV_TO_UTF8( GetMemberName( ii ) ) );

    fprintf( aFile, "$EndNETCLASS\n" );

    return success;
}


/**
 * Function Save
 * writes the data structures for this object out to a FILE in "*.brd" format.
 * @param aFile The FILE to write to.
 * @return bool - true if success writing else false.
 */
bool NET_DESIGN_PARAMS::Save( FILE* aFile ) const
{
    bool success = true;

    fprintf( aFile, "$PARAMS_START\n" );
    fprintf( aFile, "TracksWidth %d\n", m_TracksWidth );
    fprintf( aFile, "TracksMinWidth %d\n", m_TracksMinWidth );
    fprintf( aFile, "ViasSize %d\n", m_ViasSize );
    fprintf( aFile, "ViasMinSize %d\n", m_ViasMinSize );
    fprintf( aFile, "Clearance %d\n", m_Clearance );
    fprintf( aFile, "MinClearance %d\n", m_MinClearance );
    fprintf( aFile, "$PARAMS_END\n" );

    return success;
}


/**
 * Function ReadDescr
 * reads the data structures for this object from a FILE in "*.brd" format.
 * @param aFile The FILE to read to.
 * @return bool - true if success reading else false.
 */
bool NET_DESIGN_PARAMS::ReadDescr( FILE* aFile, int* aLineNum )
{
    bool success = true;
    char Line[1024];

    while( GetLine( aFile, Line, aLineNum, 1024 ) != NULL )
    {
        if( strnicmp( Line, "$PARAMS_END", 11 ) == 0 )
            return success;

        if( strnicmp( Line, "TracksWidth", 11 ) == 0 )
        {
            m_TracksWidth = atoi( Line + 11 );
            continue;
        }
        if( strnicmp( Line, "TracksMinWidth", 14 ) == 0 )
        {
            m_TracksMinWidth = atoi( Line + 14 );
            continue;
        }
        if( strnicmp( Line, "ViasSize", 8 ) == 0 )
        {
            m_ViasSize = atoi( Line + 8 );
            continue;
        }
        if( strnicmp( Line, "ViasMinSize", 11 ) == 0 )
        {
            m_ViasMinSize = atoi( Line + 11 );
            continue;
        }
        if( strnicmp( Line, "Clearance", 9 ) == 0 )
        {
            m_Clearance = atoi( Line + 9 );
            continue;
        }
        if( strnicmp( Line, "MinClearance", 12 ) == 0 )
        {
            m_MinClearance = atoi( Line + 12 );
            continue;
        }
    }

    return success;
}


/**
 * Function ReadDescr
 * reads the data structures for this object from a FILE in "*.brd" format.
 * @param aFile The FILE to read to.
 * @return bool - true if success reading else false.
 */
bool NETCLASS::ReadDescr( FILE* aFile, int* aLineNum )
{
    bool success = true;
    char Line[1024];
    char Buffer[1024];

    while( GetLine( aFile, Line, aLineNum, 1024 ) != NULL )
    {
        if( strnicmp( Line, "$endNETCLASS", 6 ) == 0 )
            return success;

        if( strnicmp( Line, "$PARAMS_START", 13 ) == 0 )
        {
            m_NetParams.ReadDescr( aFile, aLineNum );
            continue;
        }

        if( strnicmp( Line, "Name", 4 ) == 0 )
        {
            ReadDelimitedText( Buffer, Line + 4, sizeof(Buffer) );
            m_Name = CONV_FROM_UTF8( Buffer );
        }

        if( strnicmp( Line, "AddNet", 6 ) == 0 )
        {
            ReadDelimitedText( Buffer, Line + 6, sizeof(Buffer) );
            wxString netname = CONV_FROM_UTF8( Buffer );
            AddMember( netname );
        }
    }

    return success;
}
