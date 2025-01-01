/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Andrew Lutsenko, anlutsenko at gmail dot com
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file shape.i
 * @brief General wrappers for geometry/shape headers
 */

/* Warning 509: Overloaded method ignored */
#pragma SWIG nowarn=509

%include <std_shared_ptr.i>
%include <std_vector.i>

%{
#include <geometry/corner_strategy.h>
#include <geometry/seg.h>
#include <geometry/shape.h>
#include <geometry/shape_arc.h>
#include <geometry/shape_circle.h>
#include <geometry/shape_compound.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_poly_set.h>
#include <geometry/shape_simple.h>
#include <geometry/shape_rect.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_simple.h>
#include <geometry/approximation.h>
%}

%shared_ptr(SHAPE)
%shared_ptr(SHAPE_BASE)
%shared_ptr(SHAPE_COMPOUND)
%shared_ptr(SHAPE_POLY_SET)
%shared_ptr(SHAPE_LINE_CHAIN_BASE)
%shared_ptr(SHAPE_LINE_CHAIN)
%shared_ptr(SHAPE_SIMPLE)
%shared_ptr(SHAPE_ARC)
%shared_ptr(SHAPE_CIRCLE)
%shared_ptr(SHAPE_RECT)
%shared_ptr(SHAPE_SEGMENT)

// ignore warning from nested classes
#pragma SWIG nowarn=325
%include <geometry/corner_strategy.h>
%include <geometry/seg.h>
%include <geometry/shape.h>
%include <geometry/shape_arc.h>
%include <geometry/shape_circle.h>
%include <geometry/shape_compound.h>
%include <geometry/shape_line_chain.h>
%include <geometry/shape_poly_set.h>
%include <geometry/shape_rect.h>
%include <geometry/shape_segment.h>
%include <geometry/shape_simple.h>
%include <geometry/approximation.h>

%template(VECTOR_SHAPEPTR) std::vector<std::shared_ptr<SHAPE>>;

static std::shared_ptr<SHAPE_ARC> Cast_to_SHAPE_ARC( std::shared_ptr<SHAPE> self );
static std::shared_ptr<SHAPE_CIRCLE> Cast_to_SHAPE_CIRCLE( std::shared_ptr<SHAPE> self );
static std::shared_ptr<SHAPE_COMPOUND> Cast_to_SHAPE_COMPOUND( std::shared_ptr<SHAPE> self );
static std::shared_ptr<SHAPE_LINE_CHAIN> Cast_to_SHAPE_LINE_CHAIN( std::shared_ptr<SHAPE> self );
static std::shared_ptr<SHAPE_POLY_SET> Cast_to_SHAPE_POLY_SET( std::shared_ptr<SHAPE> self );
static std::shared_ptr<SHAPE_RECT> Cast_to_SHAPE_RECT( std::shared_ptr<SHAPE> self );
static std::shared_ptr<SHAPE_SEGMENT> Cast_to_SHAPE_SEGMENT( std::shared_ptr<SHAPE> self );
static std::shared_ptr<SHAPE_SIMPLE> Cast_to_SHAPE_SIMPLE( std::shared_ptr<SHAPE> self );

%{
static std::shared_ptr<SHAPE_ARC> Cast_to_SHAPE_ARC( std::shared_ptr<SHAPE> self ) { return std::dynamic_pointer_cast<SHAPE_ARC>( self ); }
static std::shared_ptr<SHAPE_CIRCLE> Cast_to_SHAPE_CIRCLE( std::shared_ptr<SHAPE> self ) { return std::dynamic_pointer_cast<SHAPE_CIRCLE>( self ); }
static std::shared_ptr<SHAPE_COMPOUND> Cast_to_SHAPE_COMPOUND( std::shared_ptr<SHAPE> self ) { return std::dynamic_pointer_cast<SHAPE_COMPOUND>( self ); }
static std::shared_ptr<SHAPE_LINE_CHAIN> Cast_to_SHAPE_LINE_CHAIN( std::shared_ptr<SHAPE> self ) { return std::dynamic_pointer_cast<SHAPE_LINE_CHAIN>( self ); }
static std::shared_ptr<SHAPE_POLY_SET> Cast_to_SHAPE_POLY_SET( std::shared_ptr<SHAPE> self ) { return std::dynamic_pointer_cast<SHAPE_POLY_SET>( self ); }
static std::shared_ptr<SHAPE_RECT> Cast_to_SHAPE_RECT( std::shared_ptr<SHAPE> self ) { return std::dynamic_pointer_cast<SHAPE_RECT>( self ); }
static std::shared_ptr<SHAPE_SEGMENT> Cast_to_SHAPE_SEGMENT( std::shared_ptr<SHAPE> self ) { return std::dynamic_pointer_cast<SHAPE_SEGMENT>( self ); }
static std::shared_ptr<SHAPE_SIMPLE> Cast_to_SHAPE_SIMPLE( std::shared_ptr<SHAPE> self ) { return std::dynamic_pointer_cast<SHAPE_SIMPLE>( self ); }
%}

%extend SHAPE
{
    %pythoncode
    %{
        def Cast(self):
            shape_type = SHAPE_TYPE_asString(self.Type())

            if shape_type == "SH_ARC":
                return Cast_to_SHAPE_ARC(self)
            elif shape_type == "SH_CIRCLE":
                return Cast_to_SHAPE_CIRCLE(self)
            elif shape_type == "SH_COMPOUND":
                return Cast_to_SHAPE_COMPOUND(self)
            elif shape_type == "SH_LINE_CHAIN":
                return Cast_to_SHAPE_LINE_CHAIN(self)
            elif shape_type == "SH_POLY_SET":
                return Cast_to_SHAPE_POLY_SET(self)
            elif shape_type == "SH_RECT":
                return Cast_to_SHAPE_RECT(self)
            elif shape_type == "SH_SEGMENT":
                return Cast_to_SHAPE_SEGMENT(self)
            elif shape_type == "SH_SIMPLE":
                return Cast_to_SHAPE_SIMPLE(self)
            else:
                raise TypeError("Unsupported shape class: %s" % shape_type)
    %}
}


%extend SHAPE_COMPOUND
{
    std::vector<std::shared_ptr<SHAPE> > GetSubshapes()
    {
        std::vector<std::shared_ptr<SHAPE>> result;
        std::for_each( $self->Shapes().begin(), $self->Shapes().end(),
                       [&]( SHAPE* shape )
                       {
                           // creating empty shared_ptr with non-null pointer
                           // so that it doesn't "own" the raw pointer
                           result.emplace_back( std::shared_ptr<SHAPE>(), shape );
                       } );
        return result;
    }
}


%extend std::vector<std::shared_ptr<SHAPE>>
{
    %pythoncode
    %{
        def __iter__(self):
            it = self.iterator()
            try:
                while True:
                    item = it.next()  # throws StopIteration when iterator reached the end.
                    yield item.Cast()
            except StopIteration:
                return
    %}
}
