#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec3 ObjectPosition;

struct Light
{
    vec3 position;
    vec3 color;
    float shininess;
};

uniform int numLights;
uniform Light lights[10];
uniform vec3 viewPos;
uniform bool isSkybox;

void main()
{
    if (isSkybox)
    {
        vec3 direction = normalize(ObjectPosition);

        float horizon =
            clamp(direction.y * 0.5 + 0.5, 0.0, 1.0);

        vec3 horizonColor = vec3(0.32, 0.18, 0.55);
        vec3 zenithColor = vec3(0.04, 0.08, 0.24);

        vec3 skyColor =
            mix(horizonColor, zenithColor, horizon);

        FragColor = vec4(skyColor, 1.0);
        return;
    }

    vec3 normal = normalize(Normal);
    vec3 normalColor = normal * 0.5 + 0.5;

    vec3 colorA = vec3(0.12, 0.25, 0.85);
    vec3 colorB = vec3(0.75, 0.18, 0.95);
    vec3 baseColor = mix(colorA, colorB, normalColor.y);

    vec3 result = baseColor * 0.18;

    for (int i = 0; i < numLights; ++i)
    {
        vec3 lightDir =
            normalize(lights[i].position - FragPos);

        float diffuseStrength =
            max(dot(normal, lightDir), 0.0);

        vec3 viewDir =
            normalize(viewPos - FragPos);

        vec3 halfwayDir =
            normalize(lightDir + viewDir);

        float specularStrength =
            pow(
                max(dot(normal, halfwayDir), 0.0),
                max(lights[i].shininess, 1.0)
            );

        vec3 diffuse =
            diffuseStrength *
            lights[i].color *
            baseColor;

        vec3 specular =
            0.35 *
            specularStrength *
            lights[i].color;

        result += diffuse + specular;
    }

    result = result / (result + vec3(1.0));

    FragColor = vec4(result, 1.0);
}
