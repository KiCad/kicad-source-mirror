/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2008 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2004-2018 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <collectors.h>
#include <class_board_item.h>             // class BOARD_ITEM

#include <class_module.h>
#include <class_pad.h>
#include <class_track.h>
#include <class_marker_pcb.h>
#include <class_zone.h>


/* This module contains out of line member functions for classes given in
 * collectors.h.  Those classes augment the functionality of class PCB_EDIT_FRAME.
 */


const KICAD_T GENERAL_COLLECTOR::AllBoardItems[] = {
    // there are some restrictions on the order of items in the general case.
    // all items in m_Drawings for instance should be contiguous.
    //  *** all items in a same list (shown here) must be contiguous ****
    PCB_MARKER_T,                // in m_markers
    PCB_TEXT_T,                  // in m_Drawings
    PCB_LINE_T,                  // in m_Drawings
    PCB_DIMENSION_T,             // in m_Drawings
    PCB_TARGET_T,                // in m_Drawings
    PCB_VIA_T,                   // in m_Tracks
    PCB_TRACE_T,                 // in m_Tracks
    PCB_PAD_T,                   // in modules
    PCB_MODULE_TEXT_T,           // in modules
    PCB_MODULE_T,                // in m_Modules
    PCB_ZONE_AREA_T,             // in m_ZoneDescriptorList
    EOT
};


const KICAD_T GENERAL_COLLECTOR::BoardLevelItems[] = {
    PCB_MARKER_T,
    PCB_TEXT_T,
    PCB_LINE_T,
    PCB_DIMENSION_T,
    PCB_TARGET_T,
    PCB_VIA_T,
    PCB_TRACE_T,
    PCB_MODULE_T,
    PCB_ZONE_AREA_T,
    EOT
};


const KICAD_T GENERAL_COLLECTOR::AllButZones[] = {
    PCB_MARKER_T,
    PCB_TEXT_T,
    PCB_LINE_T,
    PCB_DIMENSION_T,
    PCB_TARGET_T,
    PCB_VIA_T,
    PCB_TRACE_T,
    PCB_PAD_T,
    PCB_MODULE_TEXT_T,
    PCB_MODULE_T,
    PCB_ZONE_AREA_T,         // if it is visible on screen, it should be selectable
    EOT
};


const KICAD_T GENERAL_COLLECTOR::Modules[] = {
    PCB_MODULE_T,
    EOT
};


const KICAD_T GENERAL_COLLECTOR::PadsOrModules[] = {
    PCB_PAD_T,
    PCB_MODULE_T,
    EOT
};


const KICAD_T GENERAL_COLLECTOR::PadsOrTracks[] = {
    PCB_PAD_T,
    PCB_VIA_T,
    PCB_TRACE_T,
    EOT
};


const KICAD_T GENERAL_COLLECTOR::ModulesAndTheirItems[] = {
    PCB_MODULE_TEXT_T,
    PCB_MODULE_EDGE_T,
    PCB_PAD_T,
    PCB_MODULE_T,
    EOT
};


const KICAD_T GENERAL_COLLECTOR::ModuleItems[] = {
    PCB_MODULE_TEXT_T,
    PCB_MODULE_EDGE_T,
    PCB_PAD_T,
    EOT
};


const KICAD_T GENERAL_COLLECTOR::Tracks[] = {
    PCB_TRACE_T,
    PCB_VIA_T,
    EOT
};


const KICAD_T GENERAL_COLLECTOR::LockableItems[] = {
    PCB_MODULE_T,
    PCB_TRACE_T,
    PCB_VIA_T,
    EOT
};


const KICAD_T GENERAL_COLLECTOR::Zones[] = {
    PCB_ZONE_AREA_T,
    EOT
};



