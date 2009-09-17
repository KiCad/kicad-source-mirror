/*! \file src/graphlst.cpp
    \brief Implements a list of graphs
    \author Klaas Holwerda 
 
    Copyright: 2001-2004 (C) Klaas Holwerda 
 
    Licence: see kboollicense.txt 
 
    RCS-ID: $Id: graphlst.cpp,v 1.4 2009/09/10 17:04:09 titato Exp $
*/

//#include "debugdrv.h"
#include "kbool/booleng.h"
#include "kbool/graphlst.h"

//extern Debug_driver* _debug_driver;
//this here is to initialize the static iterator of graphlist
//with NOLIST constructor

int  graphsorterX( kbGraph *, kbGraph * );
int  graphsorterY( kbGraph *, kbGraph * );

kbGraphList::kbGraphList( Bool_Engine* GC )
{
    _GC = GC;
}

kbGraphList::kbGraphList( kbGraphList* other )
{
    _GC = other->_GC;

    TDLI<kbGraph> _LI = TDLI<kbGraph>( other );
    _LI.tohead();
    while ( !_LI.hitroot() )
    {
        insend( new kbGraph( _LI.item() ) );
        _LI++;
    }
}

kbGraphList::~kbGraphList()
{
    TDLI<kbGraph> _LI = TDLI<kbGraph>( this );
    //first empty the graph
    _LI.delete_all();
}

//prepare the graphlist for the boolean operations
//group all graphs into ONE graph
void kbGraphList::Prepare( kbGraph* total )
{
    if ( empty() )
        return;

    //round to grid and put in one graph
    _GC->SetState( "Simplify" );

    // Simplify all graphs in the list
    Simplify( ( double ) _GC->GetGrid() );

    if ( ! _GC->GetOrientationEntryMode() )
    {
        TDLI<kbGraph> _LI = TDLI<kbGraph>( this );
        _LI.tohead();
        while ( !_LI.hitroot() )
        {
            _LI.item()->MakeClockWise();
            _LI++;
        }
    }

    Renumber();

    //the graplist contents will be transferred to one graph
    MakeOneGraph( total );
}

// the function will make from all the graphs in the graphlist one graph,
// simply by throwing all the links in one graph, the graphnumbers will
// not be changed
void kbGraphList::MakeOneGraph( kbGraph* total )
{
    TDLI<kbGraph> _LI = TDLI<kbGraph>( this );
    _LI.tohead();
    while( !_LI.hitroot() )
    {
        total->TakeOver( _LI.item() );
        delete _LI.item();
        _LI.remove();
    }
}

//
// Renumber all the graphs
//
void kbGraphList::Renumber()
{
    if ( _GC->GetOrientationEntryMode() )
    {
        TDLI<kbGraph> _LI = TDLI<kbGraph>( this );
        _LI.tohead();
        while ( !_LI.hitroot() )
        {
            if ( _LI.item()->GetFirstLink()->Group() == GROUP_A )
                _LI.item()->SetNumber( 1 );
            else
                _LI.item()->SetNumber( 2 );
            _LI++;
        }
    }
    else
    {
        unsigned int Number = 1;
        TDLI<kbGraph> _LI = TDLI<kbGraph>( this );
        _LI.tohead();
        while ( !_LI.hitroot() )
        {
            _LI.item()->SetNumber( Number++ );
            _LI++;
        }
    }
}


// Simplify the graphs
void kbGraphList::Simplify( double marge )
{
    TDLI<kbGraph> _LI = TDLI<kbGraph>( this );
    _LI.foreach_mf( &kbGraph::Reset_Mark_and_Bin );

    _LI.tohead();
    while ( !_LI.hitroot() )
    {
        if ( _LI.item()->Simplify( ( B_INT ) marge ) )
        {
            if ( _LI.item()->GetNumberOfLinks() < 3 )
                // delete this graph from the graphlist
            {
                delete _LI.item();
                _LI.remove();
            }
        }
        else
            _LI++;
    }
}

// Smoothen the graphs
void kbGraphList::Smoothen( double marge )
{
    TDLI<kbGraph> _LI = TDLI<kbGraph>( this );
    _LI.foreach_mf( &kbGraph::Reset_Mark_and_Bin );

    _LI.tohead();
    while ( !_LI.hitroot() )
    {
        if ( _LI.item()->Smoothen( ( B_INT ) marge ) )
        {
            if ( _LI.item()->GetNumberOfLinks() < 3 )
                // delete this graph from the graphlist
            {
                delete _LI.item();
                _LI.remove();
            }
        }
        else
            _LI++;
    }
}


// Turn off all markers in all the graphs
void kbGraphList::UnMarkAll()
{
    TDLI<kbGraph> _LI = TDLI<kbGraph>( this );
    _LI.foreach_mf( &kbGraph::Reset_Mark_and_Bin );
}

//==============================================================================
//
//======================== BOOLEAN FUNCTIONS ===================================
//
//==============================================================================

