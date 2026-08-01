#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

struct Light {
    vec3 position;
    vec3 color;
    float shininess;
};

uniform int numLights;
uniform Light lights[10]; // max 10 lights
uniform vec3 viewPos;
uniform sampler2D texture1;

void main()
{
    vec3 norm = normalize(Normal);
    vec3 texColor = texture(texture1, TexCoords).rgb;

    vec3 result = vec3(0.0);

    for (int i = 0; i < numLights; ++i)
    {
        vec3 lightDir = normalize(lights[i].position - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);

        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), lights[i].shininess);

        // Slightly stronger ambient and specular
        vec3 ambient  = 0.15 * lights[i].color;  // boost ambient a bit
        vec3 diffuse  = diff * lights[i].color;
        vec3 specular = 10.5 * spec * lights[i].color; // half-strength specular

        result += ambient + diffuse + specular;
    }

    // Tone mapping to prevent over-brightening
    result = result / (result + vec3(1.0));

    FragColor = vec4(result * texColor, 1.0);
}
