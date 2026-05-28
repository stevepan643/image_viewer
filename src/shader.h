#ifndef IMG_VWR_SHADER_H
#define IMG_VWR_SHADER_H

const char *vertex_code =
"#version 330 core\n" \
"layout (location = 0) in vec3 aPos;\n" \
"layout (location = 1) in vec2 aTex;\n" \
"out vec2 vTex;\n" \
"uniform mat4 tex_trs;\n" \
"void main() {\n" \
"   gl_Position = vec4(aPos, 1.0);\n" \
"   vec2 uv = vec2(aTex.x, 1.0 - aTex.y);\n" \
"   vTex = vec2(tex_trs * vec4(uv, 0.0, 1.0));\n" \
"}";
const char *fragment_code =
"#version 330 core\n" \
"out vec4 FragColor;\n" \
"in vec2 vTex;\n" \
"uniform sampler2D texture1;\n" \
"void main() {\n" \
"   if (vTex.x < 0.0 || vTex.x > 1.0 || vTex.y < 0.0 || vTex.y > 1.0) {\n" \
"       discard;" \
"   }" \
"   FragColor = texture(texture1, vTex);\n" \
"}";

#endif
