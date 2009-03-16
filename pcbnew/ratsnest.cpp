/***********************/
/**** ratsnest.cpp  ****/
/* Ratsnets functions  */
/***********************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "pcbnew.h"
#include "autorout.h"

#include "protos.h"

/* exported variables */
CHEVELU*    g_pt_chevelu;
CHEVELU*    local_liste_chevelu;    // Buffer address for local ratsnest
// (ratnest relative to one footprint while moving it
int         nb_local_chevelu;       // link count (active ratnest count) for the footprint beeing moved

/* local variables */
static int  nb_pads_ref;                    // node count (node = pad with a net code) for the footprint beeing moved
static int  nb_pads_externes;               // Connected pads count ( pads which are
//	in other footprints and connected to a pad of the footprint beeing moved
static bool DisplayRastnestInProgress;      // Enable the display of the ratsnest during the ratsnest computations

/* Note about the ratsnest computation:
 *  Building the general ratsnest:
 *  I used the "lee algoritm".
 *  This is a 2 steps algoritm.
 *  the m_SubRatsnest member of pads handle a "block number" or a "cluster number" or a "subnet number"
 *  initially, m_SubRatsnest = 0 (pad not connected).
 *  Build_Board_Ratsnest( wxDC* DC )  Create this rastnest
 *  for each net:
 *  First:
 *  we create a link (and therefore a logical block) between 2 pad. This is achieved by:
 *  search for a pad without link.
 *  search its nearest pad
 *  link these 2 pads (i.e. create a ratsnest item)
 *  the pads are grouped in a logical block ( a cluster).
 *  until no pad without link found.
 *  Each logical block has a number called block number or "subnet number",
 *  stored in m_SubRatsnest member for each pad of the block.
 *  The first block has its block number = 1, the second is 2 ...
 *  the function to do thas is gen_rats_pad_to_pad()
 *
 *  Secondly:
 *  The first pass created many logical blocks
 *  A block contains 2 or more pads.
 *  we create links between 2 block. This is achieved by:
 *  Test all pads in the first block, and search (for each pad)
 *  a neighboor in other blocks and compute the distance between pads,
 *  We select the pad pair which have the smallest distance.
 *  These 2 pads are linked (i.e. a new ratsnest item is created between thes 2 pads)
 *  and the 2 block are merged.
 *  Therefore the logical block 1 contains the initial block 1 "eats" the pads of the other block
 *  The computation is made until only one block is found.
 *  the function used is gen_rats_block_to_block()
 *
 *
 *  How existing and new tracks are handled:
 *  The complete rastnest (using the pad analysis) is computed.
 *  it is independant of the tracks and handle the "logical connections".
 *  It depends only on the footprints geometry (and the netlist),
 *  and must be computed only after a netlist read or a footprints geometry change.
 *  Each link (ratsnest) can only be INACTIVE (because pads are connected by a track) or ACTIVE (no tracks)
 *
 *  After the complete rastnest is built, or when a track is added or deleted,
 * we run an algorithm derived from the complete rastnest computation.
 * it is much faster because it analyses only the existing rastnest and not all the pads list
 * and determine only if an existing rastnest must be activated
 * (no physical track exists) or not (a physical track exists)
 * if a track is added or deleted only the corresponding net is tested.
 *
 *  the m_SubRatsnest member of pads is set to 0 (no blocks), and alls links are set to INACTIVE (ratsnest not show).
 *  Before running this fast lee algorithm, we create blocks (and their corresponding block number)
 *  by grouping pads connected by tracks.
 *  So, when tracks exists, the fast lee algorithm is started with some blocks already created.
 * because the fast lee algorithm test only the ratsnest and does not search for
 * nearest pads (this search was previously made) the online ratsnest can be done
 * when a track is created without noticeable computing time
 *  First:
 * for all links (in this step, all are inactive):
 * search for a link which have 1 (or 2) pad having the m_SubRatsnest member = 0.
 * if found the link is set to ACTIVE (i.e. the ratsnest will be showed) and the pad is meged with the block
 * or a new block is created ( see tst_rats_pad_to_pad() ).
 * Secondly:
 * blocks are tested:
 * for all links we search if the 2 pads linkeds are in 2 different block.
 * if yes, the link status is set to ACTIVE, and the 2 block are merged
 * until only one block is found
 * ( see tst_rats_block_to_block() )
 *
 *
 */
/******************************************************************************/
void WinEDA_BasePcbFrame::Compile_Ratsnest( wxDC* DC, bool display_status_pcb )
/******************************************************************************/

/*
 *  Create the entire board ratsnesr.
 *  Msut be called AFTER the connectivity computation
 *  Must be called after a board change (changes for
 *  pads, footprints or a read netlist ).
 *
 *  if display_status_pcb == true : Display the computation results
 */
{
    wxString msg;

    DisplayRastnestInProgress = TRUE;

    /* Create the sorted pad list */
    m_Pcb->m_Status_Pcb = 0;        /* we want a full ratnest computation, from the scratch */
    build_liste_pads();

    MsgPanel->EraseMsgBox();

    msg.Printf( wxT( " %d" ), m_Pcb->m_Pads.size() );
    Affiche_1_Parametre( this, 1, wxT( "pads" ), msg, RED );

    msg.Printf( wxT( " %d" ), m_Pcb->m_Equipots.GetCount() );
    Affiche_1_Parametre( this, 8, wxT( "Nets" ), msg, CYAN );
    
    reattribution_reference_piste( display_status_pcb );

    /* Compute the full ratsnest
     *  which can be see like all the possible links or logical connections.
     *  some of thems are active (no track connected) and others are inactive (when track connect pads)
     *  This full ratsnest is not modified by track editing.
     *  It change only when a netlist is read, or footprints are modified
     */
    Build_Board_Ratsnest( DC );

    /* Compute the pad connections due to the existing tracks (physical connections)*/
    test_connexions( DC );

    /* Compute the active ratsnest, i.e. the unconnected links
     *  it is faster than Build_Board_Ratsnest()
     *  because many optimisations and computations are already made
     */
    Tst_Ratsnest( DC, 0 );

    // Redraw the active ratsnest ( if enabled )
    if( g_Show_Ratsnest )
        DrawGeneralRatsnest( DC, 0 );

    if( display_status_pcb )
        m_Pcb->Display_Infos( this );
}


/*****************************************************************/
static int tri_par_net( const void* o1, const void* o2 )
/****************************************************************/

/* Sort function used by  QSORT
 *  Sort pads by net code
 */
{
    LISTE_PAD* pt_ref     = (LISTE_PAD*) o1;
    LISTE_PAD* pt_compare = (LISTE_PAD*) o2;

    return (*pt_ref)->GetNet() - (*pt_compare)->GetNet();
}


