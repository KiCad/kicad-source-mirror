/*************/
/* Edit pads */
/*************/

#include "fctsys.h"

#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "pcbnew.h"
#include "dialog_global_pads_edition_base.h"


/************************************/
/* class DIALOG_GLOBAL_PADS_EDITION */
/************************************/

class DIALOG_GLOBAL_PADS_EDITION : public DIALOG_GLOBAL_PADS_EDITION_BASE
{
private:
    WinEDA_BasePcbFrame* m_Parent;
    D_PAD*      m_CurrentPad;

public:
    static bool m_Pad_Shape_Filter;
    static bool m_Pad_Layer_Filter;
    static bool m_Pad_Orient_Filter;

public:
    DIALOG_GLOBAL_PADS_EDITION( WinEDA_BasePcbFrame* parent, D_PAD* Pad );
    ~DIALOG_GLOBAL_PADS_EDITION() { }

private:
    void InstallPadEditor( wxCommandEvent& event );
    void PadPropertiesAccept( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );
};


DIALOG_GLOBAL_PADS_EDITION::DIALOG_GLOBAL_PADS_EDITION(
    WinEDA_BasePcbFrame* parent,
    D_PAD*               Pad ) :
    DIALOG_GLOBAL_PADS_EDITION_BASE( parent )
{
    m_Parent     = parent;
    m_CurrentPad = Pad;

    // Pad filter selection.
    m_Pad_Shape_Filter_CB->SetValue( m_Pad_Shape_Filter );
    m_Pad_Layer_Filter_CB->SetValue( m_Pad_Layer_Filter );
    m_Pad_Orient_Filter_CB->SetValue( m_Pad_Orient_Filter );

    SetFocus();

    GetSizer()->Fit( this );
    Centre();
}


/* Class DIALOG_GLOBAL_PADS_EDITION static variables */
bool DIALOG_GLOBAL_PADS_EDITION::m_Pad_Shape_Filter  = true;
bool DIALOG_GLOBAL_PADS_EDITION::m_Pad_Layer_Filter  = true;
bool DIALOG_GLOBAL_PADS_EDITION::m_Pad_Orient_Filter = true;


void DIALOG_GLOBAL_PADS_EDITION::OnCancelClick( wxCommandEvent& event )
{
    EndModal( -1 );
}


/* Calls the Pad editor.
 */
void DIALOG_GLOBAL_PADS_EDITION::InstallPadEditor( wxCommandEvent& event )
{
    m_Parent->InstallPadOptionsFrame( m_CurrentPad );
}


/* Update the parameters for the component being edited.
 */
void DIALOG_GLOBAL_PADS_EDITION::PadPropertiesAccept( wxCommandEvent& event )
{
    int returncode = 0;

    switch( event.GetId() )
    {
    case ID_CHANGE_ID_MODULES:
        returncode = 1;

    // Fall through

    case ID_CHANGE_CURRENT_MODULE:
        m_Pad_Shape_Filter  = m_Pad_Shape_Filter_CB->GetValue();
        m_Pad_Layer_Filter  = m_Pad_Layer_Filter_CB->GetValue();
        m_Pad_Orient_Filter = m_Pad_Orient_Filter_CB->GetValue();
        EndModal( returncode );
        break;
    }

    m_Parent->OnModify();
}


/**
 * Function Global_Import_Pad_Settings
 * Function to change pad caracteristics for the given footprint
 * or alls footprints which look like the given footprint
 * @param aPad pad to use as pattern. The given footprint is the parent of
 *             this pad
 * @param aDraw: if true: redraws the footprint
 */
