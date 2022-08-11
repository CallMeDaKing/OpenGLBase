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

/*
    Mac 系统下，  #include <GLUT/GLUT.h>
    Windows 和 Linux 使用freeglut 的静态库版本 并且需要添加一个宏
 */
#include <GLUT/GLUT.h>

//定义一个，着色管理器

GLShaderManager shaderManager;

//简单的批次容器，是GLTools的一个简单的容器类。  三角形类批次类  OpenGLES 不适用该类  OpenGL代码仅做了解，真正做开发 是OpenGL ES

GLBatch squareBatch;
GLBatch greenBatch;
GLBatch redBatch;
GLBatch blueBatch;
GLBatch blackBatch;

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

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    
    // 开启混合  混合的只是图层
    glEnable(GL_BLEND);
    // 开启组合函数，计算混合银子   混合的是颜色
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // 定义四种颜色
    GLfloat vRed[] = {1.0f, 0.0f, 0.0f, 1.0f};
    GLfloat vRed1[] = { 1.0f, 0.0f, 0.0f, 0.5f };
    GLfloat vGreen[] = { 0.0f, 1.0f, 0.0f, 0.5f };
    GLfloat vBlue[] = { 0.0f, 0.0f, 1.0f, 0.5f };
    GLfloat vBlack[] = { 0.0f, 0.0f, 0.0f, 0.5f };
    
    shaderManager.UseStockShader(GLT_SHADER_IDENTITY, vRed);
    squareBatch.Draw();
    
    shaderManager.UseStockShader(GLT_SHADER_IDENTITY, vGreen);
    greenBatch.Draw();
    
    shaderManager.UseStockShader(GLT_SHADER_IDENTITY, vRed1);
    redBatch.Draw();
    
    shaderManager.UseStockShader(GLT_SHADER_IDENTITY, vBlue);
    blueBatch.Draw();
    
    shaderManager.UseStockShader(GLT_SHADER_IDENTITY, vBlack);
    blackBatch.Draw();
    
    // 渲染结束关闭混合
    glDisable(GL_BLEND);
    
    glutSwapBuffers();

}

/*
    手动仅调用一次
    处理业务：  1、 设置窗口背景颜色  2、初始化存储着色器 shaderManager  3、 设置图形顶点数据  4、 利用GLBatch 三角形批次类 将数据传递到着色器
 */
void setupRC() {

    //设置清屏颜色（背景颜色）
    glClearColor(0.98f, 0.40f, 0.7f, 1);

    //没有着色器，在OpenGL 核心框架中是无法进行任何渲染的。初始化一个渲染管理器。
    // 初始化固定着色器  采用固管线渲染，后面会学着用OpenGL着色语言来写着色器
    shaderManager.InitializeStockShaders();

    //指定顶点 图元链接方式
    squareBatch.Begin(GL_TRIANGLE_FAN, 4);
    squareBatch.CopyVertexData3f(vVerts);
    squareBatch.End();
    
    // 绘制四个矩形
    GLfloat vBlock[] = { 0.25f, 0.25f, 0.0f,
        0.75f, 0.25f, 0.0f,
        0.75f, 0.75f, 0.0f,
        0.25f, 0.75f, 0.0f};
    
    greenBatch.Begin(GL_TRIANGLE_FAN, 4);
    greenBatch.CopyVertexData3f(vBlock);
    greenBatch.End();
    
    GLfloat vBlock2[] = { -0.75f, 0.25f, 0.0f,
        -0.25f, 0.25f, 0.0f,
        -0.25f, 0.75f, 0.0f,
        -0.75f, 0.75f, 0.0f};
    
    redBatch.Begin(GL_TRIANGLE_FAN, 4);
    redBatch.CopyVertexData3f(vBlock2);
    redBatch.End();

    GLfloat vBlock3[] = { -0.75f, -0.75f, 0.0f,
        -0.25f, -0.75f, 0.0f,
        -0.25f, -0.25f, 0.0f,
        -0.75f, -0.25f, 0.0f};
    
    blueBatch.Begin(GL_TRIANGLE_FAN, 4);
    blueBatch.CopyVertexData3f(vBlock3);
    blueBatch.End();
    
    GLfloat vBlock4[] = { 0.25f, -0.75f, 0.0f,
        0.75f, -0.75f, 0.0f,
        0.75f, -0.25f, 0.0f,
        0.25f, -0.25f, 0.0f};
    
    blackBatch.Begin(GL_TRIANGLE_FAN, 4);
    blackBatch.CopyVertexData3f(vBlock4);
    blackBatch.End();

}

void SpecialKeys(int key, int x, int y) {
    
    GLfloat stepSize = 0.025f;
    
    GLfloat blockX = vVerts[0];
    GLfloat blockY = vVerts[10];
    
    if (key == GLUT_KEY_UP) {
        blockY += stepSize;
    }

    if (key == GLUT_KEY_DOWN) {
        blockY -= stepSize;
    }

    if (key == GLUT_KEY_LEFT) {
        blockX -= stepSize;
    }

    if (key == GLUT_KEY_RIGHT) {
        blockX += stepSize;
    }

    // 边界处理
    if (blockX < -1.0f) {
        blockX = -1.0f;
    }

    if (blockX > 1.0 - blockSize * 2) {
        blockX = 1.0f - blockSize * 2;
    }

    if (blockY < -1.0f + blockSize * 2) {
        blockY = -1.0f + blockSize * 2;
    }

    if (blockY > 1.0f) {
        blockY = 1.0f;
    }

    vVerts[0] = blockX;
    vVerts[1] = blockY - blockSize * 2;

    vVerts[3] = blockX + blockSize*2;
    vVerts[4] = blockY - blockSize*2;

    vVerts[6] = blockX + blockSize*2;
    vVerts[7] = blockY;

    vVerts[9] = blockX;
    vVerts[10] = blockY;

    squareBatch.CopyVertexData3f(vVerts);

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

    glutCreateWindow("移动矩形，观察混合颜色");

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