/********************************************************/
static int sort_by_length( const void* o1, const void* o2 )
/********************************************************/

/* Sort function used by  QSORT
 *  Sort ratsnest by lenght
 */
{
    CHEVELU* ref     = (CHEVELU*) o1;
    CHEVELU* compare = (CHEVELU*) o2;

    return ref->dist - compare->dist;
}


/*****************************************************************************/
static int gen_rats_block_to_block( WinEDA_DrawPanel* DrawPanel, wxDC* DC,
                                    LISTE_PAD* pt_liste_pad, LISTE_PAD* pt_limite, int* nblinks )
/*****************************************************************************/

/**
 *  Function used by Build_Board_Ratsnest()
 *  This function creates a rastsnet between two blocks ( which fit the same net )
 *  A block is a group of pads already linked (by a previous ratsnest computation, or tracks)
 *  The search is made between the pads in block 1 (the reference block) and other blocks
 *  the block n ( n > 1 ) it connected to block 1 by their 2 nearest pads.
 *  When the block is found, it is merged with the block 1
 *  the D_PAD member m_SubRatsnest handles the block number
 *  @param  pt_liste_pad = starting address (within the pad list) for search
 *  @param  pt_limite	  = ending address (within the pad list) for search
 *      return in global variables:
 *          ratsnest list in buffer
 *          g_pt_chevelu updated to the first free memory location
 *  @return blocks not connected count
 */
{
    int        dist_min, current_dist;
    int        current_num_block = 1;
    LISTE_PAD* pt_liste_pad_tmp;
    LISTE_PAD* pt_liste_pad_aux;
    LISTE_PAD* pt_liste_pad_block1 = NULL;
    LISTE_PAD* pt_start_liste;

    pt_liste_pad_tmp = NULL;

    dist_min = 0x7FFFFFFF;

    pt_start_liste = pt_liste_pad;

    if( DC )
        GRSetDrawMode( DC, GR_XOR );

    /* Search the nearest pad from block 1 */
    for( ; pt_liste_pad < pt_limite; pt_liste_pad++ )
    {
        D_PAD* ref_pad = *pt_liste_pad;

        /* search a pad which is in the block 1 */
        if( ref_pad->GetSubRatsnest() != 1 )
            continue;

        /* pad is found, search its nearest neighbour in other blocks */
        for( pt_liste_pad_aux = pt_start_liste; ; pt_liste_pad_aux++ )
        {
            D_PAD* curr_pad = *pt_liste_pad_aux;

            if( pt_liste_pad_aux >= pt_limite )
                break;

            if( curr_pad->GetSubRatsnest() == 1 )  // not in an other block
                continue;

            /* Compare distance between pads ("Manhattan" distance) */
            current_dist = abs( curr_pad->m_Pos.x - ref_pad->m_Pos.x ) +
                           abs( curr_pad->m_Pos.y - ref_pad->m_Pos.y );

            if( dist_min > current_dist )   // we have found a better pad pair
            {
                // The tested block can be a good candidate for merging
                // we memorise the "best" current values for merging
                current_num_block = curr_pad->GetSubRatsnest();
                dist_min = current_dist;

                pt_liste_pad_tmp    = pt_liste_pad_aux;
                pt_liste_pad_block1 = pt_liste_pad;
            }
        }
    }

    /*  The reference block is labelled block 1.
     *  if current_num_block != 1 we have found an other block, and we must merge it
     *  with the reference block
     *  The link is made by the 2 nearest pads
     */
    if( current_num_block > 1 )
    {
        /* The block n is merged with the bloc 1 :
         *  to do that, we set the m_SubRatsnest member to 1 for all pads in block n
         */
        for( pt_liste_pad = pt_start_liste; pt_liste_pad < pt_limite; pt_liste_pad++ )
        {
            if( (*pt_liste_pad)->GetSubRatsnest() == current_num_block )
                (*pt_liste_pad)->SetSubRatsnest( 1 );
        }

        pt_liste_pad = pt_liste_pad_block1;

        /* Create the new ratsnet */
        (*nblinks)++;
        g_pt_chevelu->SetNet( (*pt_liste_pad)->GetNet() );
        g_pt_chevelu->status    = CH_ACTIF | CH_VISIBLE;
        g_pt_chevelu->dist      = dist_min;
        g_pt_chevelu->pad_start = *pt_liste_pad;
        g_pt_chevelu->pad_end   = *pt_liste_pad_tmp;

        if( DisplayRastnestInProgress && DC )
            GRLine( &DrawPanel->m_ClipBox, DC, g_pt_chevelu->pad_start->m_Pos.x,
                    g_pt_chevelu->pad_start->m_Pos.y,
                    g_pt_chevelu->pad_end->m_Pos.x,
                    g_pt_chevelu->pad_end->m_Pos.y,
                    0, g_DesignSettings.m_RatsnestColor );

        g_pt_chevelu++;
    }
    return current_num_block;
}


/*****************************************************************************/
static int gen_rats_pad_to_pad( WinEDA_DrawPanel* DrawPanel, wxDC* DC,
                                LISTE_PAD* pt_liste_pad,
                                LISTE_PAD* pt_limite, int current_num_block, int* nblinks )
/*****************************************************************************/

