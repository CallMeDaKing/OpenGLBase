attribute vec4 position;
attribute vec2 textCoordinate;
varying lowp vec2 varyTexCoord;      
 
void main() {
    varyTexCoord = textCoordinate;
    gl_Position = position;
}
