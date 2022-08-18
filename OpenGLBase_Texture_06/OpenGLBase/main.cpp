//
//  main.cpp
//  OpenGLBase
//
//  Created by Li King on 2022/7/20.
//

#include <stdio.h>

#define GL_SILENCE_DEPRECATION   // 去除废弃api 警告

#include "GLTools.h"             // GLTool.h 包含了大部分类似C语言的独立函数

#include "GLShaderManager.h"     // 管理固定着色器管理类， 如果没有着色器则无法在OpenGL核心框架进行着色， 两个功能： 1、创建并管理着色器 2、提供一组存储着色器，进行初步的渲染操作

/*  GLMatrixStack 通过调用顶部载入这个单位矩阵
    void GLMatrixStack::LoadIndentiy(void);

    //在堆栈顶部载入任何矩阵
    void GLMatrixStack::LoadMatrix(const M3DMatrix44f m);
*/
#include "GLMatrixStack.h"       // 矩阵工具类，矩阵堆栈，使用GLMatrixStack 压栈/出栈操作，加载单元矩阵/矩阵相乘//缩放/平移/旋转， 这个矩阵初始化时默认包含了单位矩阵 ,默认堆栈深度为64，

#include "GLFrame.h"             // 矩阵工具类，使用GLFrame表示位置，设置Origin,vForward,vUp

#include "GLFrustum.h"           // 矩阵工具类，用来快速设置正投影和透视投影矩阵，完成坐标3D->2D 映射

#include "GLBatch.h"             // 三角形批次类，使用GLBatch传输顶点、光照、纹理、颜色数据到存储着色器

#include "GLGeometryTransform.h" // 变换管道类，用于快速在代码中传输视图矩阵、投影矩阵、视图投影变化矩阵等

#include <math.h>               // 内置数学库

#include "StopWatch.h"

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

#define NUM_SPHERES 50


//定义一个，着色管理器管理类
GLShaderManager         shaderManager;
GLMatrixStack           modelViewMatrix;      // 模型视图矩阵堆栈 管理模型矩阵变化
GLMatrixStack           projectionMatrix;     // 投影矩阵堆栈 管理投影矩阵变化
GLFrame                 cameraFrame;          // 记录观察者（矩阵） 变化 上下左右移动
GLFrame                 objectFrame;          // 记录物体（矩阵）变化 操作当前对象的变化的矩阵， 上下左右移动。
GLFrustum               viewFrustum;          // 投影矩阵，设置图元绘制时的投影方式  透视投影 perspective  正投影 orthographics

/*
    GLFrame 叫参考帧， 存储了世界1个坐标系 2个世界坐标系下的方向向量  也就是9个glfloat值分别用来表示： 当前位置、向前方向向量、向上方向向量
            用于表示世界坐标系中的任意物质的位置和方向， 无论是相机还是物体模型，都可以使用GLFrame表示
    
    一般来说针对物体本身的坐标系，X轴永远平行于视口的水平方向， +X 的方向 根据右手准则结合 +Y +Z推导。
    先确定Y轴和Z轴方向，Y轴永远平行于视口的垂直方向，竖直向上为+Y,Z轴永远平行于视口的垂直直面向里的方向，正前方为+Z
    也就是在世界坐标系中，Y方向代表了向上商量， Z方向代表了向前的向量，
    根据右手定则，结合+Y 、+Z 推导出+X是水平向左的。
 
    GLFrame 默认构造函数，会将物体初始化位置为 （0,0，0）点， up(向上向量)为 （0,1，0）， forward为（0，0，-1）朝向-Z, 也就是初始化后的物体坐标系X Z 和                 世界坐标系是相反的，默认适合相机的坐标系，但是不适合物体模型，如果要建立模型，需要将坐标系设置为和世界坐标系相同，即model.SetForwardVector(0.0f,0.0f,1.0f);
        
    根据相机的自身坐标系， 使用右手定则可以知道， +x 是从原点向左，所以X变大 相机是往左走，同样物体是往右移动的
    
    重点： 相机形变 一定要使用  ApplyCameraTransform  物体形变一定要使用 ApplyTransform ，否则坐标计算是反的
 
    在OpenGL 世界中，Y 是指向天空的UP，XOZ 构成地面， Z 是Forward
 
    总结: 实际上GLFrame 是一系列变化，有GLFrame可以导出变换矩阵，只要与该变换矩阵相乘，任何物体都可以进行GLFrame相应的变化。 比如两个物体AB， 经过A 的GLFrame 导出变换矩阵，让B乘以变换矩阵，本来B 的坐标系是相对于世界坐标系的， 现在变变为了相对于A的坐标系。
 */

