uniform sampler2D color_texture;
uniform sampler2D depth_texture;
varying vec2 f_texcoord;

uniform float width;
uniform float height;

uniform float focalDepth;  //focal distance value in meters, but you may use autofocus option below
uniform float focalLength; //focal length in mm
uniform float fstop; //f-stop value
uniform bool showFocus;

#define PI  3.14159265

float znear = 0.1; //camera clipping start
float zfar = 2000.0; //camera clipping end

int samples = 4; //samples on the first ring
int rings = 4; //ring count

float CoC = 0.003;

bool autofocus = false;
vec2 focus = vec2(0.5,0.5); // autofocus point on screen (0.0,0.0 - left lower corner, 1.0,1.0 - upper right)
float maxblur = 1.8; // max blur (0.0 = no blur,1.0 default)

vec2 texel = vec2(1.0/width,1.0/height);

float blurDepth(vec2 coords) //blurring depth
{
    float dbsize = 1.25;
    float d = 0.0;
    float kernel[9];
    vec2 offset[9];

    vec2 wh = vec2(texel.x, texel.y) * dbsize;

    offset[0] = vec2(-wh.x,-wh.y);
    offset[1] = vec2( 0.0, -wh.y);
    offset[2] = vec2( wh.x -wh.y);

    offset[3] = vec2(-wh.x,  0.0);
    offset[4] = vec2( 0.0,   0.0);
    offset[5] = vec2( wh.x,  0.0);

    offset[6] = vec2(-wh.x, wh.y);
    offset[7] = vec2( 0.0,  wh.y);
    offset[8] = vec2( wh.x, wh.y);

    kernel[0] = 1.0/16.0;
    kernel[1] = 2.0/16.0;
    kernel[2] = 1.0/16.0;
    kernel[3] = 2.0/16.0;
    kernel[4] = 4.0/16.0;
    kernel[5] = 2.0/16.0;
    kernel[6] = 1.0/16.0;
    kernel[7] = 2.0/16.0;
    kernel[8] = 1.0/16.0;

    for( int i=0; i<9; i++ ){
        float tmp = texture2D(depth_texture, coords + offset[i]).r;
        d += tmp * kernel[i];
    }

    return d;
}

vec3 color(vec2 coords,float blur) //processing the sample
{
    return texture2D(color_texture, coords).rgb;
}

vec3 showFocusZones(vec3 col, float blur, float depth)
{
    float edge = 0.002*depth; //distance based edge smoothing
    float m = clamp(smoothstep(0.0, edge, blur), 0.0, 1.0);
    float e = clamp(smoothstep(1.0 - edge, 1.0, blur), 0.0, 1.0);

    col = mix(col,vec3(1.0,0.5,0.0),(1.0-m)*0.6);
    col = mix(col,vec3(0.0,0.5,1.0),((1.0-e)-(1.0-m))*0.2);

    return col;
}

float linearize(float depth)
{
    return -zfar * znear / (depth * (zfar - znear) - zfar);
}

void main()
{
    //scene depth calculation

    float depth = linearize(texture2D(depth_texture,f_texcoord).x);

    depth = linearize(blurDepth(f_texcoord));

    //focal plane calculation

    float fDepth = focalDepth;

    if (autofocus) {
        fDepth = linearize(texture2D(depth_texture,focus).x);
    }

    //dof blur factor calculation
    float f = focalLength; //focal length in mm
    float d = fDepth*1000.0; //focal plane in mm
    float o = depth*1000.0; //depth in mm

    float a = (o*f)/(o-f);
    float b = (d*f)/(d-f);
    float c = (d-f)/(d*fstop*CoC);

    float blur = abs(a-b)*c;

    blur = clamp(blur,0.0,1.0);

    // getting blur x and y step factor
    float w = (1.0/width)*blur*maxblur;
    float h = (1.0/height)*blur*maxblur;

    // calculation of final color

    vec3 col = vec3(0.0);

    if(blur < 0.05) {
        col = texture2D(color_texture, f_texcoord).rgb;
    } else {
        col = texture2D(color_texture, f_texcoord).rgb;
        float s = 1.0;
        int ringsamples;

        for (int i = 1; i <= rings; i += 1){
            ringsamples = i * samples;
            for (int j = 0 ; j < ringsamples ; j += 1)
            {
                float step = PI*2.0 / float(ringsamples);
                float pw = (cos(float(j)*step)*float(i));
                float ph = (sin(float(j)*step)*float(i));
                float p = 1.0;
                col += color(f_texcoord + vec2(pw*w,ph*h),blur)*mix(1.0,(float(i))/(float(rings)),0.5)*p;
                s += 1.0*mix(1.0,(float(i))/(float(rings)),0.5)*p;
            }
        }
        col /= s;
    }

    if (true) {
        col = showFocusZones(col, blur, depth);
    }

    gl_FragColor.rgb = col;
    gl_FragColor.a = 1.0;
}
