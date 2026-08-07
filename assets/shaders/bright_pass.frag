uniform sampler2D source;
uniform float threshold;
uniform float softness;

void main()
{
    vec4 sampleColor = texture2D(source, gl_TexCoord[0].xy);
    float brightness = max(sampleColor.r, max(sampleColor.g, sampleColor.b));
    float emission = smoothstep(threshold - softness, threshold + softness, brightness);
    emission *= sampleColor.a;

    gl_FragColor = gl_Color * vec4(emission, emission, emission, emission);
}