GLBatch         pyramidBatch;
GLuint          textTureID;

// 变换管道
GLGeometryTransform transformPipline;

// 当屏幕进行刷新的时候调用多次，系统在刷新的时候主动调用 比如60帧 相当于每秒刷新60次， 调用60 次
void RenderScene(void) {
    
    // 清理缓存
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    //当前模型视图压栈
    modelViewMatrix.PushMatrix();
    
    // 添加相机矩阵
    M3DMatrix44f camera;
    // 从camera 中获取一个4x4 矩阵
    cameraFrame.GetCameraMatrix(camera);
    // 将相机的矩阵压入栈顶
    modelViewMatrix.MultMatrix(camera);
    
    // 添加物体矩阵
    M3DMatrix44f object;
    objectFrame.GetMatrix(object);
    modelViewMatrix.MultMatrix(object);
    
    // 绑定纹理  当项目中有多个纹理时，指定对应纹理很关键
    glBindTexture(GL_TEXTURE_2D, textTureID);
    
    // 使用纹理替换着色器
    /*
     参数1： GLT_SHADER_TEXTURE_REPLACE(着色器标签)
     参数2： 模型视图矩阵
     参数3： 纹理层
     */
    shaderManager.UseStockShader(GLT_SHADER_TEXTURE_REPLACE, transformPipline.GetModelViewProjectionMatrix(), 0);
    pyramidBatch.Draw();
    
    modelViewMatrix.PopMatrix();
    
    glutSwapBuffers();
}

// 两个作用， 1.设置视图大小 2.设置投影矩阵
void changeSize(int w,int h) {
    // 设置视窗大小
    glViewport(0, 0, w, h);
    
    // 2. 设置投影矩阵
    viewFrustum.SetPerspective(35.0f, float(w)/float(h), 1, 500.0f);
    projectionMatrix.LoadMatrix(viewFrustum.GetProjectionMatrix());
    
    // 3.
    transformPipline.SetMatrixStacks(modelViewMatrix, projectionMatrix);
}

bool LoadTGATexture(const char *zfileName, GLenum minFilter, GLenum magFilter, GLenum wrapMode){
    GLbyte *pBites;
    int nWidth, nHeight, nComponents;    // nComponents 像素组成 RGBA RGB ...
    GLenum eFormat;  // 像素数据类型， 常见的无符号整形 GL_UNSIGNED_BYTE
    
    /*
    《1》读取纹理， 读取像素
        参数1 纹理文件名称
        参数2 文件宽度地址
        参数3 文件高度地址
        参数4 文件组件地址
        参数5 文件格式地址
        返回值： pBits 指向图像数据的指针
     */
    pBites = gltReadTGABits(zfileName, &nWidth, &nHeight, &nComponents, &eFormat);
    if (pBites == NULL)
        return false;
    
    /*
    《2》设置纹理参数
        参数1： 纹理维度  1D 2D 3D  默认使用 GL_TEXTURE_2D
        参数2： 为ST 坐标设置模式
        参数3： wrapMode 环绕模式
    */
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
    
    /*
        设置放大缩小的填充模式
     */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
    
    /*
     《3》 载入纹理
        参数1： 纹理维度 1D 2D 3D
        参数2： mip 贴图层次 默认0
        参数3： 纹理单元存储的颜色成分 （从读取像素图获取）
        参数4： 加载纹理宽
        参数5:  加载纹理高
        参数6： 加载纹理深度
        参数7： 像素数据的数据类型  GL_UNSIGNED_BYTE, 每个颜色分量是一个8位无符号整数
        参数8： 指向纹理图像数据的指针
     */
    
    glTexImage2D(GL_TEXTURE_2D, 0, nComponents, nWidth, nHeight, 0, eFormat, GL_UNSIGNED_BYTE, pBites);
    
    // 使用完pBits 释放
    free(pBites);
    
    if (minFilter == GL_LINEAR_MIPMAP_LINEAR  ||
        minFilter == GL_LINEAR_MIPMAP_NEAREST ||
        minFilter == GL_NEAREST_MIPMAP_LINEAR ||
        minFilter == GL_NEAREST_MIPMAP_NEAREST) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    
    return true;
}

