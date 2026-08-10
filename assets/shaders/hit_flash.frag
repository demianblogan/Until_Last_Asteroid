uniform sampler2D source;
uniform float intensity;

void main()
{
    vec4 color = texture2D(source, gl_TexCoord[0].xy) * gl_Color;
    vec3 flashColor = vec3(0.82, 0.96, 1.0);
    color.rgb = mix(color.rgb, flashColor, clamp(intensity, 0.0, 1.0) * color.a);
    gl_FragColor = color;
}
