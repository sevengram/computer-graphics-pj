uniform sampler2D colorTexture;
uniform sampler2D depthTexture;
varying vec2 texcoord;

uniform float width;
uniform float height;

uniform float focusDistance;  //focal distance value in meters
uniform float focalLength; //focal length in mm
uniform float fstop; //f-stop value
uniform int showFocus;
uniform int autoFocus;

#define PI  3.14159265

float znear = 0.1; //camera clipping start
float zfar = 2000.0; //camera clipping end
int samples = 4; //samples on the first ring
int rings = 4; //ring count
float CoC = 0.003;
vec2 autoFocusPoint = vec2(0.5,0.5);
float maxblur = 1.8;
vec2 texel = vec2(1.0/width,1.0/height);

float blurDepth(vec2 coords)
{
    float dbsize = 1.25;
    float d = 0.0;
    float kernel[9];
    vec2 offset[9];
    vec2 p = vec2(texel.x, texel.y) * dbsize;

    offset[0] = vec2(-p.x,-p.y);
    offset[1] = vec2( 0.0,-p.y);
    offset[2] = vec2( p.x,-p.y);
    offset[3] = vec2(-p.x, 0.0);
    offset[4] = vec2( 0.0, 0.0);
    offset[5] = vec2( p.x, 0.0);
    offset[6] = vec2(-p.x, p.y);
    offset[7] = vec2( 0.0, p.y);
    offset[8] = vec2( p.x, p.y);

    kernel[0] = 1.0/16.0;
    kernel[1] = 2.0/16.0;
    kernel[2] = 1.0/16.0;
    kernel[3] = 2.0/16.0;
    kernel[4] = 4.0/16.0;
    kernel[5] = 2.0/16.0;
    kernel[6] = 1.0/16.0;
    kernel[7] = 2.0/16.0;
    kernel[8] = 1.0/16.0;

    for(int i = 0; i < 9; i++){
        float tmp = texture2D(depthTexture, coords + offset[i]).r;
        d += tmp * kernel[i];
    }
    return d;
}

vec3 color(vec2 coords,float blur)
{
    return texture2D(colorTexture, coords).rgb;
}

vec3 showFocusZones(vec3 color, float blur, float depth)
{
    if (depth < 1000){
        float edge = 0.002*depth;
        float m = clamp(smoothstep(0.0, edge, blur), 0.0, 1.0);
        float e = clamp(smoothstep(1.0-edge, 1.0, blur), 0.0, 1.0);
        color = mix(color,vec3(1.0,0.5,0.0),(1.0-m)*0.6);
        color = mix(color,vec3(0.0,0.6,1.0),(m-e)*0.2);
    }
    return color;
}

float linearize(float depth)
{
    return -zfar * znear / (depth * (zfar - znear) - zfar);
}

void main()
{
    // scene depth calculation
    float depth = linearize(texture2D(depthTexture,texcoord).x);

    // blur the depth
    depth = linearize(blurDepth(texcoord));

    float dist = focusDistance;
    if (autoFocus == 1) {
        dist = linearize(texture2D(depthTexture, autoFocusPoint).x);
    }

    //dof blur factor calculation
    float f = focalLength; //focal length in mm
    float d = dist*1000.0; //focal plane in mm
    float o = depth*1000.0; //depth in mm

    float a = (o*f)/(o-f);
    float b = (d*f)/(d-f);
    float c = (d-f)/(d*fstop*CoC);

    float blur = clamp(abs(a-b)*c,0.0,1.0);

    // getting blur x and y step factor
    float w = (1.0/width)*blur*maxblur;
    float h = (1.0/height)*blur*maxblur;

    vec3 color = vec3(0.0);
    if(blur < 0.05) {
        color = texture2D(colorTexture, texcoord).rgb;
    } else {
        color = texture2D(colorTexture, texcoord).rgb;
        float s = 1.0;
        int ringsamples;
        for (int i = 1; i <= rings; i += 1){
            ringsamples = i * samples;
            for (int j = 0 ; j < ringsamples ; j += 1) {
                float step = PI*2.0 / float(ringsamples);
                float pw = (cos(float(j)*step)*float(i));
                float ph = (sin(float(j)*step)*float(i));
                float p = 1.0;
                color += color(texcoord + vec2(pw*w,ph*h),blur)*mix(1.0,(float(i))/(float(rings)),0.5)*p;
                s += 1.0*mix(1.0,(float(i))/(float(rings)),0.5)*p;
            }
        }
        color /= s;
    }
    if (showFocus == 1) {
        color = showFocusZones(color, blur, depth);
    }
    gl_FragColor.rgb = color;
    gl_FragColor.a = 1.0;
}
