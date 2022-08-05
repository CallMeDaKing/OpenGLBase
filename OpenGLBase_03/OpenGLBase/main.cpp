//
//  main.cpp
//  OpenGLBase
//
//  Created by Li King on 2022/7/20.
//

#include <stdio.h>

#define GL_SILENCE_DEPRECATION   // 去除废弃api 警告

#include "GLShaderManager.h"     // 管理固定着色器， 如果没有着色器则无法在OpenGL核心框架进行着色， 两个功能： 1、创建并管理着色器 2、提供一组存储着色器，进行初步的渲染操作

#include "GLTools.h"             // GLTool.h 包含了大部分类似C语言的独立函数

/*  GLMatrixStack 通过调用顶部载入这个单位矩阵
    void GLMatrixStack::LoadIndentiy(void);

    //在堆栈顶部载入任何矩阵
    void GLMatrixStack::LoadMatrix(const M3DMatrix44f m);
*/
#include "GLMatrixStack.h"       // 矩阵工具类，使用GLMatrixStack加载单元矩阵/矩阵/矩阵相乘/压栈/出栈/缩放/平移/旋转， 这个矩阵初始化时默认包含了单位矩阵 ,默认堆栈深度为64，

#include "GLFrame.h"             // 矩阵工具类，使用GLFrame表示位置，设置Origin,vForward,vUp

#include "GLFrustum.h"           // 矩阵工具类，用来快速设置正投影和透视投影矩阵，完成坐标3D->2D 映射

#include "GLBatch.h"             // 三角形批次类，使用GLBatch传输顶点、光照、纹理、颜色数据到存储着色器

#include "GLGeometryTransform.h" // 变换管道类，用于快速在代码中传输视图矩阵、投影矩阵、视图投影变化矩阵等

#include <math.h>               // 内置数据库

#ifdef  __APPLE__
/*
    Mac 系统下，  #include <GLUT/GLUT.h>
    Windows 和 Linux 使用freeglut 的静态库版本 并且需要添加一个宏
 */
#include <GLUT/GLUT.h>
#else
#define FREEGLUT_STATIC
#include <GL/glut.h>
#endif


//定义一个，着色管理器
GLShaderManager         shaderManager;
GLMatrixStack           modelViewMatrix;      // 模型视图矩阵堆栈
GLMatrixStack           projectionMatrix;     // 投影矩阵堆栈
GLFrame                 cameraFrame;          // 观察者视图坐标
GLFrame                 objectFrame;          // 当前对象的视图坐标
GLFrustum               viewFrustum;          // 投影矩阵，设置图元绘制时的投影方式

// 容器类 (7 中不同容器对象， 每个容器类对应一个对象)
GLBatch     pointBatch;
GLBatch     lineBatch;
GLBatch     lineStripBatch;
GLBatch     lineLoopBatch;
GLBatch     triangleBatch;
GLBatch     triangleStripBatch;
GLBatch     triangleFanBatch;

// 变换管道
GLGeometryTransform transformPipline;

GLfloat vGreen[] = {0.0f, 1.0f, 0.0f, 1.0f};
GLfloat vBlack[] = {0.0f, 0.0f, 0.0f, 1.0f};

// 跟踪效果步骤
int nStep = 0;


/*

在窗口大小改变时，接收新的宽度&高度。

*/

// 正方形顶点数据
GLfloat blockSize = 0.1f;
GLfloat vVerts[] {
    -blockSize, -blockSize, 0.0f,
    blockSize,  -blockSize, 0.0f,
    blockSize,  blockSize, 0.0f,
    -blockSize, blockSize, 0.0f,
};

GLfloat xPos = 0.0f;
GLfloat yPos = 0.0f;

void changeSize(int w,int h) {
    
    glViewport(0, 0, w, h);
    
}