// 清理…例如删除纹理对象
void ShutdownRC(void)
{
    glDeleteTextures(1, &textTureID);
}

void MakePyramid(GLBatch& pyramidBatch) {
    /*
        1. 通过pyramidBatch 组件三角形批次类
     参数1 ：类型
     参数2 ：顶点数
     参数3 ：这个批次中将会应用1个纹理 如果不写默认为0
     */
    pyramidBatch.Begin(GL_TRIANGLES, 18, 1);
    
    /*
        设置纹理坐标
        void MultiTexCoord2f(GLuint textture,GLclampf s, GLclampf t);
        参数1: texture, 纹理层次， 对于使用存储着色器来进行渲染，设置为0
        参数2: s 对应顶点坐标中的x坐标
        参数3: t 对应顶掉坐标中的y坐标
        strq 对应顶点中的xyzw
        pyramidBatch.MultiTexCoord2f(0, s, t);
     
     
        向三角形批次类中添加顶点数据 （x,y,z）
        pyramidBatch.Vertex3f(-1.0f, -1.0f, -1.0f);
     */
//    pyramidBatch.MultiTexCoord2f(0, s, t);
//    pyramidBatch.Vertex3f(-1.0f, -1.0f, -1.0f);
    
    M3DVector3f vApex = { 0.0f, 1.0f, 0.0f };
    M3DVector3f vFrontLeft = { -1.0f, -1.0f, 1.0f };
    M3DVector3f vFrontRight = { 1.0f, -1.0f, 1.0f };
    M3DVector3f vBackLeft = { -1.0f,  -1.0f, -1.0f };
    M3DVector3f vBackRight = { 1.0f,  -1.0f, -1.0f };

    // 参考讲义纹理.pdf
    //金字塔底部
    //底部的四边形 = 三角形X + 三角形Y
    //三角形X = (vBackLeft,vBackRight,vFrontRight)
    //vBackLeft   纹理层次 默认0  后面两个 是纹理坐标
    pyramidBatch.MultiTexCoord2f(0, 0.0f, 0.0f);
    
    // 传入对应的顶点坐标
    pyramidBatch.Vertex3fv(vBackLeft);
    
    //vBackRight
    pyramidBatch.MultiTexCoord2f(0, 1.0f, 0.0f);
    pyramidBatch.Vertex3fv(vBackRight);
    
    //vFrontRight
    pyramidBatch.MultiTexCoord2f(0, 1.0f, 1.0f);
    pyramidBatch.Vertex3fv(vFrontRight);
    
    
    //三角形Y =(vFrontLeft,vBackLeft,vFrontRight)
    //vFrontLeft
    pyramidBatch.MultiTexCoord2f(0, 0.0f, 1.0f);
    pyramidBatch.Vertex3fv(vFrontLeft);
    
    //vBackLeft
    pyramidBatch.MultiTexCoord2f(0, 0.0f, 0.0f);
    pyramidBatch.Vertex3fv(vBackLeft);
    
    //vFrontRight
    pyramidBatch.MultiTexCoord2f(0, 1.0f, 1.0f);
    pyramidBatch.Vertex3fv(vFrontRight);

    
    // 金字塔前面
    //三角形：（Apex，vFrontLeft，vFrontRight）
    pyramidBatch.MultiTexCoord2f(0, 0.5f, 1.0f);
    pyramidBatch.Vertex3fv(vApex);

    pyramidBatch.MultiTexCoord2f(0, 0.0f, 0.0f);
    pyramidBatch.Vertex3fv(vFrontLeft);

    pyramidBatch.MultiTexCoord2f(0, 1.0f, 0.0f);
    pyramidBatch.Vertex3fv(vFrontRight);
    
    //金字塔左边
    //三角形：（vApex, vBackLeft, vFrontLeft）
    pyramidBatch.MultiTexCoord2f(0, 0.5f, 1.0f);
    pyramidBatch.Vertex3fv(vApex);
    
    pyramidBatch.MultiTexCoord2f(0, 1.0f, 0.0f);
    pyramidBatch.Vertex3fv(vBackLeft);
    
    pyramidBatch.MultiTexCoord2f(0, 0.0f, 0.0f);
    pyramidBatch.Vertex3fv(vFrontLeft);
    
    //金字塔右边
    //三角形：（vApex, vFrontRight, vBackRight）
    pyramidBatch.MultiTexCoord2f(0, 0.5f, 1.0f);
    pyramidBatch.Vertex3fv(vApex);
    
    pyramidBatch.MultiTexCoord2f(0, 1.0f, 0.0f);
    pyramidBatch.Vertex3fv(vFrontRight);

    pyramidBatch.MultiTexCoord2f(0, 0.0f, 0.0f);
    pyramidBatch.Vertex3fv(vBackRight);
    
    //金字塔后边
    //三角形：（vApex, vBackRight, vBackLeft）
    pyramidBatch.MultiTexCoord2f(0, 0.5f, 1.0f);
    pyramidBatch.Vertex3fv(vApex);
    
    pyramidBatch.MultiTexCoord2f(0, 0.0f, 0.0f);
    pyramidBatch.Vertex3fv(vBackRight);
    
    pyramidBatch.MultiTexCoord2f(0, 1.0f, 0.0f);
    pyramidBatch.Vertex3fv(vBackLeft);
    
    pyramidBatch.End();
}

