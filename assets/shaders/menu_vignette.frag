uniform float aspectRatio;
uniform float strength;
uniform vec2 viewportOrigin;
uniform vec2 viewportSize;

void main()
{
    vec2 uv = (gl_FragCoord.xy - viewportOrigin) / viewportSize;
    vec2 centered = uv - vec2(0.5);
    centered.x *= aspectRatio;
    float edgeDistance = length(centered);
    float vignette = smoothstep(0.32, 0.90, edgeDistance) * strength;
    gl_FragColor = vec4(0.0, 0.012, 0.030, vignette);
}