// 当屏幕进行刷新的时候调用多次，系统在刷新的时候主动调用 比如60帧 相当于每秒刷新60次， 调用60 次
void RenderScene(void) {

    //MARK: 普通实现方法
//    //1.清除一个或者一组特定的缓存区  该方法会作为屏幕刷新事件多次触发，所以每次绘制前需要清空下该缓存区
//    /*
//        OpenGL 中有多重缓存去 颜色缓存区GL_COLOR_BUFFER_BIT、 深度缓存区GL_DEPTH_BUFFER_BIT、 模型缓存区 GL_STENCIL_BUFFER_BIT
//        缓存区： 是用来存储图像信息的存储空间， RGBA分量通常一起作为颜色缓存区或者像素缓存区引用
//
//     */
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
//
//    //2.设置一组浮点数来表示红色  配置颜色
//
//    GLfloat vRed[] = {1.0,0.0,0.0,1.0f};
//
//    //传递到存储着色器，即GLT_SHADER_IDENTITY 固定单元着色器着色器，这个着色器只是使用指定颜色以默认笛卡尔坐标第在屏幕上渲染几何图形
//    shaderManager.UseStockShader(GLT_SHADER_IDENTITY,vRed);
//
//    //提交着色器
//    triangleBatch.Draw();
//
//    // 将后台缓冲区进行渲染，然后结束后交换给前台  （双缓存区， 将绘制好的buffer 显示）
//    glutSwapBuffers();
    
    
    //MARK: 方法2  使用矩阵实现
    // 清除上一次缓存
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    
    GLfloat vRed[] = {1.0f,0.0f,0.0f,0.0f};

    // 声明 平移旋转 和最终矩阵
    M3DMatrix44f mFinalTransform, mTransformMatrix, mRotationMatrix;
    // 平移
    m3dTranslationMatrix44(mTransformMatrix, xPos, yPos, 0.0f);
    
    // 每次平移时旋转 5°
    static float yRot = 0.0f;
    yRot += 5.0f;
    m3dRotationMatrix44(mRotationMatrix, m3dDegToRad(yRot), 0.0f, 0.0f, 1.0f);
    
    // 将平移和旋转的矩阵合并到 mFinalTransform
    m3dMatrixMultiply44(mFinalTransform, mTransformMatrix, mRotationMatrix);
    
    // 将矩阵结果给 平面着色器 中绘制
    shaderManager.UseStockShader(GLT_SHADER_FLAT, mFinalTransform, vRed);
    
    // 提交绘制
    triangleBatch.Draw();
    
    glutSwapBuffers();

}

/*
    手动仅调用一次
    处理业务：  1、 设置窗口背景颜色  2、初始化存储着色器 shaderManager  3、 设置图形顶点数据  4、 利用GLBatch 三角形批次类 将数据传递到着色器
 */
