precisition highp float;

varying lowp vec2 varyTexCoord;
uniform sampler2D colorMap;

void main() {
    lowp vec4 temp = texture2D(colorMap, varyTexCoord);
    gl_FragColor = temp;
}