/**
 *  Function used by Build_Board_Ratsnest()
 *  this is the first pass of the lee algorithm
 *  This function creates the link (ratsnest) between 2 pads ( fitting the same net )
 *  the function search for a first not connected pad
 *  and search its nearest neighboor
 * Its creates a block if the 2 pads are not connected, or merge the unconnected pad to the existing block.
 * These blocks include 2 pads and the 2 pads are linked by a ratsnest.
 *
 * @param   pt_liste_pad = starting address in the pad buffer
 * @param   pt_limite	  = ending address
 * @param   current_num_block = Last existing block number de pads
 * These block are created by the existing tracks analysis
 *
 *     output:
 *          Ratsnest list
 *          g_pt_chevelu updated to the first free memory address
 *
 * @return:
 *          last block number used
 */
{
    int        dist_min, current_dist;
    LISTE_PAD* pt_liste_pad_tmp;
    LISTE_PAD* pt_liste_pad_aux;
    LISTE_PAD* pt_start_liste;
    D_PAD*     ref_pad, * pad;

    pt_start_liste = pt_liste_pad;

    if( DC )
        GRSetDrawMode( DC, GR_XOR );

    for(  ; pt_liste_pad < pt_limite; pt_liste_pad++ )
    {
        ref_pad = *pt_liste_pad;

        if( ref_pad->GetSubRatsnest() )
            continue; // Pad already connected

        pt_liste_pad_tmp = NULL;
        dist_min = 0x7FFFFFFF;

        for( pt_liste_pad_aux = pt_start_liste; ; pt_liste_pad_aux++ )
        {
            if( pt_liste_pad_aux >= pt_limite )
                break;

            if( pt_liste_pad_aux == pt_liste_pad )
                continue;

            pad = *pt_liste_pad_aux;

            /* Compare distance between pads ("Manhattan" distance) */
            current_dist =  abs( pad->m_Pos.x - ref_pad->m_Pos.x ) +
                            abs( pad->m_Pos.y - ref_pad->m_Pos.y );

            if( dist_min > current_dist )
            {
                dist_min = current_dist;
                pt_liste_pad_tmp = pt_liste_pad_aux;
            }
        }

        if( pt_liste_pad_tmp != NULL )
        {
            pad = *pt_liste_pad_tmp;

            /* Update the block number
             *  if the 2 pads are not already created : a new block is created
             */
            if( (pad->GetSubRatsnest() == 0) && (ref_pad->GetSubRatsnest() == 0) )
            {
                current_num_block++;
                pad->SetSubRatsnest( current_num_block);
                ref_pad->SetSubRatsnest( current_num_block);
            }
            /* If a pad is already connected connected : merge the other pad in the block */
            else
            {
                ref_pad->SetSubRatsnest( pad->GetSubRatsnest());
            }

            (*nblinks)++;
            g_pt_chevelu->SetNet( ref_pad->GetNet() );
            g_pt_chevelu->status    = CH_ACTIF | CH_VISIBLE;
            g_pt_chevelu->dist      = dist_min;
            g_pt_chevelu->pad_start = ref_pad;
            g_pt_chevelu->pad_end   = pad;

            if( DisplayRastnestInProgress && DC )
            {
                GRLine( &DrawPanel->m_ClipBox, DC, g_pt_chevelu->pad_start->m_Pos.x,
                        g_pt_chevelu->pad_start->m_Pos.y,
                        g_pt_chevelu->pad_end->m_Pos.x,
                        g_pt_chevelu->pad_end->m_Pos.y,
                        0, g_DesignSettings.m_RatsnestColor );
            }
            g_pt_chevelu++;
        }
    }

    return current_num_block;
}


/***********************************************************/
void WinEDA_BasePcbFrame::Build_Board_Ratsnest( wxDC* DC )
/***********************************************************/

/**  Function to compute the full ratsnest (using the LEE algorithm )
 *  In the functions tracks are not considered
 *  This is only the "basic" ratsnest depending only on pads.
 *
 *  - Create the sorted pad list (if necessary)
 *          The active pads (i.e included in a net ) are called nodes
 *    This pad list is sorted by net codes
 *
 *  - Compute the ratsnest (LEE algorithm ):
 *      a - Create the ratsnest between a not connected pad and its nearest
 *          neighbour. Blocks of pads are created
 *      b - Create the ratsnest between blocks:
 *          Test the pads of the 1st block and create a link (ratsnest)
 *           with the nearest pad found in an other block.
 *          The other block is merged with the first block.
 *           until only one block is left.
 *
 *   A ratnest can be seen as a logical connection.
 *
 * Update :
 *      nb_nodes = Active pads count for the board
 *      nb_links = link count for the board (logical connection count)
 *           (there are n-1 links for an equipotent which have n active pads) .
 *
 */
{
    LISTE_PAD* pt_liste_pad, * pt_start_liste, * pt_end_liste, * pt_liste_pad_limite;
    D_PAD*     pad;
    int        num_block, nbpads;
    int        ii;
    CHEVELU*   pt_deb_liste_ch;
    int        current_net_code, noconn;

    m_Pcb->m_NbNoconnect = 0;
    m_Pcb->m_NbLinks = 0;

    if( m_Pcb->m_Ratsnest )
        MyFree( m_Pcb->m_Ratsnest );
    m_Pcb->m_Ratsnest = NULL;


    if( m_Pcb->m_Pads.size() == 0 )
        return;

    /* Created pad list and the net_codes if needed */
    if( (m_Pcb->m_Status_Pcb & NET_CODES_OK) == 0 )
        recalcule_pad_net_code();

    for( unsigned i=0;  i<m_Pcb->m_Pads.size();  ++i )
    {
        pad = m_Pcb->m_Pads[i];
        pad->SetSubRatsnest( 0 );
    }

    /* Sort the pad list by nets */
    qsort( &m_Pcb->m_Pads[0], m_Pcb->m_Pads.size(), sizeof(LISTE_PAD), tri_par_net );

    /* Allocate memory for buffer ratsnest: there are nb_nodes - 1 ratsnest
     *  maximum ( 1 node = 1 active pad ).
     * Memory is allocated for nb_nodes ratsnests... (+ a bit more, just in case)
     *  The real ratsnests count nb_links < nb_nodes
     */
    if( m_Pcb->m_NbNodes == 0 )
        return; /* pas de connexions utiles */

    m_Pcb->m_Ratsnest = (CHEVELU*) MyZMalloc( (m_Pcb->m_NbNodes + 10 ) * sizeof(CHEVELU) );
    if( m_Pcb->m_Ratsnest == NULL )
        return;

    /* Ratsnest computation */
    DisplayRastnestInProgress = TRUE;
    g_pt_chevelu = m_Pcb->m_Ratsnest;

    pt_liste_pad = pt_start_liste = &m_Pcb->m_Pads[0];
    pt_liste_pad_limite = pt_start_liste + m_Pcb->m_Pads.size();

    current_net_code    = 1; // 1er net_code a analyser (net_code = 0 -> no connect)


    EQUIPOT*   equipot = m_Pcb->m_Equipots;
    noconn  = 0;

    for( ; pt_liste_pad < pt_liste_pad_limite; )
    {
        pt_deb_liste_ch = g_pt_chevelu;
        pad = *pt_liste_pad;

        /* Skip the not connected pads */
        if( pad->GetNet() == 0 )
        {
            pt_liste_pad++;
            pt_start_liste = pt_liste_pad;
            continue;
        }

        /* Search the end of pad list des pads for the current net */
        num_block = pad->GetSubRatsnest();
        nbpads    = 0;
        for( pt_end_liste = pt_liste_pad + 1; ; pt_end_liste++ )
        {
            if( pt_end_liste >= pt_liste_pad_limite )
                break;

            pad = *pt_end_liste;
            if( pad->GetNet() != current_net_code )
                break;

            nbpads++;
            if( num_block < pad->GetSubRatsnest() )
                num_block = pad->GetSubRatsnest();
        }

        m_Pcb->m_NbLinks += nbpads;

        /* End of list found: Compute the ratsnest relative to the current net "net_code" */
        equipot = m_Pcb->FindNet( current_net_code );
        if( equipot == NULL )
            DisplayError( this, wxT( "Gen ratsnest err: NULL equipot" ) );
        else
        {
            equipot->m_NbNodes       = nbpads;
            equipot->m_NbLink        = nbpads + 1;
            equipot->m_PadzoneStart  = pt_start_liste;
            equipot->m_PadzoneEnd    = pt_end_liste;
            equipot->m_RatsnestStart = g_pt_chevelu;
        }

        /* a - first pass : create the blocks from "not in block" pads */
        ii = gen_rats_pad_to_pad( DrawPanel, DC, pt_start_liste,
                                  pt_end_liste, num_block, &noconn );

        /* b - blocks connection (Iteration) */
        while( ii > 1 )
        {
            ii = gen_rats_block_to_block( DrawPanel, DC, pt_liste_pad,
                                          pt_end_liste, &noconn );
        }

        if( equipot )
        {
            equipot->m_RatsnestEnd = g_pt_chevelu;

            /* sort by lenght */
            qsort( equipot->m_RatsnestStart,
                   equipot->m_RatsnestEnd - equipot->m_RatsnestStart,
                   sizeof(CHEVELU),
                   sort_by_length );
        }
        pt_liste_pad    = pt_start_liste = pt_end_liste;
        pt_deb_liste_ch = g_pt_chevelu;
        if( pt_start_liste < pt_liste_pad_limite )
            current_net_code = (*pt_start_liste)->GetNet();
    }

    m_Pcb->m_NbNoconnect = noconn;
    m_Pcb->m_Status_Pcb |= LISTE_CHEVELU_OK;
    adr_lowmem = buf_work;

    // erase the ratsnest displayed on screen if needed
    CHEVELU* Chevelu = (CHEVELU*) m_Pcb->m_Ratsnest;
    if ( DC )
        GRSetDrawMode( DC, GR_XOR );

    for( ii = m_Pcb->GetNumRatsnests(); ii > 0; ii--, Chevelu++ )
    {
        if( !g_Show_Ratsnest )
            Chevelu->status &= ~CH_VISIBLE;

        if( DC )
            GRLine( &DrawPanel->m_ClipBox, DC,
                    Chevelu->pad_start->m_Pos.x, Chevelu->pad_start->m_Pos.y,
                    Chevelu->pad_end->m_Pos.x, Chevelu->pad_end->m_Pos.y,
                    0, g_DesignSettings.m_RatsnestColor );
    }
}


