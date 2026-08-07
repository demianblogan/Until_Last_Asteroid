uniform sampler2D source;
uniform vec2 direction;

void main()
{
    vec2 coordinate = gl_TexCoord[0].xy;

    vec4 color = texture2D(source, coordinate) * 0.2270270270;
    color += texture2D(source, coordinate + direction * 1.3846153846) * 0.3162162162;
    color += texture2D(source, coordinate - direction * 1.3846153846) * 0.3162162162;
    color += texture2D(source, coordinate + direction * 3.2307692308) * 0.0702702703;
    color += texture2D(source, coordinate - direction * 3.2307692308) * 0.0702702703;

    gl_FragColor = gl_Color * color;
}