void setupRC() {

    //设置清屏颜色（背景颜色 灰色）
    glClearColor(0.7f, 0.7f, 0.7f, 1.0f );
    shaderManager.InitializeStockShaders()
    glEnable(
    
    

}

void SpecialKeys(int key, int x, int y) {
    
    GLfloat stepSize = 0.025f;
    
//    GLfloat blockX = vVerts[0];
//    GLfloat blockY = vVerts[10];
    
//    if (key == GLUT_KEY_UP) {
//        blockY += stepSize;
//    }
//
//    if (key == GLUT_KEY_DOWN) {
//        blockY -= stepSize;
//    }
//
//    if (key == GLUT_KEY_LEFT) {
//        blockX -= stepSize;
//    }
//
//    if (key == GLUT_KEY_RIGHT) {
//        blockX += stepSize;
//    }
//
//    // 边界处理
//    if (blockX < -1.0f) {
//        blockX = -1.0f;
//    }
//
//    if (blockX > 1.0 - blockSize * 2) {
//        blockX = 1.0f - blockSize * 2;
//    }
//
//    if (blockY < -1.0f + blockSize * 2) {
//        blockY = -1.0f + blockSize * 2;
//    }
//
//    if (blockY > 1.0f) {
//        blockY = 1.0f;
//    }
//
//    vVerts[0] = blockX;
//    vVerts[1] = blockY - blockSize * 2;
//
//    vVerts[3] = blockX + blockSize*2;
//    vVerts[4] = blockY - blockSize*2;
//
//    vVerts[6] = blockX + blockSize*2;
//    vVerts[7] = blockY;
//
//    vVerts[9] = blockX;
//    vVerts[10] = blockY;
//
//    triangleBatch.CopyVertexData3f(vVerts);
    
    // MARK: 使用矩阵 关注中间变化量
    if (key == GLUT_KEY_UP) {
        
        yPos += stepSize;
    }
    
    if (key == GLUT_KEY_DOWN) {
        yPos -= stepSize;
    }
    
    if (key == GLUT_KEY_LEFT) {
        xPos -= stepSize;
    }
    
    if (key == GLUT_KEY_RIGHT) {
        xPos += stepSize;
    }
    
    //碰撞检测
    if (xPos < (-1.0f + blockSize)) {
        
        xPos = -1.0f + blockSize;
    }
    
    if (xPos > (1.0f - blockSize)) {
        xPos = 1.0f - blockSize;
    }
    
    if (yPos < (-1.0f + blockSize)) {
        yPos = -1.0f + blockSize;
    }
    
    if (yPos > (1.0f - blockSize)) {
        yPos = 1.0f - blockSize;
    }
    // 强制渲染
    glutPostRedisplay();
    
}


int main(int argc,char *argv[])  {
    
    //设置当前工作目录，针对MAC OS X
    /*
     `GLTools`函数`glSetWorkingDrectory`用来设置当前工作目录。实际上在Windows中是不必要的，因为工作目录默认就是与程序可执行执行程序相同的目录。但是在Mac OS X中，这个程序将当前工作文件夹改为应用程序捆绑包中的`/Resource`文件夹。`GLUT`的优先设定自动进行了这个中设置，但是这样中方法更加安全。
     */
    gltSetWorkingDirectory(argv[0]);
    
    //初始化GLUT库,这个函数只是传说命令参数并且初始化glut库

    glutInit(&argc, argv);

    /*

     初始一个双缓冲窗口，配置各种模板 GLUT_DOUBLE、GLUT_RGBA GLUT_DEPTH  GLUT_STENCIL
     GLUT_DOUBLE: 双缓存窗口， 绘图实际是离屏缓存区执行，然后切成窗口视图，经常用来生成动画效果
     GLUT_DEPTH : 标志将一个深度缓存区分配为显示的一部分， 执行深度测试（将当前（观察者角度看到的物体）片段，和深度缓冲区深度只比较，符合则测通过，然后将当前片段的值替换深度缓冲区的值，如果测试不通过，丢弃当前片段）。
        为什么要深度测试？： 实现场景中物体遮挡效果 避免出现物体显示不全或者穿模效果。
     GLUT_STENCIL: 深度模板
    */

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_STENCIL);

    //GLUT窗口大小、窗口标题

    glutInitWindowSize(600, 600);

    glutCreateWindow("Square");

    /*

    GLUT 内部运行一个本地消息循环 ，glutMainLoop，拦截适当的消息。然后调用我们不同时间注册的回调函数。

    */

    //注册重塑函数
    glutReshapeFunc(changeSize);

    //注册显示函数
    glutDisplayFunc(RenderScene);
    

    // 获取键盘事件
    glutSpecialFunc(SpecialKeys);

    /*

    初始化一个GLEW库,确保OpenGL API对程序完全可用。

    在试图做任何渲染之前，要检查确定驱动程序的初始化过程中没有任何问题

    */

    GLenum status = glewInit();

    if (GLEW_OK != status) {

        printf("GLEW Error:%s\n",glewGetErrorString(status));

        return 1;

    }

    //设置我们的渲染环境

    setupRC();

    glutMainLoop();

    return  0;
}
