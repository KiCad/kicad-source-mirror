/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 */

#include <rhi/rhi_metal_view.h>

#import <AppKit/NSView.h>
#import <AppKit/NSColorSpace.h>
#import <AppKit/NSWindow.h>
#import <Foundation/NSThread.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>


// wxWidgets' canvas view paints over an assigned Metal layer. A separate layer-backed
// NSView avoids its drawRect: path while allowing mouse events through to the canvas.
@interface KiCadMetalView : NSView
@end

@implementation KiCadMetalView

- (CALayer*) makeBackingLayer
{
    CAMetalLayer* layer = [CAMetalLayer layer];
    layer.pixelFormat     = MTLPixelFormatBGRA8Unorm;
    layer.framebufferOnly = NO;
    layer.opaque          = YES;

    return layer;
}

- (BOOL) wantsUpdateLayer
{
    return YES;
}

- (void) updateLayer
{
    // Metal presents from the render thread instead of AppKit's display cycle.
}

- (NSView*) hitTest:(NSPoint)aPoint
{
    // Keep the host view transparent to wxWidgets canvas input.
    return nil;
}

// A Metal layer without a colour space is shown as sRGB but the OpenGL canvas uses the window's,
// so match the window to keep colours identical on both canvases.
- (void) followWindowColorSpace
{
    if( [self.layer isKindOfClass:[CAMetalLayer class]] && self.window )
        ( (CAMetalLayer*) self.layer ).colorspace = self.window.colorSpace.CGColorSpace;
}

- (void) viewDidMoveToWindow
{
    [super viewDidMoveToWindow];
    [self followWindowColorSpace];
}

- (void) viewDidChangeBackingProperties
{
    [super viewDidChangeBackingProperties];

    if( [self.layer isKindOfClass:[CAMetalLayer class]] && self.window )
        ( (CAMetalLayer*) self.layer ).contentsScale = self.window.backingScaleFactor;

    [self followWindowColorSpace];
}

@end


RHI_METAL_HOST_VIEW RhiMetalAttachHostView( void* aCanvasView )
{
    if( !aCanvasView || ![NSThread isMainThread] )
        return {};

    NSView* canvasView = (__bridge NSView*) aCanvasView;
    KiCadMetalView* view = [[KiCadMetalView alloc] initWithFrame:canvasView.bounds];
    view.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    [view setWantsLayer:YES];
    view.layerContentsRedrawPolicy = NSViewLayerContentsRedrawNever;

    // Go beneath the canvas's existing children so the infobar remains visible.
    [canvasView addSubview:view positioned:NSWindowBelow relativeTo:nil];

    CAMetalLayer* layer = (CAMetalLayer*) view.layer;

    if( !layer )
    {
        [view removeFromSuperview];
        return {};
    }

    return { (__bridge void*) view, (__bridge void*) layer };
}


void RhiMetalDetachHostView( void* aHostView )
{
    if( !aHostView || ![NSThread isMainThread] )
        return;

    [(__bridge NSView*) aHostView removeFromSuperview];
}