SEARCH_RESULT GENERAL_COLLECTOR::Inspect( EDA_ITEM* testItem, void* testData )
{
    BOARD_ITEM*     item   = (BOARD_ITEM*) testItem;
    MODULE*         module = NULL;
    D_PAD*          pad    = NULL;
    bool            pad_through = false;
    VIA*            via    = NULL;
    MARKER_PCB*     marker = NULL;
    ZONE_CONTAINER* zone   = NULL;

#if 0   // debugging
    static int  breakhere = 0;

    switch( item->Type() )
    {
    case PCB_PAD_T:
        {
            MODULE* m = (MODULE*) item->GetParent();

            if( m->GetReference() == wxT( "Y2" ) )
            {
                breakhere++;
            }
        }
        break;

    case PCB_VIA_T:
        breakhere++;
        break;

    case PCB_TRACE_T:
        breakhere++;
        break;

    case PCB_TEXT_T:
        breakhere++;
        break;

    case PCB_LINE_T:
        breakhere++;
        break;

    case PCB_DIMENSION_T:
        breakhere++;
        break;

    case PCB_MODULE_TEXT_T:
        {
            TEXTE_MODULE* tm = (TEXTE_MODULE*) item;

            if( tm->GetText() == wxT( "10uH" ) )
            {
                breakhere++;
            }
        }
        break;

    case PCB_MODULE_T:
        {
            MODULE* m = (MODULE*) item;

            if( m->GetReference() == wxT( "C98" ) )
            {
                breakhere++;
            }
        }
        break;

    case PCB_MARKER_T:
        breakhere++;
        break;

    default:
        breakhere++;
        break;
    }

#endif


    switch( item->Type() )
    {
    case PCB_PAD_T:
        // there are pad specific visibility controls.
        // Criterias to select a pad is:
        // for smd pads: the module parent must be seen, and pads on the corresponding
        // board side must be seen
        // if pad is a thru hole, then it can be visible when its parent module is not.
        // for through pads: pads on Front or Back board sides must be seen
        pad = (D_PAD*) item;

        if( (pad->GetAttribute() != PAD_ATTRIB_SMD) &&
            (pad->GetAttribute() != PAD_ATTRIB_CONN) )    // a hole is present, so multiple layers
        {
            // proceed to the common tests below, but without the parent module test,
            // by leaving module==NULL, but having pad != null
            pad_through = true;
        }
        else  // smd, so use pads test after module test
        {
            module = static_cast<MODULE*>( item->GetParent() );
        }

        break;

    case PCB_VIA_T:     // vias are on many layers, so layer test is specific
        via = static_cast<VIA*>( item );
        break;

    case PCB_TRACE_T:
        if( m_Guide->IgnoreTracks() )
            goto exit;
        break;

    case PCB_ZONE_AREA_T:
        zone = static_cast<ZONE_CONTAINER*>( item );
        break;

    case PCB_TEXT_T:
        break;

    case PCB_LINE_T:
        break;

    case PCB_DIMENSION_T:
        break;

    case PCB_TARGET_T:
        break;

    case PCB_MODULE_TEXT_T:
        {
            TEXTE_MODULE *text = static_cast<TEXTE_MODULE*>( item );
            if( m_Guide->IgnoreMTextsMarkedNoShow() && !text->IsVisible() )
                goto exit;

            if( m_Guide->IgnoreMTextsOnBack() && IsBackLayer( text->GetLayer() ) )
                goto exit;

            if( m_Guide->IgnoreMTextsOnFront() && IsFrontLayer( text->GetLayer() ) )
                goto exit;

            /* The three text types have different criteria: reference
             * and value have their own ignore flags; user text instead
             * follows their layer visibility. Checking this here is
             * simpler than later (when layer visibility is checked for
             * other entities) */

            switch( text->GetType() )
            {
            case TEXTE_MODULE::TEXT_is_REFERENCE:
                if( m_Guide->IgnoreModulesRefs() )
                    goto exit;
                break;

            case TEXTE_MODULE::TEXT_is_VALUE:
                if( m_Guide->IgnoreModulesVals() )
                    goto exit;
                break;

            case TEXTE_MODULE::TEXT_is_DIVERS:
                if( !m_Guide->IsLayerVisible( text->GetLayer() )
                        && m_Guide->IgnoreNonVisibleLayers() )
                    goto exit;
                break;
            }

            // Extract the module since it could be hidden
            module = static_cast<MODULE*>( item->GetParent() );
        }
        break;

    case PCB_MODULE_T:
        module = static_cast<MODULE*>( item );
        break;

    case PCB_MARKER_T:
        marker = static_cast<MARKER_PCB*>( item );
        break;

    default:
        break;
    }

    // common tests:

    if( module )    // true from case PCB_PAD_T, PCB_MODULE_TEXT_T, or PCB_MODULE_T
    {
        if( m_Guide->IgnoreModulesOnBack() && (module->GetLayer() == B_Cu) )
            goto exit;

        if( m_Guide->IgnoreModulesOnFront() && (module->GetLayer() == F_Cu) )
            goto exit;
    }

    // Pads are not sensitive to the layer visibility controls.
    // They all have their own separate visibility controls
    // skip them if not visible
    if( pad )
    {
        if( m_Guide->IgnorePads() )
            goto exit;

        if( ! pad_through )
        {
            if( m_Guide->IgnorePadsOnFront() && pad->IsOnLayer(F_Cu ) )
                goto exit;

            if( m_Guide->IgnorePadsOnBack() && pad->IsOnLayer(B_Cu ) )
                goto exit;
        }
    }

    if( marker )
    {
        // Markers are not sensitive to the layer
        if( marker->HitTest( m_RefPos ) )
            Append( item );

        goto exit;
    }

    if( via )
    {
        auto type = via->GetViaType();

        if( ( m_Guide->IgnoreThroughVias() && type == VIA_THROUGH ) ||
            ( m_Guide->IgnoreBlindBuriedVias() && type == VIA_BLIND_BURIED ) ||
            ( m_Guide->IgnoreMicroVias() && type == VIA_MICROVIA ) )
        {
            goto exit;
        }
    }

    if( item->IsOnLayer( m_Guide->GetPreferredLayer() ) || m_Guide->IgnorePreferredLayer() )
    {
        PCB_LAYER_ID layer = item->GetLayer();

        // Modules and their subcomponents: reference, value and pads are not sensitive
        // to the layer visibility controls.  They all have their own separate visibility
        // controls for vias, GetLayer() has no meaning, but IsOnLayer() works fine. User
        // text in module *is* sensitive to layer visibility but that was already handled.

        if( via || module || pad || m_Guide->IsLayerVisible( layer )
                || !m_Guide->IgnoreNonVisibleLayers() )
        {
            if( !m_Guide->IsLayerLocked( layer ) || !m_Guide->IgnoreLockedLayers() )
            {
                if( !item->IsLocked() || !m_Guide->IgnoreLockedItems() )
                {
                    if( zone )
                    {
                        bool testFill = !m_Guide->IgnoreZoneFills();
                        int  accuracy = KiROUND( 5 * m_Guide->OnePixelInIU() );

                        if( zone->HitTestForCorner( m_RefPos, accuracy * 2 )
                            || zone->HitTestForEdge( m_RefPos, accuracy )
                            || ( testFill && zone->HitTestFilledArea( m_RefPos ) ) )
                        {
                            Append( item );
                            goto exit;
                        }
                    }
                    else if( item->HitTest( m_RefPos ) )
                    {
                        if( !module || module->HitTestAccurate( m_RefPos ) )
                        {
                            Append( item );
                            goto exit;
                        }
                    }
                }
            }
        }
    }


    if( m_Guide->IncludeSecondary() )
    {
        // for now, "secondary" means "tolerate any layer".  It has
        // no effect on other criteria, since there is a separate "ignore" control for
        // those in the COLLECTORS_GUIDE

        PCB_LAYER_ID layer = item->GetLayer();

        // Modules and their subcomponents: reference, value and pads are not sensitive
        // to the layer visibility controls.  They all have their own separate visibility
        // controls for vias, GetLayer() has no meaning, but IsOnLayer() works fine. User
        // text in module *is* sensitive to layer visibility but that was already handled.

        if( via || module || pad || m_Guide->IsLayerVisible( layer )
                || !m_Guide->IgnoreNonVisibleLayers() )
        {
            if( !m_Guide->IsLayerLocked( layer ) || !m_Guide->IgnoreLockedLayers() )
            {
                if( !item->IsLocked() || !m_Guide->IgnoreLockedItems() )
                {
                    if( zone )
                    {
                        bool testFill = !m_Guide->IgnoreZoneFills();
                        int  accuracy = KiROUND( 5 * m_Guide->OnePixelInIU() );

                        if( zone->HitTestForCorner( m_RefPos, accuracy * 2 )
                            || zone->HitTestForEdge( m_RefPos, accuracy )
                            || ( testFill && zone->HitTestFilledArea( m_RefPos ) ) )
                        {
                            Append2nd( item );
                            goto exit;
                        }
                    }
                    else if( item->HitTest( m_RefPos ) )
                    {
                        Append2nd( item );
                        goto exit;
                    }
                }
            }
        }
    }

exit:
    return SEARCH_CONTINUE;     // always when collecting
}


