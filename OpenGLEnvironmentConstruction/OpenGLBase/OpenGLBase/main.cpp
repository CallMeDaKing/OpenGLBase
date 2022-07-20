//
//  main.cpp
//  OpenGLBase
//
//  Created by Li King on 2022/7/20.
//

#include <stdio.h>

#define GL_SILENCE_DEPRECATION   // 去除废弃api 警告

#include "GLShaderManager.h"

#include "GLTools.h"

#include <GLUT/GLUT.h>

//定义一个，着色管理器

GLShaderManager shaderManager;

//简单的批次容器，是GLTools的一个简单的容器类。

GLBatch triangleBatch;

/*

在窗口大小改变时，接收新的宽度&高度。

*/

void changeSize(int w,int h)

{

    glViewport(0, 0, w, h);

}

void RenderScene(void)

{

    //1.清除一个或者一组特定的缓存区

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

    //2.设置一组浮点数来表示红色

    GLfloat vRed[] = {1.0,0.0,0.0,1.0f};

    //传递到存储着色器，即GLT_SHADER_IDENTITY着色器，这个着色器只是使用指定颜色以默认笛卡尔坐标第在屏幕上渲染几何图形

    shaderManager.UseStockShader(GLT_SHADER_IDENTITY,vRed);

    //提交着色器

    triangleBatch.Draw();

    //将后台缓冲区进行渲染，然后结束后交换给前台

    glutSwapBuffers();

}

void setupRC()

{

    //设置清屏颜色（背景颜色）

    glClearColor(0.98f, 0.40f, 0.7f, 1);

    //没有着色器，在OpenGL 核心框架中是无法进行任何渲染的。初始化一个渲染管理器。

    //在前面的课程，我们会采用固管线渲染，后面会学着用OpenGL着色语言来写着色器

    shaderManager.InitializeStockShaders();

    //指定顶点

    //在OpenGL中，三角形是一种基本的3D图元绘图原素。

    GLfloat vVerts[] = {

        -0.5f,0.0f,0.0f,

        0.5f,0.0f,0.0f,

        0.0f,0.5f,0.0f

    };

    triangleBatch.Begin(GL_TRIANGLES, 3);

    triangleBatch.CopyVertexData3f(vVerts);

    triangleBatch.End();

}

int main(int argc,char *argv[])

{

    //初始化GLUT库,这个函数只是传说命令参数并且初始化glut库

    glutInit(&argc, argv);

    /*

    初始化双缓冲窗口，

    */

    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA|GLUT_DEPTH|GLUT_STENCIL);

    //GLUT窗口大小、窗口标题

    glutInitWindowSize(800, 600);

    glutCreateWindow("Triangle");

    /*

    GLUT 内部运行一个本地消息循环，拦截适当的消息。然后调用我们不同时间注册的回调函数。

    */

    //注册重塑函数

    glutReshapeFunc(changeSize);

    //注册显示函数

    glutDisplayFunc(RenderScene);

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