/**********************************************************************/
void WinEDA_BasePcbFrame::ReCompile_Ratsnest_After_Changes( wxDC* DC )
/**********************************************************************/

/* recompile rastnest after a module move, delete, ..
 */
{
    if( g_Show_Ratsnest && DC )
        Compile_Ratsnest( DC, TRUE );
}


/*********************************************************************/
void WinEDA_BasePcbFrame::DrawGeneralRatsnest( wxDC* DC, int net_code )
/*********************************************************************/

/**
 *  Displays the general ratsnest
 *  Only ratsnets with the status bit CH_VISIBLE is set are displayed
 *  @param netcode if > 0, Display only the ratsnest relative to the correponding net_code
 */
{
    int      ii;
    CHEVELU* Chevelu;

    if( (m_Pcb->m_Status_Pcb & LISTE_CHEVELU_OK) == 0 )
        return;
    if( (m_Pcb->m_Status_Pcb & DO_NOT_SHOW_GENERAL_RASTNEST) )
        return;
    if( DC == NULL )
        return;

    Chevelu = m_Pcb->m_Ratsnest;
    if( Chevelu == NULL )
        return;

    GRSetDrawMode( DC, GR_XOR );
    for( ii = m_Pcb->GetNumRatsnests(); ii > 0; Chevelu++, ii-- )
    {
        if( ( Chevelu->status & (CH_VISIBLE | CH_ACTIF) ) != (CH_VISIBLE | CH_ACTIF) )
            continue;

        if( (net_code <= 0) || (net_code == Chevelu->GetNet()) )
        {
            GRLine( &DrawPanel->m_ClipBox, DC,
                    Chevelu->pad_start->m_Pos.x, Chevelu->pad_start->m_Pos.y,
                    Chevelu->pad_end->m_Pos.x, Chevelu->pad_end->m_Pos.y,
                    0, g_DesignSettings.m_RatsnestColor );
        }
    }
}


/*****************************************************************************/
static int tst_rats_block_to_block( WinEDA_DrawPanel* DrawPanel, wxDC* DC,
                                    LISTE_PAD* pt_liste_pad_start, LISTE_PAD* pt_liste_pad_end,
                                    CHEVELU* start_rat_list, CHEVELU* end_rat_list )
/*****************************************************************************/

/**
 *  Function used by Tst_Ratsnest()
 *  Function like gen_rats_block_to_block(..)
 *  Function testing the ratsnest between 2 blocks ( same net )
 *  The search is made between pads in block 1 and the others blocks
 *  The block n ( n > 1 ) is merged with block 1 by the smallest ratsnest
 *  Différence between gen_rats_block_to_block(..):
 *  The analysis is not made pads to pads but uses the general ratsnest list.
 *  The function activate the smallest ratsnest between block 1 and the block n
 *  (activate a logical connexion)
 *
 *  @param  pt_liste_pad_start = adresse de debut de zone pad utile
 *          pt_liste_pad_end = adresse de fin de zone pad
 *          start_rat_list = adresse de debut de zone ratsnest utile
 *          end_rat_list = adresse de fin de zone ratsnest
 *      output:
 *          .state member of the ratsnests
 *  @return    blocks not connected count
 */
{
    int        current_num_block, min_block;
    LISTE_PAD* pt_liste_pad;
    CHEVELU*   chevelu, * min_chevelu;

    /* Search a link from a blockto an other block */
    min_chevelu = NULL;
    for( chevelu = start_rat_list; chevelu < end_rat_list; chevelu++ )
    {
        if( chevelu->pad_start->GetSubRatsnest() == chevelu->pad_end->GetSubRatsnest() )  // Same block
            continue;

        if( min_chevelu == NULL )
            min_chevelu = chevelu;
        else if( min_chevelu->dist > chevelu->dist )
            min_chevelu = chevelu;
    }

    if( min_chevelu == NULL )
        return 1;

    /* At this point we have found a link between 2 differents blocks (clusters) :
     * we must set its status to ACTIVE and merge the 2 blocks
     */
    min_chevelu->status |= CH_ACTIF;
    current_num_block    = min_chevelu->pad_start->GetSubRatsnest();
    min_block = min_chevelu->pad_end->GetSubRatsnest();

    if( min_block > current_num_block )
        EXCHG( min_block, current_num_block );

    /* Merging the 2 blocks in one cluster */
    for( pt_liste_pad = pt_liste_pad_start; pt_liste_pad < pt_liste_pad_end; pt_liste_pad++ )
    {
        if( (*pt_liste_pad)->GetSubRatsnest() == current_num_block )
        {
            (*pt_liste_pad)->SetSubRatsnest( min_block);
        }
    }

    return current_num_block;
}


