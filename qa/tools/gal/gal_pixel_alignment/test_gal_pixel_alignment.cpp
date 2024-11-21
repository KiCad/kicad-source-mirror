#include <pcb_test_frame.h>
#include <tool/tool_manager.h>

#include <gal/graphics_abstraction_layer.h>
#include <view/view.h>
#include <view/view_item.h>
#include <pcb_draw_panel_gal.h>

#include <layer_ids.h>

const VECTOR2D xform2( const MATRIX3x3D& minv, const VECTOR2D& p )
{
    auto t = minv * p;
    return VECTOR2D((float)t.x, (float)t.y);
}

void screenSpaceLine( KIGFX::GAL* gal, const VECTOR2D& p0, const VECTOR2D& p1, double w )
{
    auto minv = gal->GetScreenWorldMatrix();

    auto pa = xform2 ( minv, p0 );
    auto pb = xform2 ( minv, p1 );

#if 0
    //shader->Deactivate();

    m_currentManager->Reserve( 6 );
    m_currentManager->Shader( SHADER_NONE );
    m_currentManager->Color( m_strokeColor.r, m_strokeColor.g, m_strokeColor.b, m_strokeColor.a );

    m_currentManager->Shader( SHADER_NONE ); m_currentManager->Vertex( pa.x, pa.y, m_layerDepth );
    m_currentManager->Shader( SHADER_NONE ); m_currentManager->Vertex( pb.x, pb.y, m_layerDepth );
    m_currentManager->Shader( SHADER_NONE ); m_currentManager->Vertex( pc.x, pc.y, m_layerDepth );
    m_currentManager->Shader( SHADER_NONE ); m_currentManager->Vertex( pa.x, pa.y, m_layerDepth );
    m_currentManager->Shader( SHADER_NONE ); m_currentManager->Vertex( pc.x, pc.y, m_layerDepth );
    m_currentManager->Shader( SHADER_NONE ); m_currentManager->Vertex( pd.x, pd.y, m_layerDepth );
    shader->Use();
#endif

    gal->SetLineWidth( w * minv.GetScale().x );
    gal->DrawLine( pa, pb );


}

void screenSpaceCircle( KIGFX::GAL* gal, const VECTOR2D& p0, double r, double w )
{
    auto minv = gal->GetScreenWorldMatrix();

    auto pa = xform2 ( minv, p0 );

    gal->SetLineWidth( w * minv.GetScale().x );
    gal->DrawCircle( pa, r * minv.GetScale().x );

}


class MY_DRAWING : public EDA_ITEM
{
public:
    MY_DRAWING() : EDA_ITEM( NOT_USED ) {};
    virtual ~MY_DRAWING(){};

    wxString GetClass() const override
    {
        return wxT( "MyDrawing" );
    }

#ifdef DEBUG
    virtual void Show( int nestLevel, std::ostream& os ) const override {}
#endif

    const BOX2I ViewBBox() const override
    {
        BOX2I bb;
        bb.SetMaximum();
        return bb;
    }