void GENERAL_COLLECTOR::Collect( BOARD_ITEM* aItem, const KICAD_T aScanList[],
                                 const wxPoint& aRefPos, const COLLECTORS_GUIDE& aGuide )
{
    Empty();        // empty the collection, primary criteria list
    Empty2nd();     // empty the collection, secondary criteria list

    // remember guide, pass it to Inspect()
    SetGuide( &aGuide );

    SetScanTypes( aScanList );

    // remember where the snapshot was taken from and pass refPos to
    // the Inspect() function.
    SetRefPos( aRefPos );

    aItem->Visit( m_inspector, NULL, m_ScanTypes );

    SetTimeNow();               // when snapshot was taken

    // record the length of the primary list before concatenating on to it.
    m_PrimaryLength = m_List.size();

    // append 2nd list onto end of the first list
    for( unsigned i = 0;  i<m_List2nd.size();  ++i )
        Append( m_List2nd[i] );

    Empty2nd();
}


SEARCH_RESULT PCB_TYPE_COLLECTOR::Inspect( EDA_ITEM* testItem, void* testData )
{
    // The Visit() function only visits the testItem if its type was in the
    // the scanList, so therefore we can collect anything given to us here.
    Append( testItem );

    return SEARCH_CONTINUE;     // always when collecting
}


void PCB_TYPE_COLLECTOR::Collect( BOARD_ITEM* aBoard, const KICAD_T aScanList[] )
{
    Empty();        // empty any existing collection

    aBoard->Visit( m_inspector, NULL, aScanList );
}


SEARCH_RESULT PCB_LAYER_COLLECTOR::Inspect( EDA_ITEM* testItem, void* testData )
{
    BOARD_ITEM* item = (BOARD_ITEM*) testItem;

    if( item->Type() == PCB_PAD_T )     // multilayer
    {
        if( static_cast<D_PAD*>( item )->IsOnLayer( m_layer_id ) )
            Append( testItem );
    }
    else if( item->GetLayer() == m_layer_id )
        Append( testItem );

    return SEARCH_CONTINUE;
}


void PCB_LAYER_COLLECTOR::Collect( BOARD_ITEM* aBoard, const KICAD_T aScanList[] )
{
    Empty();

    aBoard->Visit( m_inspector, NULL, aScanList );
}
