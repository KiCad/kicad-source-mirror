
#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"

#include "zones.h"

#include "dialog_non_copper_zones_properties.h"

/* Local functions */

/* Local variables */

/* Class DialogNonCopperZonesEditor
 * Dialog editor for non copper zones properties
 * Derived from DialogNonCopperZonesProperties, created by wxFormBuilder
 */
class DialogNonCopperZonesEditor : public DialogNonCopperZonesProperties
{
private:
    WinEDA_PcbFrame* m_Parent;
    ZONE_CONTAINER*  m_Zone_Container;

private:
    void OnOkClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );
    void InitDialog( wxInitDialogEvent& event );

public:
    DialogNonCopperZonesEditor( WinEDA_PcbFrame* parent,
                                ZONE_CONTAINER*  zone_container );
    ~DialogNonCopperZonesEditor();
};


/*******************************************************************************************/
DialogNonCopperZonesEditor::DialogNonCopperZonesEditor( WinEDA_PcbFrame* parent,
                                                        ZONE_CONTAINER*  zone_container ) :
    DialogNonCopperZonesProperties( parent )
/*******************************************************************************************/
{
    m_Parent = parent;
    m_Zone_Container = zone_container;
    SetFont( *g_DialogFont );
}


/********************************************************/
DialogNonCopperZonesEditor::~DialogNonCopperZonesEditor()
/********************************************************/
{
}

/* install function for DialogNonCopperZonesEditor dialog frame :*/
bool InstallDialogNonCopperZonesEditor(WinEDA_PcbFrame* aParent, ZONE_CONTAINER* aZone)
{
    DialogNonCopperZonesEditor* frame = new DialogNonCopperZonesEditor( aParent, aZone );
    bool diag = frame->ShowModal();
    frame->Destroy();

    return diag;
}


/********************************************************************/
void DialogNonCopperZonesEditor::InitDialog( wxInitDialogEvent& event )
/********************************************************************/
{
    SetFocus();
    SetReturnCode( ZONE_ABORT );  // Will be changed on buttons click

    if( g_Zone_45_Only )
        m_OrientEdgesOpt->SetSelection( 1 );

    switch( g_Zone_Default_Setting.m_Zone_HatchingStyle )
    {
    case CPolyLine::NO_HATCH:
        m_OutlineAppearanceCtrl->SetSelection( 0 );
        break;

    case CPolyLine::DIAGONAL_EDGE:
        m_OutlineAppearanceCtrl->SetSelection( 1 );
        break;

    case CPolyLine::DIAGONAL_FULL:
        m_OutlineAppearanceCtrl->SetSelection( 2 );
        break;
    }

    for( int layer_number = FIRST_NO_COPPER_LAYER, ii = 0;
         layer_number <= LAST_NO_COPPER_LAYER;
         layer_number++, ii++ )
    {
        wxString msg;

        msg = m_Parent->m_Pcb->GetLayerName( layer_number ).Trim();
        m_LayerSelectionCtrl->InsertItems( 1, &msg, ii );

        if( m_Zone_Container )
        {
            if( m_Zone_Container->GetLayer() == layer_number )
                m_LayerSelectionCtrl->SetSelection( ii );
        }
        else
        {
            if( ( (PCB_SCREEN*) ( m_Parent->GetScreen() ) )->m_Active_Layer == layer_number )
                m_LayerSelectionCtrl->SetSelection( ii );
        }
    }

    /* the size of m_LayerSelectionCtrl has changed, so we must recall SetSizeHints() */
    GetSizer()->SetSizeHints(this);
}


/******************************************************************/
void DialogNonCopperZonesEditor::OnOkClick( wxCommandEvent& event )
/******************************************************************/
{
    switch( m_OutlineAppearanceCtrl->GetSelection() )
    {
    case 0:
        g_Zone_Default_Setting.m_Zone_HatchingStyle = CPolyLine::NO_HATCH;
        break;

    case 1:
        g_Zone_Default_Setting.m_Zone_HatchingStyle = CPolyLine::DIAGONAL_EDGE;
        break;

    case 2:
        g_Zone_Default_Setting.m_Zone_HatchingStyle = CPolyLine::DIAGONAL_FULL;
        break;
    }

    if( m_Parent->m_Parent->m_EDA_Config )
    {
        m_Parent->m_Parent->m_EDA_Config->Write( ZONE_NET_OUTLINES_HATCH_OPTION_KEY,
            (long) g_Zone_Default_Setting.m_Zone_HatchingStyle );
    }

    if( m_OrientEdgesOpt->GetSelection() == 0 )
        g_Zone_45_Only = FALSE;
    else
        g_Zone_45_Only = TRUE;

    /* Get the layer selection for this zone */
    int ii = m_LayerSelectionCtrl->GetSelection();
    if( ii < 0 )
    {
        DisplayError( this, _( "Error : you must choose a layer" ) );
        return;
    }
    g_Zone_Default_Setting.m_CurrentZone_Layer = ii + FIRST_NO_COPPER_LAYER;
    EndModal( ZONE_OK );
}


/**********************************************************************/
void DialogNonCopperZonesEditor::OnCancelClick( wxCommandEvent& event )
/**********************************************************************/
{
    EndModal( ZONE_ABORT );
}



/***********************************************************/
int ZONE_CONTAINER::BuildFilledPolysListData( BOARD * aPcb )
/***********************************************************/
/** function BuildFilledPolysListData
 * Build m_FilledPolysList data from real outlines (m_Poly)
 * in order to have drawable (and plottable) filled polygons
 * drawable filled polygons are polygons without hole
 * @param aPcb: the current board (can be NULL for non copper zones)
 * @return number of polygons
 * This function does not add holes for pads and tracks but calls
 * AddClearanceAreasPolygonsToPolysList() to do that for copper layers
 */
{

    // Currently, for copper zones,  we can use segment filling or filling by polygon areas
    // if m_GridFillValue == 0 polygon areas will be used (No Grid)
    if ( IsOnCopperLayer() && ( m_GridFillValue != 0 ) )
        return 0;

    m_FilledPolysList.clear();
    /* convert outlines + holes to outlines without holes (adding extra segments if necessary)
    * m_Poly data is expected normalized, i.e. NormalizeAreaOutlines was used after building this zone
    */
    m_Poly->MakeKboolPoly( -1, -1, NULL, true );
    int count = 0;
    while( m_Poly->GetKboolEngine()->StartPolygonGet() )
    {
        CPolyPt corner(0,0,false);
        while( m_Poly->GetKboolEngine()->PolygonHasMorePoints() )
        {
            corner.x = (int)m_Poly->GetKboolEngine()->GetPolygonXPoint();
            corner.y = (int)m_Poly->GetKboolEngine()->GetPolygonYPoint();
            corner.end_contour = false;
            m_FilledPolysList.push_back(corner);
            count ++;
        }
        corner.end_contour = true;
        m_FilledPolysList.pop_back();
        m_FilledPolysList.push_back(corner);
        m_Poly->GetKboolEngine()->EndPolygonGet();
    }

    m_Poly->FreeKboolEngine();

    /* For copper layers, we now must add holes in the Polygon list.
    holes are pads and tracks with their clearance area
    */

    if ( IsOnCopperLayer() )
        AddClearanceAreasPolygonsToPolysList( aPcb );

    return count;
}