void kbGraphList::Correction()
{
    TDLI<kbGraph> _LI = TDLI<kbGraph>( this );
    int todo = _LI.count();

    if ( _GC->GetInternalCorrectionFactor() ) //not zero
    {
        _LI.tohead();
        for( int i = 0; i < todo ; i++ )
        {
            //the input graph will be empty in the end
            kbGraphList *_correct = new kbGraphList( _GC );
            {
                _LI.item()->MakeClockWise();
                _LI.item()->Correction( _correct, _GC->GetInternalCorrectionFactor() );

                delete _LI.item();
                _LI.remove();

                //move corrected graphlist to result
                while ( !_correct->empty() )
                {
                    //add to end
                    _LI.insend( ( kbGraph* )_correct->headitem() );
                    _correct->removehead();
                }
            }
            delete _correct;
        }
    }
}

void kbGraphList::MakeRings()
{
    TDLI<kbGraph> _LI = TDLI<kbGraph>( this );
    int todo = _LI.count();

    _LI.tohead();
    for( int i = 0; i < todo ; i++ )
    {
        //the input graph will be empty in the end
        kbGraphList *_ring = new kbGraphList( _GC );
        {
            _LI.item()->MakeClockWise();
            _LI.item()->MakeRing( _ring, _GC->GetInternalCorrectionFactor() );

            delete _LI.item();
            _LI.remove();

            //move created rings graphs to this
            while ( !_ring->empty() )
            {
                //add to end
                ( ( kbGraph* )_ring->headitem() )->MakeClockWise();
                _LI.insend( ( kbGraph* )_ring->headitem() );
                _ring->removehead();
            }
        }
        delete _ring;
    }

}

//merge the graphs in the list and return the merged result
void kbGraphList::Merge()
{
    if ( count() <= 1 )
        return;

    {
        TDLI<kbGraph> _LI = TDLI<kbGraph>( this );
        _LI.tohead();
        while ( !_LI.hitroot() )
        {
            _LI.item()->SetGroup( GROUP_A );
            _LI++;
        }
    }

    kbGraph* _tomerge = new kbGraph( _GC );

    Renumber();

    //the graplist contents will be transferred to one graph
    MakeOneGraph( _tomerge );
    //the original is empty now

    _tomerge->Prepare( 1 );
    _tomerge->Boolean( BOOL_OR, this );

    delete _tomerge;
}

#define TRIALS 30
#define SAVEME 1

//perform boolean operation on the graphs in the list
void kbGraphList::Boolean( BOOL_OP operation, int intersectionRunsMax )
{
    _GC->SetState( "Performing Boolean Operation" );

    if ( count() == 0 )
        return;

    kbGraph* _prepared = new kbGraph( _GC );

    if ( empty() )
        return;

    //round to grid and put in one graph
    _GC->SetState( "Simplify" );

    int intersectionruns = 1;

    while ( intersectionruns <= intersectionRunsMax )
    {
        try
        {
            Prepare( _prepared );

            if ( _prepared->GetNumberOfLinks() )
            {
                //calculate intersections etc.
                _GC->SetState( "prepare" );
                _prepared->Prepare( intersectionruns );
                //_prepared->writegraph(true);
                _prepared->Boolean( operation, this );
            }
            intersectionruns = intersectionRunsMax + 1;
        }
        catch ( Bool_Engine_Error & error )
        {
#if KBOOL_DEBUG
            _prepared->WriteGraphKEY( _GC );
#endif
            intersectionruns++;
            if ( intersectionruns == intersectionRunsMax )
            {
                _prepared->WriteGraphKEY( _GC );
                _GC->info( error.GetErrorMessage(), "error" );
                throw error;
            }
        }
        catch( ... )
        {

#if KBOOL_DEBUG
            _prepared->WriteGraphKEY( _GC );
#endif
            intersectionruns++;
            if ( intersectionruns == intersectionRunsMax )
            {
                _prepared->WriteGraphKEY( _GC );
                _GC->info( "Unknown exception", "error" );
                throw;
            }
        }
    }

    delete _prepared;
}


void kbGraphList::WriteGraphs()
{
    TDLI<kbGraph> _LI = TDLI<kbGraph>( this );
    _LI.tohead();
    while( !_LI.hitroot() )
    {
        _LI.item()->writegraph( false );
        _LI++;
    }
}

void kbGraphList::WriteGraphsKEY( Bool_Engine* GC )
{
    FILE * file = fopen( "graphkeyfile.key", "w" );

    fprintf( file, "\
             HEADER 5; \
             BGNLIB; \
             LASTMOD {2-11-15  15:39:21}; \
             LASTACC {2-11-15  15:39:21}; \
             LIBNAME trial; \
             UNITS; \
             USERUNITS 0.0001; PHYSUNITS 1e-009; \
             \
             BGNSTR;  \
             CREATION {2-11-15  15:39:21}; \
             LASTMOD  {2-11-15  15:39:21}; \
             STRNAME top; \
             ");

    TDLI<kbGraph> _LI=TDLI<kbGraph>(this);
    _LI.tohead();
    while(!_LI.hitroot())
    {
        _LI.item()->WriteKEY( GC, file );
        _LI++;
    }

    fprintf(file,"\
            ENDSTR top; \
            ENDLIB; \
            ");

    fclose (file);
}
