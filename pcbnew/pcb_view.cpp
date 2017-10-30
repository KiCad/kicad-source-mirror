#include <pcb_view.h>
#include <pcb_display_options.h>
#include <pcb_painter.h>

namespace KIGFX
{

PCB_VIEW::PCB_VIEW( bool aIsDynamic ) :
    VIEW ( aIsDynamic )
{

}

PCB_VIEW::~PCB_VIEW()
{

}

void PCB_VIEW::Add( VIEW_ITEM* aItem, int aDrawPriority )
{
    VIEW::Add( aItem, aDrawPriority );
}

void PCB_VIEW::Remove( VIEW_ITEM* aItem )
{
    VIEW::Remove( aItem );
}

void PCB_VIEW::Update( VIEW_ITEM* aItem, int aUpdateFlags )
{
    VIEW::Update( aItem, aUpdateFlags );
}

/// @copydoc VIEW::Update()
void PCB_VIEW::Update( VIEW_ITEM* aItem )
{
    VIEW::Update( aItem );
}

void PCB_VIEW::UpdateDisplayOptions( PCB_DISPLAY_OPTIONS* aOptions )
{
    auto painter = static_cast<KIGFX::PCB_PAINTER*>( GetPainter() );
    auto settings = static_cast<KIGFX::PCB_RENDER_SETTINGS*>( painter->GetSettings() );
    settings->LoadDisplayOptions( aOptions );
}

};
