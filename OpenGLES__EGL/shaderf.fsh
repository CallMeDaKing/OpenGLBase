// 定义精度  （正常不要使用中文注释） 编译需删除中文注释
precisition highp float;      //  float  默认使用高精度

// 纹理坐标
varying lowp vec2 varyTexCoord;       // lowp 低精度
// 纹理  sampler2D类型
uniform sampler2D colorMap;

void main() {
    // 1. 拿纹理对应坐标下的纹素
    // 纹理对应像素点的颜色值； texture2D(纹理 纹理坐标)  返回值颜色值
    lowp vec4 temp = texture2D(colorMap, varyTexCoord);
    
    // 2 透明度修改  ==> 30%
    
    // 内建变量 gl_FragColor 用于存储片元着色器结果的属性
    gl_FragColor = temp;
    
    
}