/*********************************************************************/
static int tst_rats_pad_to_pad( WinEDA_DrawPanel* DrawPanel, wxDC* DC,
                                int current_num_block,
                                CHEVELU* start_rat_list, CHEVELU* end_rat_list )
/**********************************************************************/

/**
 *  Function used by Tst_Ratsnest_general()
 *  The general ratsnest list must exists
 *  Activates the ratsnest between 2 pads ( supposes du meme net )
 *  The function links 1 pad not already connected an other pad and activate
 *  some blocks linked by a ratsnest
 *  Its test only the existing ratsnest and activate some ratsnest (status bit CH_ACTIF set)
 *
 * @param   start_rat_list = starting address for the ratnest list
 * @param   end_rat_list   = ending address for the ratnest list
 * @param   current_num_block =  last block number (computed from the track analysis)
 *
 *      output:
 *          ratsnest list (status member set)
 *          and pad list (m_SubRatsnest set)
 *
 * @return new block number
 */
{
    D_PAD*   pad_start, * pad_end;
    CHEVELU* chevelu;

    for( chevelu = start_rat_list; chevelu < end_rat_list; chevelu++ )
    {
        pad_start = chevelu->pad_start;
        pad_end   = chevelu->pad_end;

        /* Update the block if the 2 pads are not connected : a new block is created
         */
        if( (pad_start->GetSubRatsnest() == 0) && (pad_end->GetSubRatsnest() == 0) )
        {
            current_num_block++;
            pad_start->SetSubRatsnest( current_num_block);
            pad_end->SetSubRatsnest( current_num_block);
            chevelu->status |= CH_ACTIF;
        }
        /* If a pad is already connected : the other is merged in the current block */
        else if( pad_start->GetSubRatsnest() == 0 )
        {
            pad_start->SetSubRatsnest( pad_end->GetSubRatsnest() );
            chevelu->status |= CH_ACTIF;
        }
        else if( pad_end->GetSubRatsnest() == 0 )
        {
            pad_end->SetSubRatsnest( pad_start->GetSubRatsnest() );
            chevelu->status |= CH_ACTIF;
        }
    }

    return current_num_block;
}


/******************************************************************/
void WinEDA_BasePcbFrame::Tst_Ratsnest( wxDC* DC, int ref_netcode )
/*******************************************************************/

/* Compute the active ratsnest
 *  The general ratsnest list must exists
 *  Compute the ACTIVE ratsnests in the general ratsnest list
 */
{
    LISTE_PAD* pt_liste_pad;
    CHEVELU*   chevelu;
    D_PAD*     pad;
    int        ii, num_block;
    int        net_code;
    EQUIPOT*   equipot;

    if( m_Pcb->m_Pads.size() == 0 )
        return;

    for( net_code = 1; ; net_code++ )
    {
        equipot = m_Pcb->FindNet( net_code );
        if( equipot == NULL )
            break;

        if( ref_netcode && (net_code != ref_netcode) )
            continue;

        num_block    = 0;
        pt_liste_pad = equipot->m_PadzoneStart;
        for( ; pt_liste_pad < equipot->m_PadzoneEnd; pt_liste_pad++ )
        {
            pad = *pt_liste_pad;
            pad->SetSubRatsnest( pad->GetSubNet() );
            num_block = MAX( num_block, pad->GetSubRatsnest() );
        }

        for( chevelu = equipot->m_RatsnestStart; chevelu < equipot->m_RatsnestEnd; chevelu++ )
        {
            chevelu->status &= ~CH_ACTIF;
        }

        /* a - tst connection between pads */
        ii = tst_rats_pad_to_pad( DrawPanel, DC, num_block,
                                  equipot->m_RatsnestStart, equipot->m_RatsnestEnd );

        /* b - test connexion between blocks (Iteration) */
        while( ii > 1 )
        {
            ii = tst_rats_block_to_block( DrawPanel, DC,
                                          equipot->m_PadzoneStart, equipot->m_PadzoneEnd,
                                          equipot->m_RatsnestStart, equipot->m_RatsnestEnd );
        }
    }

    m_Pcb->m_NbNoconnect = 0;
    CHEVELU* Chevelu = m_Pcb->m_Ratsnest;
    for( ii = m_Pcb->GetNumRatsnests(); ii > 0; ii--, Chevelu++ )
    {
        if( Chevelu->status & CH_ACTIF )
            m_Pcb->m_NbNoconnect++;
    }
}


/**************************************************************************/
int WinEDA_BasePcbFrame::Test_1_Net_Ratsnest( wxDC* DC, int ref_netcode )
/**************************************************************************/

/**
 *  Compute the rastnest relative to the net "net_code"
 *  @param ref_netcode = netcode used to compute the rastnest.
 */
{
    DisplayRastnestInProgress = FALSE;
    DrawGeneralRatsnest( DC, ref_netcode );
    Tst_Ratsnest( DC, ref_netcode );
    DrawGeneralRatsnest( DC, ref_netcode );

    return m_Pcb->GetNumRatsnests();
}


/*****************************************************/
void WinEDA_BasePcbFrame::recalcule_pad_net_code()
/*****************************************************/

