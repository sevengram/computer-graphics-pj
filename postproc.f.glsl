//uniform sampler2D fbo_texture;
//uniform float offset;
//varying vec2 f_texcoord;
//
//void main(void) {
//  vec2 texcoord = f_texcoord;
//  texcoord.x += sin(texcoord.y * 4.0 * 2.0 * 3.14159 + offset) / 100.0;
//  gl_FragColor = texture2D(fbo_texture, texcoord);
//}
//

uniform sampler2D fbo_texture;
varying vec2 f_texcoord;
 
void main()
{
    vec2 gaussFilter[7];
    gaussFilter[0] = vec2(-0.03,0.015625);
    gaussFilter[1] = vec2(-0.02,0.09375);
    gaussFilter[2] = vec2(-0.01,0.234375);
    gaussFilter[3] = vec2(0.0,0.3125);
    gaussFilter[4] = vec2(0.01,0.234375);
    gaussFilter[5] = vec2(0.02,0.09375);
    gaussFilter[6] = vec2(0.03,0.015625);

//    gaussFilter[0] = vec2(-3.0,0);
//    gaussFilter[1] = vec2(-2.0,0);
//    gaussFilter[2] = vec2(-1.0,0);
//    gaussFilter[3] = vec2(0.0,1.0);
//    gaussFilter[4] = vec2(1.0,0);
//    gaussFilter[5] = vec2(2.0,0);
//    gaussFilter[6] = vec2(3.0,0);

    vec4 color = vec4(0.0, 0.0, 0.0, 0.0);
    for( int i = 0; i < 7; i++ )
    {
    color += texture2D(fbo_texture, vec2(f_texcoord.x + gaussFilter[i].x, f_texcoord.y + gaussFilter[i].x)) * gaussFilter[i].y;
    }
    gl_FragColor = color;
}