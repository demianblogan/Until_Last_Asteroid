uniform sampler2D source;
uniform sampler2D bloomTexture;
uniform vec3 tint;
uniform float saturation;
uniform float contrast;
uniform float bloomIntensity;
uniform float vignetteStrength;
uniform float damageVignette;
uniform float aspectRatio;
uniform int shockwaveCount;
uniform vec2 shockwaveCenters[8];
uniform float shockwaveRadii[8];
uniform float shockwaveStrengths[8];
uniform float timeSlowdownStrength;

void main()
{
    vec2 uv = gl_TexCoord[0].xy;
    vec2 sceneUv = uv;

    for (int index = 0; index < 8; ++index)
    {
		if (index >= shockwaveCount)
			break;
		vec2 delta = uv - shockwaveCenters[index];
        delta.x *= aspectRatio;
        float distanceFromCenter = length(delta);
		float ringDistance = abs(distanceFromCenter - shockwaveRadii[index]);
        float ring = exp(-ringDistance * ringDistance / 0.00020);
        vec2 direction = distanceFromCenter > 0.0001 ? delta / distanceFromCenter : vec2(0.0);
        direction.x /= aspectRatio;
		sceneUv += direction * ring * shockwaveStrengths[index] * 0.010;
    }
	sceneUv = clamp(sceneUv, 0.001, 0.999);

    vec2 chromaOffset = vec2(0.0022 * timeSlowdownStrength, 0.0);
    vec3 color = texture2D(source, sceneUv).rgb;
    if (timeSlowdownStrength > 0.0)
    {
        color.r = texture2D(source, clamp(sceneUv + chromaOffset, 0.001, 0.999)).r;
        color.b = texture2D(source, clamp(sceneUv - chromaOffset, 0.001, 0.999)).b;
    }
    color += texture2D(bloomTexture, uv).rgb * bloomIntensity;

    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    color = mix(vec3(luminance), color, saturation);
    color = (color - 0.5) * contrast + 0.5;
    color *= tint;

    float slowLuminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    vec3 temporalColor = mix(
        vec3(slowLuminance) * vec3(0.70, 0.84, 1.12),
        color * vec3(0.82, 0.92, 1.14),
        0.48);
    color = mix(color, temporalColor, timeSlowdownStrength * 0.72);

    vec2 centered = uv - vec2(0.5);
    centered.x *= aspectRatio;
    float edgeDistance = length(centered);
    float vignette = smoothstep(0.36, 0.91, edgeDistance);
    color *= 1.0 - vignette * (vignetteStrength + timeSlowdownStrength * 0.13);

    float damageEdge = smoothstep(0.23, 0.88, edgeDistance) * damageVignette;
    color = mix(color, vec3(0.58, 0.015, 0.008), damageEdge * 0.66);
    color *= 1.0 - damageEdge * 0.16;

    gl_FragColor = vec4(max(color, vec3(0.0)), 1.0);
}
