uniform sampler2D source;
uniform sampler2D bloomTexture;
uniform vec3 tint;
uniform float saturation;
uniform float contrast;
uniform float bloomIntensity;
uniform float vignetteStrength;
uniform float damageVignette;
uniform float aspectRatio;
uniform bool shockwaveActive;
uniform vec2 shockwaveCenter;
uniform float shockwaveRadius;
uniform float shockwaveStrength;

void main()
{
    vec2 uv = gl_TexCoord[0].xy;
    vec2 sceneUv = uv;

    if (shockwaveActive && shockwaveStrength > 0.0)
    {
        vec2 delta = uv - shockwaveCenter;
        delta.x *= aspectRatio;
        float distanceFromCenter = length(delta);
        float ringDistance = abs(distanceFromCenter - shockwaveRadius);
        float ring = exp(-ringDistance * ringDistance / 0.00020);
        vec2 direction = distanceFromCenter > 0.0001 ? delta / distanceFromCenter : vec2(0.0);
        direction.x /= aspectRatio;
        sceneUv = clamp(uv + direction * ring * shockwaveStrength * 0.010, 0.001, 0.999);
    }

    vec3 color = texture2D(source, sceneUv).rgb;
    color += texture2D(bloomTexture, uv).rgb * bloomIntensity;

    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    color = mix(vec3(luminance), color, saturation);
    color = (color - 0.5) * contrast + 0.5;
    color *= tint;

    vec2 centered = uv - vec2(0.5);
    centered.x *= aspectRatio;
    float edgeDistance = length(centered);
    float vignette = smoothstep(0.36, 0.91, edgeDistance);
    color *= 1.0 - vignette * vignetteStrength;

    float damageEdge = smoothstep(0.23, 0.88, edgeDistance) * damageVignette;
    color = mix(color, vec3(0.58, 0.015, 0.008), damageEdge * 0.66);
    color *= 1.0 - damageEdge * 0.16;

    gl_FragColor = vec4(max(color, vec3(0.0)), 1.0);
}