/**
 *  Compute and update the net_codes for PADS et and equipots (.m_NetCode member)
 *  net_codes are >= 1 (net_code = 0 means not connected)
 *  Update the equipotents buffer
 *  Must be called after editing pads (netname, or deleting) or after read a netlist
 *  set to 1 flag NET_CODE_OK  of m_Pcb->m_Status_Pcb;
 *  m_Pcb->m_NbNodes and m_Pcb->m_NbNets are updated
 */
{
    LISTE_PAD*      pad_ref, * pad_courant;
    int             ii;
    EQUIPOT**       BufPtEquipot;

    /* Build the PAD list */
    build_liste_pads();

    /* calcul des net_codes des pads */
    ii = m_Pcb->m_Pads.size();

    m_Pcb->m_NbNodes = 0;

    int netcode = 0;

    /* search for differents netnames, and create a netcode for each netname */
    pad_courant = &m_Pcb->m_Pads[0];
    for( ; ii > 0; pad_courant++, ii-- )
    {
        if( (*pad_courant)->GetNetname().IsEmpty() ) // pad not connected
        {
            (*pad_courant)->SetNet( 0 );
            continue;
        }

        m_Pcb->m_NbNodes++;

        /* if the current netname was already found: use the current net_code , else create a new net_code */
        pad_ref = &m_Pcb->m_Pads[0];
        while( pad_ref < pad_courant )
        {
            if( (*pad_ref)->GetNetname() == (*pad_courant)->GetNetname() )
                break; // sont du meme met

            pad_ref++;
        }

        /* if pad_ref != pad_courant we have found 2 pads on the same net., Use the current net_code for pad_courant
         * if pad_ref == pad_courant: new net found (end of list reached) without other pad found on the same net:
         * we must create a new net_code
         */
        if( pad_ref == pad_courant )    // create a new net_code
        {
            netcode++;
            (*pad_courant)->SetNet( netcode );
        }
        else  //  Use the current net_code for pad_courant
            (*pad_courant)->SetNet( (*pad_ref)->GetNet() );
    }

    /* Build or update the equipotent list: we reuse the old list */
    BufPtEquipot = (EQUIPOT**) MyMalloc( sizeof(EQUIPOT*) * (netcode + 1) );

    EQUIPOT* equipot = m_Pcb->m_Equipots;
    for( ii = 0;  ii<= netcode;  equipot = equipot->Next(), ++ii )
    {
        if( equipot == NULL )    /* Create a new equipot if no more equipot in old list */
        {
            equipot = new EQUIPOT( m_Pcb );
            m_Pcb->Add( equipot, ADD_APPEND );
        }

        // Set the net_code for this equipot and reset other values
        equipot->SetNet(ii);
        equipot->m_NbNodes = 0;
        equipot->SetNetname(wxEmptyString);

        BufPtEquipot[ii] = equipot;
    }

    /* Delete the unused equipots in the old list */
    while( equipot )
    {
        EQUIPOT* next =equipot->Next();
        equipot->DeleteStructure();
        equipot = next;
    }

    pad_courant = &m_Pcb->m_Pads[0];
    equipot  = m_Pcb->m_Equipots;

    /* Set the equpot net name and node count for each equipot in equipot list */
    for( ii = m_Pcb->m_Pads.size(); ii > 0; pad_courant++, ii-- )
    {
        int net = (*pad_courant)->GetNet();

        equipot = BufPtEquipot[net];
        equipot->m_NbNodes++;

        if( equipot->GetNetname().IsEmpty() )
        {
            equipot->SetNetname((*pad_courant)->GetNetname());
        }
    }

    MyFree( BufPtEquipot );
    m_Pcb->m_Status_Pcb |= NET_CODES_OK;

    m_Pcb->SetAreasNetCodesFromNetNames();
}


/***********************************************/
void WinEDA_BasePcbFrame::build_liste_pads()
/***********************************************/

/*
 *  Create the pad list
 * initialise:
 *   m_Pcb->m_Pads (list of pads)
 *   m_Pcb->m_NbNodes = node count
 * set m_Pcb->m_Status_Pcb = LISTE_PAD_OK;
 * and clear for all pad their m_SubRatsnest member;
 * delete ( free memory) m_Pcb->m_Ratsnest and set m_Pcb->m_Ratsnest to NULL
 */
{
    if( m_Pcb->m_Status_Pcb & LISTE_PAD_OK )
        return;

    // empty the old list
    m_Pcb->m_Pads.clear();

    m_Pcb->m_NbNodes = 0;

    /* Clear variables used in rastnest computation */
    for( MODULE* module = m_Pcb->m_Modules;  module;  module = module->Next() )
    {
        for( D_PAD* pad = module->m_Pads;  pad;  pad = pad->Next() )
        {
            m_Pcb->m_Pads.push_back( pad );

            pad->SetSubRatsnest( 0 );
            pad->SetParent( module );

            if( pad->GetNet() )
                m_Pcb->m_NbNodes++;
        }
    }

    adr_lowmem = buf_work;

    if( m_Pcb->m_Ratsnest )
    {
        MyFree( m_Pcb->m_Ratsnest );
        m_Pcb->m_Ratsnest = NULL;
    }

    m_Pcb->m_Status_Pcb = LISTE_PAD_OK;
}


/*****************************************************************************/
char* WinEDA_BasePcbFrame::build_ratsnest_module( wxDC* DC, MODULE* Module )
/*****************************************************************************/

