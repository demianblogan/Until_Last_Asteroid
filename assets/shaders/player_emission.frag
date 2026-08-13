uniform sampler2D source;

void main()
{
    vec4 sampleColor = texture2D(source, gl_TexCoord[0].xy) * gl_Color;
    float cyanBase = min(sampleColor.g, sampleColor.b);
    float cyanDominance = cyanBase - sampleColor.r * 0.62;
    float cyanMask = smoothstep(0.08, 0.34, cyanDominance) *
        smoothstep(0.22, 0.70, max(sampleColor.g, sampleColor.b)) * sampleColor.a;
    vec3 emissionColor = vec3(
        sampleColor.r * 0.18,
        sampleColor.g * 1.18,
        sampleColor.b * 1.42);
    gl_FragColor = vec4(emissionColor * cyanMask, cyanMask * 0.86);
}
