uniform sampler2D source;

void main()
{
    vec4 sampleColor = texture2D(source, gl_TexCoord[0].xy) * gl_Color;
    float competingChannel = max(sampleColor.g, sampleColor.b);
    float redDominance = sampleColor.r - competingChannel * 0.82;
    float redMask = smoothstep(0.10, 0.38, redDominance) *
        smoothstep(0.24, 0.72, sampleColor.r) * sampleColor.a;
    vec3 emissionColor = vec3(
        sampleColor.r * 1.35,
        sampleColor.g * 0.34 + redMask * 0.08,
        sampleColor.b * 0.30);
    gl_FragColor = vec4(emissionColor * redMask, redMask * 0.82);
}
