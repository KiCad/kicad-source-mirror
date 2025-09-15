# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright (C) 2025 Mark Roszko <mark.roszko@gmail.com>
# Copyright The KiCad Developers, see AUTHORS.txt for contributors.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, you may find one here:
# http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
# or you may search the http://www.gnu.org website for the version 2 license,
# or you may write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

meta:
  id: u3d
  file-extension: u3d
  endian: le
doc: |
  U3D is file format for 3D models which is documented by ECMA-363
  https://ecma-international.org/wp-content/uploads/ECMA-363_4th_edition_june_2007.pdf

  This Katai struct definition is not fully complete but good enough.
seq:
  - id: content
    type: block
    repeat: eos
types:
  file_header:
    seq:
      - id: version_major
        type: u2
      - id: version_minor
        type: u2
      - id: profile_id
        type: u4
      - id: declaration_size
        type: u4
      - id: file_size
        type: u8
      - id: encoding
        type: u4
      - id: scaling_factor
        type: f8
        if: profile_id & 0x8 != 0
  block_header:
    seq:
      - id: block_type
        type: u4
        enum: block_type
      - id: data_size
        type: u4
      - id: meta_size
        type: u4
  block:
    seq:
      - id: header
        type: block_header
      - id: data
        size: header.data_size
        type:
          switch-on: header.block_type
          cases:
            'block_type::file_header': file_header
            'block_type::group_node': group_node
            'block_type::model_node': model_node
            'block_type::clod_mesh_declaration': clod_mesh_declaration
            'block_type::clod_mesh_cont': clod_mesh_cont
            'block_type::shading_modifier': shading_modifier
            'block_type::lit_texture_shader': lit_texture_shader
            'block_type::material_resource': material_resource
            'block_type::modifier_chain': modifier_chain
            'block_type::light_node': light_node
            'block_type::light_resource': light_resource
      - id: data_padding
        size: (- _io.pos) % 4
  string:
    seq:
      - id: length
        type: u2
      - id: chars
        type: str
        size: length
        encoding: UTF-8
  group_node:
    seq:
      - id: group_name
        type: string
      - id: parent_node
        type: parent_node_data
  parent_node_data:
    seq:
      - id: parent_node_count
        type: u4
      - id: parent_node_name
        type: string
        if: parent_node_count > 0
      - id: parent_node_matrix
        type: transform_matrix
        if: parent_node_count > 0
  transform_matrix:
    seq:
      - id: a
        type: f4
      - id: b
        type: f4
      - id: c
        type: f4
      - id: d
        type: f4
      - id: e
        type: f4
      - id: f
        type: f4
      - id: g
        type: f4
      - id: h
        type: f4
      - id: i
        type: f4
      - id: j
        type: f4
      - id: k
        type: f4
      - id: l
        type: f4
      - id: m
        type: f4
      - id: n
        type: f4
      - id: o
        type: f4
      - id: p
        type: f4
  lit_texture_shader:
    seq:
      - id: name
        type: string
      - id: attributes
        type: u4
      - id: alpha_test_reference
        type: f4
      - id: alpha_test_function
        type: u4
      - id: color_blend_function
        type: u4
      - id: render_pass_enabled_flags
        type: u4
      - id: shader_channels
        type: u4
      - id: alpha_texture_channels
        type: u4
      - id: material_name
        type: string
    # TBD texture information
    
  light_resource:
    seq:
      - id: name
        type: string
      - id: attributes
        type: u4
      - id: type
        type: u1
      - id: color
        type: color4
      - id: attn_constant
        type: f4
      - id: attn_linear
        type: f4
      - id: attn_quadratic
        type: f4
      - id: spot_angle
        type: f4
      - id: intensity
        type: f4
        
  material_resource:
    seq:
      - id: name
        type: string
      - id: attributes
        type: u4
      - id: ambient_color
        type: color3
      - id: diffuse_color
        type: color3
      - id: specular_color
        type: color3
      - id: emissive_color
        type: color3
      - id: reflectively
        type: f4
      - id: opacity
        type: f4
  color3:
    seq:
      - id: red
        type: f4
      - id: green
        type: f4
      - id: blue
        type: f4
  color4:
    seq:
      - id: red
        type: f4
      - id: green
        type: f4
      - id: blue
        type: f4
      - id: alpha
        type: f4
  clod_mesh_declaration:
    seq:
      - id: name
        type: string
      - id: chain_index
        type: u4
      - id: max_mesh_description
        type: max_mesh_description
      - id: clod_description
        type: clod_description
      - id: resource_description
        type: resource_description
      - id: skeleton_description
        type: skeleton_description
        
  clod_mesh_cont:
    seq:
      - id: mesh_name
        type: string
      - id: chain_index # should always be zero for this type
        type: u4
      # base mesh description
      - id: face_count
        type: u4
      - id: position_count
        type: u4
      - id: normal_count
        type: u4
      - id: diffuse_color_count
        type: u4
      - id: specular_color_count
        type: u4
      - id: texture_coord_count
        type: u4
      - id: base_positions
        type: coord
        repeat: expr
        repeat-expr: position_count
      - id: base_normals
        type: coord
        repeat: expr
        repeat-expr: normal_count
      - id: base_diffuse_colors
        type: color4
        repeat: expr
        repeat-expr: diffuse_color_count
      - id: base_specular_color
        type: color4
        repeat: expr
        repeat-expr: specular_color_count
      - id: base_texture_coords
        type: texture_coord
        repeat: expr
        repeat-expr: texture_coord_count
      - id: base_faces
        type: base_face
        repeat: expr
        repeat-expr: 1
  base_face:
    seq:
      - id: shading_id
        type: u4
  coord:
    seq:
      - id: x
        type: f4
      - id: y
        type: f4
      - id: z
        type: f4
  texture_coord:
    seq:
      - id: u
        type: f4
      - id: v
        type: f4
      - id: s
        type: f4
      - id: t
        type: f4
  clod_description:
    seq:
      - id: minimum_resolution
        type: u4
      - id: final_maximum_resolution
        type: u4
  resource_description:
    seq:
      - id: quality_factors
        type: quality_factors
      - id: inverse_quantization
        type: inverse_quantization
      - id: resource_parameters
        type: resource_parameters
  quality_factors:
    seq:
      - id: position_quality_factor
        type: u4
      - id: normal_quality_factor
        type: u4
      - id: texture_quality_factor
        type: u4
  inverse_quantization:
    seq:
      - id: position_inverse_quant
        type: f4
      - id: normal_inverse_quant
        type: f4
      - id: texture_coord_inverse_quant
        type: f4
      - id: diffuse_color_inverse_quant
        type: f4
      - id: specular_color_inverse_quant
        type: f4
  resource_parameters:
    seq:
      - id: normal_crease_parameter
        type: f4
      - id: normal_update_parameter
        type: f4
      - id: normal_tolerance_parameter
        type: f4
  skeleton_description:
    seq:
      - id: bone_count
        type: u4
  max_mesh_description:
    seq:
      - id: attributes
        type: u4
      - id: face_count
        type: u4
      - id: position_count
        type: u4
      - id: normal_count
        type: u4
      - id: diffuse_color_count
        type: u4
      - id: specular_color_count
        type: u4
      - id: texture_color_count
        type: u4
      - id: shading_count
        type: u4
      - id: shading_descriptions
        type: max_mesh_description_shading_description
        repeat: expr
        repeat-expr: shading_count
  max_mesh_description_shading_description:
    seq:
      - id: attributes
        type: u4
      - id: texture_layer_count
        type: u4
      - id: texture_layer_dimensions
        type: u4
        repeat: expr
        repeat-expr: texture_layer_count
      - id: original_shading_id
        type: u4
  model_node:
    seq:
      - id: name
        type: string
      - id: parent_node
        type: parent_node_data
      - id: model_resource_name
        type: string
      - id: visibility
        type: u4
        enum: model_visibility
  light_node:
    seq:
      - id: name
        type: string
      - id: parent_node
        type: parent_node_data
      - id: light_resource_name
        type: string
  shading_modifier:
    seq:
      - id: name
        type: string
      - id: chain_index
        type: u4
      - id: attributes
        type: u4
      - id: shader_list_count
        type: u4
      - id: shader
        type: shading_modifier_shader_list_shader
        repeat: expr
        repeat-expr: shader_list_count
  shading_modifier_shader_list_shader:
    seq:
      - id: shader_count
        type: u4
      - id: shader_name
        type: string
  modifier_chain:
    seq:
      - id: name
        type: string
      - id: type
        type: u4
        enum: chain_types
      - id: attributes
        type: u4
      - id: padding
        size: (- _io.pos) % 4
      # some things here
      - id: modifier_count
        type: u4
      - id: modifiers
        type: block
        repeat: expr
        repeat-expr: modifier_count
      
      
        
        
enums:
  block_type:
    0x00443355: file_header
    0xFFFFFF21: group_node
    0xFFFFFF22: model_node
    0xFFFFFF31: clod_mesh_declaration
    0xFFFFFF3B: clod_mesh_cont
    0xFFFFFF45: shading_modifier
    0xFFFFFF53: lit_texture_shader
    0xFFFFFF54: material_resource
    0xFFFFFF14: modifier_chain
    0xFFFFFF23: light_node
    0xFFFFFF51: light_resource
  chain_types:
    0: node_modifier_chain
    1: model_resource_modifier
    2: texture_resource_modifier
  model_visibility:
    0: not_visible
    1: front_visible
    2: back_visible
    3: front_and_back_visible