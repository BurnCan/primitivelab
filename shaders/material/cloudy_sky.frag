#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec3 ObjectPosition;

uniform bool isSkybox;

// A small, deterministic 3D value-noise implementation keeps the pattern
// continuous where the six faces of the skybox meet.
float hash(vec3 point)
{
    point = fract(point * 0.3183099 + vec3(0.11, 0.17, 0.13));
    point *= 17.0;
    return fract(point.x * point.y * point.z * (point.x + point.y + point.z));
}

float noise(vec3 point)
{
    vec3 cell = floor(point);
    vec3 local = fract(point);
    vec3 blend = local * local * (3.0 - 2.0 * local);

    float x00 = mix(hash(cell + vec3(0.0, 0.0, 0.0)),
                    hash(cell + vec3(1.0, 0.0, 0.0)), blend.x);
    float x10 = mix(hash(cell + vec3(0.0, 1.0, 0.0)),
                    hash(cell + vec3(1.0, 1.0, 0.0)), blend.x);
    float x01 = mix(hash(cell + vec3(0.0, 0.0, 1.0)),
                    hash(cell + vec3(1.0, 0.0, 1.0)), blend.x);
    float x11 = mix(hash(cell + vec3(0.0, 1.0, 1.0)),
                    hash(cell + vec3(1.0, 1.0, 1.0)), blend.x);

    return mix(mix(x00, x10, blend.y),
               mix(x01, x11, blend.y), blend.z);
}

float turbulence(vec3 point)
{
    float value = 0.0;
    float amplitude = 0.5;

    for (int octave = 0; octave < 5; ++octave)
    {
        value += amplitude * noise(point);
        point = point * 2.03 + vec3(7.1, 3.7, 5.9);
        amplitude *= 0.5;
    }

    return value;
}

void main()
{
    // PrimitiveLab supplies the original cube position to material shaders.
    // Sampling a normalized direction avoids per-face orientation uniforms.
    vec3 direction = normalize(isSkybox ? ObjectPosition : Normal);
    float height = clamp(direction.y * 0.5 + 0.5, 0.0, 1.0);

    vec3 horizonColor = vec3(0.50, 0.70, 0.95);
    vec3 zenithColor = vec3(0.16, 0.39, 0.78);
    vec3 skyColor = mix(horizonColor, zenithColor, smoothstep(0.0, 0.85, height));

    // Stretch the field horizontally to form broad, soft cloud banks.
    vec3 cloudPoint = direction * vec3(3.5, 1.6, 3.5);
    float cloudField = turbulence(cloudPoint);
    float cloudDensity = smoothstep(0.48, 0.68, cloudField);

    // Keep the zenith airy and fade clouds below the visible horizon.
    float horizonMask = smoothstep(0.08, 0.35, height);
    float zenithMask = 1.0 - 0.35 * smoothstep(0.72, 1.0, height);
    float clouds = cloudDensity * horizonMask * zenithMask;

    vec3 cloudShadow = vec3(0.72, 0.78, 0.86);
    vec3 cloudLight = vec3(1.0, 0.99, 0.96);
    vec3 cloudColor = mix(cloudShadow, cloudLight,
                          smoothstep(0.50, 0.86, cloudField));

    vec3 finalColor = mix(skyColor, cloudColor, clouds * 0.88);
    FragColor = vec4(finalColor, 1.0);
}
