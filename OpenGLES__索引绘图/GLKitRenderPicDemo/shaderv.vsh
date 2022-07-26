attribute vec4 position;
attribute vec4 positionColor;

uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;
varying lowp vec4 varyColor;

void main() {
    varyColor = positionColor;
    
    gl_Position = projectionMatrix * modelViewMatrix * position;;
}
