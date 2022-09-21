/*
 3 种修饰类型
 uniform , attribute, varying
 
 1、uniform -> （OC Swift）传递vertex fragment 变量到着色器， 用于不常变化的变量
 对应 glUniform 方法 多种API
 uniform 用于视图矩阵， 投影矩阵， 投影视图矩阵
 
 uniform： 传递方式
 viewProMatrix： 参数名称
 mat4: 数据类型
 uniform mat4 viewProMatrix;
 
 2、attribute: 只能从客户端往顶点着色器 也只能在顶点着色器使用
 修饰顶点 纹理坐标 颜色 法线
 对应API glVertex...
 
 attribute：修饰符
 vec4: 类型向量  
 attribute vec4 position;
 attribure vec4 color;
 attribute vec2 texCoord;
 
 attribute 无法直接传递纹理坐标到偏远着色器的 所以需要先传递到顶点着色器，再桥接到片元着色器
 需要使用varying
 3、 varying
 将attribute传递进来的纹理坐标通过varying属性 桥接到片元着色器。 在片元着色器需要声明一个完全相同的该修饰属性的变量，用于接受改属性的值
 
 4. shaderf 中 //precisition highp float; 属性打开会导致无法绘制吗，目前还未知具体原因
 https://stackoverflow.com/questions/31997762/gpuimage-shader-crashing-with-error-one-or-more-attached-shaders-not-successfu
 */


