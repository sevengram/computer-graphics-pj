attribute vec2 coord;
varying vec2 texcoord;

void main(void) {
    gl_Position = vec4(coord, 0.0, 1.0);
    texcoord = (coord + 1.0) / 2.0;
}