/**
 *  Build a rastenest relative to one footprint. This is a simplified computation
 *  used only in move footprint. It is not optimal, but it is fast and sufficient
 *  to guide a footprint placement
 * It shows the connections from a pad to the nearest conected pad
 *  @param Module = module to consider.
 *
 *  the general buffer adr_lowmem is used to store the local footprint ratnest (to do: better to allocate memory)
 *  The ratsnest has 2 sections:
 *      - An "internal" ratsnet relative to pads of this footprint which are in the same net.
 *          this ratsnest section is computed once.
 *      - An "external" rastnest connecting a pad of this footprint to an other pad (in an other footprint)
 *          The ratsnest section must be computed for each new position
 */
{
    LISTE_PAD*      pt_liste_pad;
    LISTE_PAD*      pt_liste_ref;
    LISTE_PAD*      pt_liste_generale;
    D_PAD*          pad_ref;
    D_PAD*          pad_externe;
    LISTE_PAD*      pt_liste_pad_limite;
    LISTE_PAD*      pt_start_liste;
    LISTE_PAD*      pt_end_liste;
    int             ii, jj;
    CHEVELU*        local_chevelu;
    static CHEVELU* pt_fin_int_chevelu;     // End list for "internal" ratsnest
    static int      nb_int_chevelu;         // "internal" ratsnest count
    int             current_net_code;
    int             increment, distance;    // variables de calcul de ratsnest
    int             pad_pos_X, pad_pos_Y;   // True pad position according to the current footprint position


    if( (m_Pcb->m_Status_Pcb & LISTE_PAD_OK) == 0 )
        build_liste_pads();

    /* Compute the "local" ratsnest if needed (when this footprint starts move)
    and the list of external pads to consider, i.e pads in others footprints which are "connected" to
    a pad in the current footprint
    */
    if( (m_Pcb->m_Status_Pcb & CHEVELU_LOCAL_OK) != 0 )
        goto calcul_chevelu_ext;

    /* Compute the "internal" ratsnest, i.e the links beteween the curent footprint pads */
    pt_liste_pad = (LISTE_PAD*) adr_lowmem;
    nb_pads_ref  = 0;

    pad_ref = Module->m_Pads;
    for( ; pad_ref != NULL; pad_ref = pad_ref->Next() )
    {
        if( pad_ref->GetNet() == 0 )
            continue;

        *pt_liste_pad = pad_ref;
        pad_ref->SetSubRatsnest( 0 );
        pad_ref->SetSubNet( 0 );
        pt_liste_pad++; nb_pads_ref++;
    }

    if( nb_pads_ref == 0 )
        return (char*) pt_liste_pad; /* pas de connexions! */

    qsort( adr_lowmem, nb_pads_ref, sizeof(D_PAD*), tri_par_net );

    /* Build the list of pads linked to the current ffotprint pads */
    DisplayRastnestInProgress = FALSE;
    pt_liste_ref = (LISTE_PAD*) adr_lowmem;

    nb_pads_externes = 0;
    current_net_code = 0;
    for( ii = 0; ii < nb_pads_ref; ii++ )
    {
        pad_ref = pt_liste_ref[ii];
        if( pad_ref->GetNet() == current_net_code )
            continue;

        current_net_code = pad_ref->GetNet();

        pt_liste_generale = &m_Pcb->m_Pads[0];
        for( jj = m_Pcb->m_Pads.size(); jj > 0; jj-- )
        {
            pad_externe = *pt_liste_generale; pt_liste_generale++;
            if( pad_externe->GetNet() != current_net_code )
                continue;

            if( pad_externe->GetParent() == Module )
                continue;

            pad_externe->SetSubRatsnest(0);
            pad_externe->SetSubNet( 0 );

            *pt_liste_pad = pad_externe;
            pt_liste_pad++;

            nb_pads_externes++;
        }
    }

    /* Sort the pad list by net_code */
    qsort( pt_liste_ref + nb_pads_ref, nb_pads_externes, sizeof(D_PAD*),
           tri_par_net );

    /* Compute the internal rats nest:
     *  this is the same as general ratsnest, but considers only the current footprint pads
     * it is therefore not time consuming, and it is made only once
    */
    local_liste_chevelu = (CHEVELU*) pt_liste_pad; // buffer chevelu a la suite de la liste des pads
    nb_local_chevelu    = 0;
    pt_liste_ref = (LISTE_PAD*) adr_lowmem;

    g_pt_chevelu = local_liste_chevelu;
    pt_liste_pad = pt_start_liste = (LISTE_PAD*) adr_lowmem;

    pt_liste_pad_limite = pt_liste_pad + nb_pads_ref;

    current_net_code    = (*pt_liste_pad)->GetNet();

    for( ; pt_liste_pad < pt_liste_pad_limite; )
    {
        /* Search the end of pad list relative to the current net */

        for( pt_end_liste = pt_liste_pad + 1; ; pt_end_liste++ )
        {
            if( pt_end_liste >= pt_liste_pad_limite )
                break;

            if( (*pt_end_liste)->GetNet() != current_net_code )
                break;
        }

        /* End of list found: */
        /* a - first step of lee algorithm : build the pad to pad link list */
        ii = gen_rats_pad_to_pad( DrawPanel, DC, pt_start_liste, pt_end_liste,
                                  0, &nb_local_chevelu );

        /* b - secon step of lee algorithm : build the block to block link list (Iteration) */
        while( ii > 1 )
        {
            ii = gen_rats_block_to_block( DrawPanel, DC, pt_liste_pad,
                                          pt_end_liste, &nb_local_chevelu );
        }

        pt_liste_pad = pt_start_liste = pt_end_liste;
        if( pt_start_liste < pt_liste_pad_limite )
            current_net_code = (*pt_start_liste)->GetNet();
    }

    pt_fin_int_chevelu = local_chevelu = g_pt_chevelu;
    nb_int_chevelu = nb_local_chevelu;

    /* set the ratsnets status, flag LOCAL_CHEVELU */
    g_pt_chevelu = local_liste_chevelu;
    while( g_pt_chevelu < pt_fin_int_chevelu )
    {
        g_pt_chevelu->status = LOCAL_CHEVELU; g_pt_chevelu++;
    }

    m_Pcb->m_Status_Pcb |= CHEVELU_LOCAL_OK;

    /*
    This section computes the "external" ratsnest: must be done when the footprint position changes
    */
calcul_chevelu_ext:

    /* This section search:
     *  for each current module pad the nearest neighbour external pad (of course for the same net code).
     *  For each current footprint cluster of pad (pads having the same net code),
     *  we keep the smaller ratsnest.
     */
    local_chevelu    = pt_fin_int_chevelu;
    nb_local_chevelu = nb_int_chevelu;
    pt_liste_ref = (LISTE_PAD*) adr_lowmem;
    pad_ref = *pt_liste_ref;

    current_net_code      = pad_ref->GetNet();
    local_chevelu->dist   = 0x7FFFFFFF;
    local_chevelu->status = 0;
    increment = 0;
    for( ii = 0; ii < nb_pads_ref; ii++ )
    {
        pad_ref = *(pt_liste_ref + ii);
        if( pad_ref->GetNet() != current_net_code )
        {
            /* if needed a new ratsenest for each new net */
            if( increment )
            {
                nb_local_chevelu++; local_chevelu++;
            }
            increment = 0;
            current_net_code    = pad_ref->GetNet();
            local_chevelu->dist = 0x7FFFFFFF;
        }

        pad_pos_X = pad_ref->m_Pos.x - g_Offset_Module.x;
        pad_pos_Y = pad_ref->m_Pos.y - g_Offset_Module.y;
        pt_liste_generale = pt_liste_ref + nb_pads_ref;

        for( jj = nb_pads_externes; jj > 0; jj-- )
        {
            pad_externe = *pt_liste_generale; pt_liste_generale++;

            /* we search pads having the same net coade */
            if( pad_externe->GetNet() < pad_ref->GetNet() )
                continue;

            if( pad_externe->GetNet() > pad_ref->GetNet() ) // remember pads are sorted by net code
                break;

            distance = abs( pad_externe->m_Pos.x - pad_pos_X ) +
                       abs( pad_externe->m_Pos.y - pad_pos_Y );

            if( distance < local_chevelu->dist )
            {
                local_chevelu->pad_start = pad_ref;
                local_chevelu->pad_end   = pad_externe;
                local_chevelu->SetNet( pad_ref->GetNet() );
                local_chevelu->dist   = distance;
                local_chevelu->status = 0;

                increment = 1;
            }
        }
    }

    if( increment ) // fin de balayage : le ratsnest courant doit etre memorise
    {
        nb_local_chevelu++;
        local_chevelu++;
    }

    /* return the new free memory buffer address, in the general buffer */
    adr_max = MAX( adr_max, (char*) (local_chevelu + 1) );

    return (char*) (local_chevelu + 1);   /* the struct pointed by local_chevelu is used
    in temporary computations, so we skip it
    */
}


