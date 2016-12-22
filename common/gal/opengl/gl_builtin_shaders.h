#ifndef GAL_GL_BUILTIN_SHADERS_H__
#define GAL_GL_BUILTIN_SHADERS_H__

namespace KIGFX {

    namespace BUILTIN_SHADERS {

        extern const char kicad_vertex_shader[];
        extern const char kicad_fragment_shader[];

        extern const char ssaa_x4_vertex_shader[];
        extern const char ssaa_x4_fragment_shader[];

        extern const char smaa_base_shader_p1[];
        extern const char smaa_base_shader_p2[];
        extern const char smaa_base_shader_p3[];
        extern const char smaa_base_shader_p4[];

        extern const char smaa_pass_1_vertex_shader[];
        extern const char smaa_pass_1_fragment_shader[];
        extern const char smaa_pass_2_vertex_shader[];
        extern const char smaa_pass_2_fragment_shader[];
        extern const char smaa_pass_3_vertex_shader[];
        extern const char smaa_pass_3_fragment_shader[];

    }
}

#endif
