//（正常不要使用中文注释） 编译需删除中文注释
attribute vec4 position;
attribute vec2 textCoordinate;
varying lowp vec2 varyTexCoord;       // lowp 低精度 将attribute传递进来的纹理坐标通过varying 桥接到片元着色器。 在片元着色器需要声明一个完全相同的属性用于接受改属性的值
 
void main() {
    varyTexCoord = textCoordinate; // 片元着色器需要声明一个完全相同的属性用于接受改属性的值
    
    // 内建变量 gl_Position 接收顶点着色器计算后的数据
    gl_Position = position;
}