/*
    手动仅调用一次
    处理业务：  1、 设置窗口背景颜色  2、初始化存储着色器 shaderManager  3、 设置图形顶点数据  4、 利用GLBatch 三角形批次类 将数据传递到着色器
 */
void setupRC() {
    //1.
    glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
    shaderManager.InitializeStockShaders();

    //2.
    glEnable(GL_DEPTH_TEST);

    //3.分配纹理对象 参数1: 纹理对象个数  参数2： 纹理对象指针   textTureID： GLUint类型
    glGenTextures(1, &textTureID);
    
    //4. 绑定纹理 参数1： 纹理状态 1D 2D 3D  参数2：纹理对象
    glBindTexture(GL_TEXTURE_2D, textTureID);
    
    // 将TGA 文件加载给2D纹理  纹理图像默认是.tga文件 在iOS和Mac开发中使用 png 或者jpg,最终使用到的是位图
    LoadTGATexture("stone.tga", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR, GL_CLAMP_TO_EDGE);
    
    //5. 创建金字塔
    MakePyramid(pyramidBatch);
    
    cameraFrame.MoveForward(-10);
 }

void SpecialKeys(int key, int x, int y) {
    
    if (key == GLUT_KEY_UP) {
        objectFrame.RotateWorld(m3dDegToRad(-5.0), 1.0f, 0.0f, 0.0f);
    }
    if (key == GLUT_KEY_DOWN) {
        objectFrame.RotateWorld(m3dDegToRad(5.0f), 1.0f, 0.0f, 0.0f);
    }
    if (key == GLUT_KEY_LEFT) {
        objectFrame.RotateWorld(m3dDegToRad(-5.0), 0.0f, 1.0f, 0.0f);
    }
    if (key == GLUT_KEY_RIGHT) {
        objectFrame.RotateWorld(m3dDegToRad(5.0f), 0.0f, 1.0f, 0.0f);
    }
    
    glutPostRedisplay();
}


int main(int argc,char *argv[])  {
    
    // argv[0] "/Users/liking/Library/Developer/Xcode/DerivedData/01_OpenGL_环境搭建-cwwgkzykdrwdzqfjiveckxxyklvj/Build/Products/Debug/01 OpenGL 环境搭建.app/Contents/MacOS/01 OpenGL 环境搭建" 路径

    gltSetWorkingDirectory(argv[0]);
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_STENCIL | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Pyramid");
    
    glutSpecialFunc(SpecialKeys);
    glutReshapeFunc(changeSize);
    glutDisplayFunc(RenderScene);
    
    GLenum status = glewInit();
    if (GLEW_OK != status) {
        fprintf(stderr, "GLEW Error: %s\n",glewGetString(status));
        return 1;
    }
    
    setupRC();
    
    glutMainLoop();
    
    ShutdownRC();
    
    return 0;

}