/***********************************************************/
void WinEDA_BasePcbFrame::trace_ratsnest_module( wxDC* DC )
/**********************************************************/

/*
 *  Display the rastnest of a moving footprint, computed by build_ratsnest_module()
 */
{
    CHEVELU* local_chevelu;
    int      ii;

    if ( DC == NULL )
        return;
    if( (m_Pcb->m_Status_Pcb & CHEVELU_LOCAL_OK) == 0 )
        return;

    local_chevelu = local_liste_chevelu;
    ii = nb_local_chevelu;

    GRSetDrawMode( DC, GR_XOR );

    while( ii-- > 0 )
    {
        if( local_chevelu->status & LOCAL_CHEVELU )
        {
            GRLine( &DrawPanel->m_ClipBox, DC,
                    local_chevelu->pad_start->m_Pos.x - g_Offset_Module.x,
                    local_chevelu->pad_start->m_Pos.y - g_Offset_Module.y,
                    local_chevelu->pad_end->m_Pos.x - g_Offset_Module.x,
                    local_chevelu->pad_end->m_Pos.y - g_Offset_Module.y,
                    0, YELLOW );
        }
        else
        {
            GRLine( &DrawPanel->m_ClipBox, DC,
                    local_chevelu->pad_start->m_Pos.x - g_Offset_Module.x,
                    local_chevelu->pad_start->m_Pos.y - g_Offset_Module.y,
                    local_chevelu->pad_end->m_Pos.x,
                    local_chevelu->pad_end->m_Pos.y,
                    0, g_DesignSettings.m_RatsnestColor );
        }
        local_chevelu++;
    }
}


/*********************************************************************************************/
/* int * WinEDA_BasePcbFrame::build_ratsnest_pad(D_PAD * pad_ref, const wxPoint & refpos) */
/*********************************************************************************************/

/**
 *  construction de la liste en mode de calcul rapide pour affichage
 *  en temps reel du chevelu d'un pad lors des tracés d'une piste démarrant
 *  sur ce pad.
 *
 *  parametres d'appel:
 *      pad_ref ( si null : mise a 0 du nombre de chevelus )
 *      ox, oy = coord de l'extremite de la piste en trace
 *      init (flag)
 *          = 0 : mise a jour des chevelu
 *          <> 0:	creation de la liste
 *  retourne: adresse memoire disponible
 */

/* Used by build_ratsnest_pad(): sort function by link lenght (manathann distance)*/
static int sort_by_localnetlength( const void* o1, const void* o2 )
{
    int* ref     = (int*) o1;
    int* compare = (int*) o2;

    int* org = (int*) adr_lowmem;	// ref coordinate (todo : change for a betted code: used an external wxPoint variable)
    int  ox  = *org++;
    int  oy  = *org++;
    int  lengthref, lengthcmp;

    lengthref = abs( *ref - ox );
    ref++;

    lengthref += abs( *ref - oy );   // = distance between ref coordinate and pad ref

    lengthcmp = abs( *compare - ox );

    compare++;

    lengthcmp += abs( *compare - oy );   // = distance between ref coordinate and the other pad

    return lengthref - lengthcmp;
}


/****************************************************************************************/
int* WinEDA_BasePcbFrame::build_ratsnest_pad( EDA_BaseStruct* ref,
                                              const wxPoint& refpos, bool init )
/****************************************************************************************/
{
    int        ii;
    int*       pt_coord, * base_data;
    int        current_net_code = 0, conn_number = 0;
    LISTE_PAD* padlist;
    D_PAD*     pad_ref = NULL;

    if( ( (m_Pcb->m_Status_Pcb & LISTE_CHEVELU_OK) == 0 )
       || ( (m_Pcb->m_Status_Pcb & LISTE_PAD_OK) == 0 ) )
    {
        nb_local_chevelu = 0;
        return NULL;
    }


    base_data = pt_coord = (int*) adr_lowmem;
    local_liste_chevelu = (CHEVELU*) pt_coord;
    if( init )
    {
        nb_local_chevelu = 0;
        if( ref == NULL )
            return NULL;

        switch( ref->Type() )
        {
        case TYPE_PAD:
            pad_ref = (D_PAD*) ref;
            current_net_code = pad_ref->GetNet();
            conn_number = pad_ref->GetSubNet();
            break;

        case TYPE_TRACK:
        case TYPE_VIA:
        {
            TRACK* track_ref = (TRACK*) ref;
            current_net_code = track_ref->GetNet();
            conn_number = track_ref->GetSubNet();
            break;
        }

        default:
            ;
        }

        if( current_net_code <= 0 )
            return NULL;

        *pt_coord++ = refpos.x;
        *pt_coord++ = refpos.y;

        if( m_Pcb->m_Ratsnest == NULL )
            return NULL;

        padlist = &m_Pcb->m_Pads[0];
        for( ii = 0; ii < (int) m_Pcb->m_Pads.size(); padlist++, ii++ )
        {
            D_PAD* pad = *padlist;
            if( pad->GetNet() != current_net_code )
                continue;

            if( pad == pad_ref )
                continue;

            if( !pad->GetSubNet() || (pad->GetSubNet() != conn_number) )
            {
                *pt_coord = pad->m_Pos.x; pt_coord++;
                *pt_coord = pad->m_Pos.y; pt_coord++;
                nb_local_chevelu++;
            }
        }
    }   /* end if Init */

    else if( nb_local_chevelu )
    {
        *pt_coord = refpos.x;
        *(pt_coord + 1) = refpos.y;
    }

    qsort( base_data + 2, nb_local_chevelu, 2 * sizeof(int),
           sort_by_localnetlength );
    return pt_coord;
}


/*******************************************************/
void WinEDA_BasePcbFrame::trace_ratsnest_pad( wxDC* DC )
/*******************************************************/

/*
 *  Displays a "ratsnest" during track creation
 */
{
    int* pt_coord;
    int  ii;
    int  refX, refY;

    if ( DC == NULL )
        return;

    if( (m_Pcb->m_Status_Pcb & LISTE_CHEVELU_OK) == 0 )
        return;

    if( nb_local_chevelu == 0 )
        return;

    if( local_liste_chevelu == NULL )
        return;

    pt_coord = (int*) local_liste_chevelu;

    refX = *pt_coord++;
    refY = *pt_coord++;

    GRSetDrawMode( DC, GR_XOR );
    for( ii = 0; ii < nb_local_chevelu; ii++ )
    {
        if( ii >= g_MaxLinksShowed )
            break;

        GRLine( &DrawPanel->m_ClipBox, DC, refX, refY, *pt_coord, *(pt_coord + 1),
                0, YELLOW );
        pt_coord += 2;
    }
}