void WinEDA_BasePcbFrame::Global_Import_Pad_Settings( D_PAD* aPad, bool aDraw )
{
    MODULE* Module_Ref, * Module;
    int     diag;
    bool    edit_Same_Modules = false;

    if( aPad == NULL )
        aPad = &g_Pad_Master;

    Module = (MODULE*) aPad->GetParent();

    if( Module == NULL )
    {
        DisplayError( this,
                     wxT( "Global_Import_Pad_Settings() Error: NULL module" ) );
        return;
    }

    Module->DisplayInfo( this );

    DIALOG_GLOBAL_PADS_EDITION* dlg = new DIALOG_GLOBAL_PADS_EDITION( this, aPad );

    diag = dlg->ShowModal();
    dlg->Destroy();

    if( diag == -1 )
        return;
    if( diag == 1 )
        edit_Same_Modules = true;

    /* Search and copy the name of library reference. */
    Module_Ref = Module;
    int pad_orient = aPad->m_Orient - Module_Ref->m_Orient;

    // Prepare an undo list:
    PICKED_ITEMS_LIST itemsList;
    Module = (MODULE*) m_Pcb->m_Modules;
    for( ; Module != NULL; Module = Module->Next() )
    {
        if( !edit_Same_Modules && (Module != Module_Ref) )
            continue;

        if( Module->m_LibRef != Module_Ref->m_LibRef )
            continue;

        bool   saveMe = false;
        D_PAD* pt_pad = (D_PAD*) Module->m_Pads;
        for( ; pt_pad != NULL; pt_pad = pt_pad->Next() )
        {
            /* Filters changes prohibited. */
            if( DIALOG_GLOBAL_PADS_EDITION::m_Pad_Shape_Filter
               && ( pt_pad->m_PadShape != aPad->m_PadShape ) )
                continue;

            if( DIALOG_GLOBAL_PADS_EDITION::m_Pad_Orient_Filter
               && ( (pt_pad->m_Orient - Module->m_Orient) != pad_orient ) )
                continue;

            if( DIALOG_GLOBAL_PADS_EDITION::m_Pad_Layer_Filter
               && ( pt_pad->m_Masque_Layer != aPad->m_Masque_Layer ) )
                continue;

            saveMe = true;
        }

        if( saveMe )
        {
            ITEM_PICKER itemWrapper( Module, UR_CHANGED );
            itemWrapper.m_PickedItemType = Module->Type();
            itemsList.PushItem( itemWrapper );
        }
    }

    SaveCopyInUndoList( itemsList, UR_CHANGED );

    /* Update the current module and same others modules if requested. */
    Module = (MODULE*) m_Pcb->m_Modules;
    for( ; Module != NULL; Module = Module->Next() )
    {
        if( !edit_Same_Modules && (Module != Module_Ref) )
            continue;

        if( Module->m_LibRef != Module_Ref->m_LibRef )
            continue;

        /* Erase module on screen */
        if( aDraw )
        {
            Module->m_Flags |= DO_NOT_DRAW;
            DrawPanel->PostDirtyRect( Module->GetBoundingBox() );
            Module->m_Flags &= ~DO_NOT_DRAW;
        }

        D_PAD* pt_pad = (D_PAD*) Module->m_Pads;
        for( ; pt_pad != NULL; pt_pad = pt_pad->Next() )
        {
            /* Filters changes prohibited. */
            if( DIALOG_GLOBAL_PADS_EDITION::m_Pad_Shape_Filter
               && ( pt_pad->m_PadShape != aPad->m_PadShape ) )
                continue;

            if( DIALOG_GLOBAL_PADS_EDITION::m_Pad_Orient_Filter
               && ( (pt_pad->m_Orient - Module->m_Orient) != pad_orient ) )
                continue;

            if( DIALOG_GLOBAL_PADS_EDITION::m_Pad_Layer_Filter )
            {
                if( pt_pad->m_Masque_Layer != aPad->m_Masque_Layer )
                    continue;
                else
                    m_Pcb->m_Status_Pcb &=
                        ~( LISTE_RATSNEST_ITEM_OK | CONNEXION_OK);
            }

            /* Change characteristics.: */
            pt_pad->m_Attribut = aPad->m_Attribut;
            pt_pad->m_PadShape = aPad->m_PadShape;

            pt_pad->m_Masque_Layer = aPad->m_Masque_Layer;

            pt_pad->m_Size = aPad->m_Size;
            pt_pad->m_DeltaSize = aPad->m_DeltaSize;
            pt_pad->m_Offset    = aPad->m_Offset;

            pt_pad->m_Drill = aPad->m_Drill;
            pt_pad->m_DrillShape = aPad->m_DrillShape;

            pt_pad->m_Orient = pad_orient + Module->m_Orient;

            // copy also local mask margins,
            // because these parameters usually depend on
            // pads sizes and layers
            pt_pad->m_LocalSolderMaskMargin  = aPad->m_LocalSolderMaskMargin;
            pt_pad->m_LocalSolderPasteMargin = aPad->m_LocalSolderPasteMargin;
            pt_pad->m_LocalSolderPasteMarginRatio = aPad->m_LocalSolderPasteMarginRatio;


            if( pt_pad->m_PadShape != PAD_TRAPEZOID )
            {
                pt_pad->m_DeltaSize.x = 0;
                pt_pad->m_DeltaSize.y = 0;
            }
            if( pt_pad->m_PadShape == PAD_CIRCLE )
                pt_pad->m_Size.y = pt_pad->m_Size.x;

            switch( pt_pad->m_Attribut & 0x7F )
            {
            case PAD_SMD:
            case PAD_CONN:
                pt_pad->m_Drill    = wxSize( 0, 0 );
                pt_pad->m_Offset.x = 0;
                pt_pad->m_Offset.y = 0;
                break;

            default:
                break;
            }

            pt_pad->ComputeShapeMaxRadius();
        }

        Module->Set_Rectangle_Encadrement();
        if( aDraw )
            DrawPanel->PostDirtyRect( Module->GetBoundingBox() );
    }

    OnModify();
}
