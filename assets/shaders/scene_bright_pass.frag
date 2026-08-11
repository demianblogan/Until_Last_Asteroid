uniform sampler2D source;
uniform float threshold;
uniform float softness;

void main()
{
    vec4 sampleColor = texture2D(source, gl_TexCoord[0].xy);
    float brightness = max(sampleColor.r, max(sampleColor.g, sampleColor.b));
    float emission = smoothstep(threshold - softness, threshold + softness, brightness);
    gl_FragColor = vec4(sampleColor.rgb * emission, sampleColor.a * emission);
}
