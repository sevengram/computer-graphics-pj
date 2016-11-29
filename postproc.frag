uniform sampler2D color_texture;
uniform sampler2D depth_texture;
varying vec2 f_texcoord;

#define PI  3.14159265

float width = 800;
float height = 600;

vec2 texel = vec2(1.0/width,1.0/height);

uniform float focalDepth;  //focal distance

float znear = 5.5/16.0; //camera znear
float zfar = 5.5*16.0; //camera zfar

int samples = 3; //samples on the first ring
int rings = 3; //ring count

float ndofstart = 8.0; //near dof blur start
float ndofdist = 10.0; //near dof blur falloff distance
float fdofstart = 8.0; //far dof blur start
float fdofdist = 10.0; //far dof blur falloff distance

float CoC = 0.03;//circle of confusion

float maxblur = 2.0;

float bias = 0.5; //bokeh edge bias
float fringe = 0.7; //bokeh chromatic aberration/fringing

float linearize(float depth)
{
    return -zfar * znear / (depth * (zfar - znear) - zfar);
}

vec3 color(vec2 coords,float blur) //processing the sample
{
    vec3 col = vec3(0.0);

    col.r = texture2D(color_texture, coords + vec2(0.0,1.0)*texel*fringe*blur).r;
    col.g = texture2D(color_texture, coords + vec2(-0.866,-0.5)*texel*fringe*blur).g;
    col.b = texture2D(color_texture, coords + vec2(0.866,-0.5)*texel*fringe*blur).b;

    return col;
}

void main()
{
    float depth = linearize(texture2D(depth_texture,f_texcoord).x);
    float fDepth = focalDepth;

    float blur = 0.0;
    float a = depth-fDepth; //focal plane
    float b = (a-fdofstart)/fdofdist; //far DoF
    float c = (-a-ndofstart)/ndofdist; //near Dof
    blur = (a>0.0)?b:c;

    blur = clamp(blur,0.0,1.0);

    // getting blur x and y step factor

    float w = (1.0/width)*blur*maxblur;
    float h = (1.0/height)*blur*maxblur;

    // calculation of final color
    vec3 col = vec3(0.0);

    if(blur < 0.05){
        col = texture2D(color_texture, f_texcoord).rgb;
    } else {
        col = texture2D(color_texture, f_texcoord).rgb;
        float s = 1.0;
        int ringsamples;

        for (int i = 1; i <= rings; i += 1)
        {
            ringsamples = i * samples;

            for (int j = 0 ; j < ringsamples ; j += 1)
            {
                float step = PI*2.0 / float(ringsamples);
                float pw = (cos(float(j)*step)*float(i));
                float ph = (sin(float(j)*step)*float(i));
                float p = 1.0;
                col += color(f_texcoord + vec2(pw*w,ph*h),blur)*mix(1.0,(float(i))/(float(rings)),bias)*p;
                s += 1.0*mix(1.0,(float(i))/(float(rings)),bias)*p;
            }
        }
        col /= s; //divide by sample count
    }

    gl_FragColor.rgb = col;
    gl_FragColor.a = 1.0;
}