    virtual void ViewDraw( int aLayer, KIGFX::VIEW* aView ) const override
    {
        auto gal = aView->GetGAL();
        gal->SetTarget( KIGFX::TARGET_NONCACHED );
        gal->SetIsStroke( true );
        gal->SetStrokeColor( COLOR4D::WHITE );

       /* for( int i=0;i < 100; i++ )
        {
            gal->SetLineWidth( 10000 );
            gal->DrawLine( VECTOR2I(0, i * 30000), VECTOR2I(1000000, i * 30000) );
            gal->DrawLine( VECTOR2I(- 2000000 + i * 30000, 0), VECTOR2I(- 2000000 + i * 30000, 1000000) );
        }

        for( float alpha = 0.0; alpha <= 360.0; alpha += 6.0)
        {
            float ca = cos(alpha * M_PI / 180.0);
            float sa = sin(alpha * M_PI / 180.0);
            VECTOR2I p0(2000000, 0);
            float r = 800000.0;
            gal->DrawLine( p0, p0 + VECTOR2I( r*ca, r*sa));
        }
        */

        int k = 0;
        double w = 0.0;
        //gal->Rotate( 1.0 / 3.0 * M_PI );
        for(int step = 10; step < 80; step += 11, k+=100)
        {
            for (int i = 0; i < 100; i++)
            {
                /*auto p0  = VECTOR2D( k + 100, 100 + i * step );
                auto p1  = VECTOR2D( k + 100 + step/2, 100 + i * step );
                auto p2  = VECTOR2D( k + 100 + step/2, 100 + i * step + step/2 );
                auto p3  = VECTOR2D( k + 100, 100 + i * step + step/2 );
                auto p4  = VECTOR2D( k + 100, 100 + i * step + step );*/

                auto p0  = VECTOR2D( k + 100, 100 + i * step );
                auto p1  = VECTOR2D( k + 100 + step/2, 100 + i * step );
                auto p2  = VECTOR2D( k + 100 + step/2, 100 + i * step + step/2 );
                auto p3  = VECTOR2D( k + 100, 100 + i * step + step/2 );
                // auto p4  = VECTOR2D( k + 100, 100 + i * step + step );


                screenSpaceLine( gal, p0, p1 , w);
                screenSpaceLine( gal, p3, p2 , w);

                p0 += VECTOR2D(50, 0);
                p1 += VECTOR2D(50, 0);
                p2 += VECTOR2D(50, 0);
                p3 += VECTOR2D(50, 0);

                screenSpaceLine( gal, p0, p3 , w);
                screenSpaceLine( gal, p1, p2 , w);


              /*  screenSpaceLine( gal, p1, p2 , w);
                screenSpaceLine( gal, p2, p3 , w);
                screenSpaceLine( gal, p3, p4 , w);

                std::swap(p0.x, p0.y );
                std::swap(p1.x, p1.y );
                std::swap(p2.x, p2.y );
                std::swap(p3.x, p3.y );
                std::swap(p4.x, p4.y );

                screenSpaceLine( gal, p0, p1 , w);
                screenSpaceLine( gal, p1, p2 , w);
                screenSpaceLine( gal, p2, p3 , w);
                screenSpaceLine( gal, p3, p4 , w);*/
            }
        }

        /*
        for(int i=0;i < 1000; i++)
        {
            int k = 0;

            for(double w=1.0; w<=8.0; w+=1.0, k += 50)
            {
                //screenSpaceLine( gal, VECTOR2D( k, i*step ),  VECTOR2D( k, i*step ) + VECTOR2D( 50, 0 ), w );
                //screenSpaceLine( gal, VECTOR2D( i*step, k ), VECTOR2D( i*step, k ) + VECTOR2D( 0, 50 ), w );
            }


            //gal->ScreenSpaceQuad( VECTOR2D( 250, 100 + i*8 ), VECTOR2D( 100, 2 ) );
            //gal->ScreenSpaceQuad( VECTOR2D( 400, 100 + i*4 + 0.5 ), VECTOR2D( 100, 1 ) );
            //gal->ScreenSpaceQuad( VECTOR2D( 550, 100 + i*8 + 0.5 ), VECTOR2D( 100, 2 ) );

        }*/

        for(int i = 1; i < 16; i+=1)
        {
            for(int j = 1; j < 16; j+=1)
            {
                gal->SetIsStroke( true );
                gal->SetIsFill( false );
                //screenSpaceCircle(gal, VECTOR2D(100 + i * 25, 100 + j * 25), (float)i/2.0, 1);
                gal->SetIsStroke( false );
                gal->SetIsFill( true );
                //screenSpaceCircle(gal, VECTOR2D(100 + i * 25, 500 + 100 + j * 25), (float)i/2.0, 1);
            }
        }
    }

    virtual std::vector<int> ViewGetLayers() const override
    {
        return { LAYER_GP_OVERLAY };
    }
};

class MY_PCB_TEST_FRAME : public PCB_TEST_FRAME
{
public:
    MY_PCB_TEST_FRAME( wxFrame* frame, const wxString& title,
            const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
            long style = wxDEFAULT_FRAME_STYLE )
            : PCB_TEST_FRAME( frame, title, pos, size, style, PCB_DRAW_PANEL_GAL::GAL_TYPE_OPENGL )
    {
        registerTools();
        m_galPanel->GetView()->Add( new MY_DRAWING );
        m_galPanel->GetView()->SetScale( 41.3526000000, VECTOR2D( 837362.6373626292, 1581684.9816849837 ) );

    }

    void registerTools();

    virtual ~MY_PCB_TEST_FRAME()
    {
    }
};

wxFrame* CreateMainFrame( const std::string& aFileName )
{
    auto frame = new MY_PCB_TEST_FRAME( nullptr, wxT( "Test PCB Frame" ) );

    if( aFileName != "" )
    {
        frame->LoadAndDisplayBoard( aFileName );
    }

    return frame;
}

void MY_PCB_TEST_FRAME::registerTools()
{
    m_toolManager->InitTools();
    m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );
}
