#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec3 ObjectPosition;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform bool isSkybox;

void main()
{
    ObjectPosition = aPos;
    TexCoords = aTexCoords;

    vec4 worldPosition = model * vec4(aPos, 1.0);
    FragPos = worldPosition.xyz;
    Normal = mat3(transpose(inverse(model))) * aNormal;

    if (isSkybox)
    {
        mat4 rotationOnlyView = mat4(mat3(view));

        vec4 clipPosition =
            projection *
            rotationOnlyView *
            model *
            vec4(aPos, 1.0);

        gl_Position = clipPosition.xyww;
    }
    else
    {
        gl_Position = projection * view * worldPosition;
    }
}
