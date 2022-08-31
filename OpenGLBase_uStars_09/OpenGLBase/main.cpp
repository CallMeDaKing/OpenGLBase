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
GLFrame spheres[NUM_SPHERES];

//定义一个，着色管理器管理类
GLShaderManager         shaderManager;
GLMatrixStack           modelViewMatrix;      // 模型视图矩阵堆栈 管理模型矩阵变化
GLMatrixStack           projectionMatrix;     // 投影矩阵堆栈 管理投影矩阵变化
GLFrame                 cameraFrame;          // 记录观察者（矩阵） 变化 上下左右移动
GLFrame                 objectFrame;          // 记录物体（矩阵）变化 操作当前对象的变化的矩阵， 上下左右移动。
GLFrustum               viewFrustum;          // 投影矩阵，设置图元绘制时的投影方式  透视投影 perspective  正投影 orthographics
GLTriangleBatch         torusBatch;
GLBatch                 floorBatch;           // 镜面处理
GLTriangleBatch         sphereBatch;          // 球批处理
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
// 变换管道
GLGeometryTransform transformPipline;
// 纹理数组
GLuint               uiTextures[3];

bool LoadTGATexture(const char *szFileName, GLenum minFilter, GLenum magFilter, GLenum wrapMode) {
    GLbyte *pBytes;
    GLint nWidth, nHeight, nComponents;
    GLenum eFormat;
    pBytes = gltReadTGABits(szFileName, &nWidth, &nHeight, &nComponents, &eFormat);
    
    if (pBytes == NULL) { return  false; }
    
    // 设置纹理环绕模式
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
    
    // 设置纹理维度
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGB, nWidth, nHeight, 0, eFormat, GL_UNSIGNED_BYTE, pBytes);
    
    free(pBytes);
    
    if (minFilter == GL_LINEAR_MIPMAP_LINEAR ||
        minFilter == GL_LINEAR_MIPMAP_NEAREST ||
        minFilter == GL_NEAREST_MIPMAP_LINEAR ||
        minFilter == GL_NEAREST_MIPMAP_NEAREST) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    
    return true;
}

/*
    手动仅调用一次
    处理业务：  1、 设置窗口背景颜色  2、初始化存储着色器 shaderManager 3、加载纹理数据 4、 设置图形顶点数据  5、 利用GLBatch 三角形批次类 将数据传递到着色器
 */
void setupRC() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    shaderManager.InitializeStockShaders();
    
    // 开启深度测试 背面踢除
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    
    // 绘制大球
    gltMakeSphere(torusBatch, 0.4f, 40, 80);
    // 绘制小球
    gltMakeSphere(sphereBatch, 0.1f, 26, 13);
    
    // 绘制地板顶点
    GLfloat texSize = 10.0f;
    floorBatch.Begin(GL_TRIANGLE_FAN, 4,1);
    
    floorBatch.MultiTexCoord2f(0, 0.0f, 0.0f);
    floorBatch.Vertex3f(-20.f, -0.41f, 20.0f);
    
    floorBatch.MultiTexCoord2f(0, texSize, 0.0f);
    floorBatch.Vertex3f(20.0f, -0.41f, 20.0f);
    
    floorBatch.MultiTexCoord2f(0, texSize, texSize);
    floorBatch.Vertex3f(20.0f, -0.41f, -20.0f);
    
    floorBatch.MultiTexCoord2f(0, 0.0f, texSize);
    floorBatch.Vertex3f(-20.0f, -0.41f, -20.0f);
    floorBatch.End();
    
    // 绘制随机的小球
    for (int i = 0; i < NUM_SPHERES; i++) {
        GLfloat x = ((GLfloat)((rand() % 400) - 200) * 0.1f);
        GLfloat z = ((GLfloat)((rand() % 400) - 200) * 0.1f);
        spheres[i].SetOrigin(x, 0.0f, z);
    }
    
    // 命名纹理对象
    glGenTextures(3, uiTextures);
    
    // 将TGA加载到2D纹理
    glBindTexture(GL_TEXTURE_2D, uiTextures[0]);
    // 获取tga
    LoadTGATexture("marble.tga", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_REPEAT);
    
    glBindTexture(GL_TEXTURE_2D, uiTextures[1]);
    LoadTGATexture("marslike.tga", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);
    
    glBindTexture(GL_TEXTURE_2D, uiTextures[2]);
    LoadTGATexture("moonlike.tga", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);
}

void shutDownRC(void) {
    glDeleteTextures(3, uiTextures);
}

void drawSomething(GLfloat yRot) {
    
    // 定义光源位置和漫反射颜色
    static GLfloat vWhite[] = {1.0f, 1.0f, 1.0f, 1.0f};
    static GLfloat vLightPos[] = { 0.0, 3.0f, 0.0f, 1.0f};  // 灯光位置
    
    // 绘制悬浮球球
    glBindTexture(GL_TEXTURE_2D, uiTextures[2]);
    for (int i = 0; i < NUM_SPHERES; i++) {
        modelViewMatrix.PushMatrix();
        modelViewMatrix.MultMatrix(spheres[i]);
        shaderManager.UseStockShader(GLT_SHADER_TEXTURE_POINT_LIGHT_DIFF,
                                     modelViewMatrix.GetMatrix(),
                                     transformPipline.GetProjectionMatrix(),
                                     vLightPos,
                                     vWhite,
                                     0);
        sphereBatch.Draw();
        modelViewMatrix.PopMatrix();
    }
    
    // 绘制大球
    modelViewMatrix.Translate(0.0f, 0.2f, -2.5f);
    modelViewMatrix.PushMatrix();
    modelViewMatrix.Rotate(yRot, 0.0f, 1.0f, 0.0f);
    glBindTexture(GL_TEXTURE_2D, uiTextures[1]);
    shaderManager.UseStockShader(GLT_SHADER_TEXTURE_POINT_LIGHT_DIFF,
                                 modelViewMatrix.GetMatrix(),
                                 transformPipline.GetProjectionMatrix(),
                                 vLightPos,
                                 vWhite,
                                 0);
    torusBatch.Draw();
    modelViewMatrix.PopMatrix();
    
    // 绘制公转的小球
    modelViewMatrix.PushMatrix();
    modelViewMatrix.Rotate(yRot * -2.0f, 0.0f, 1.0f, 0.0f);
    modelViewMatrix.Translate(0.8f, 0.0f, 0.0f);
    glBindTexture(GL_TEXTURE_2D, uiTextures[2]);
    shaderManager.UseStockShader(GLT_SHADER_TEXTURE_POINT_LIGHT_DIFF,
                                 modelViewMatrix.GetMatrix(),
                                 transformPipline.GetProjectionMatrix(),
                                 vLightPos,
                                 vWhite,
                                 0);
    sphereBatch.Draw();
    modelViewMatrix.PopMatrix();
}

// 当屏幕进行刷新的时候调用多次，系统在刷新的时候主动调用 比如60帧 相当于每秒刷新60次， 调用60 次
void RenderScene(void) {
    
    static GLfloat vFloorColor[] = {1.0f,1.0f,0.0f,0.75f};
    
    // 基于动画时间
    static CStopWatch rotTimer;
    float yRot = rotTimer.GetElapsedSeconds() * 60.0f;
    
    // 清楚颜色缓冲区
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // 压栈 单元矩阵
    modelViewMatrix.PushMatrix();
    
    // 设置观察者矩阵
    M3DMatrix44f mCamera;
    cameraFrame.GetCameraMatrix(mCamera);
    modelViewMatrix.MultMatrix(mCamera);
    
    // 压入堆栈 顶部是观察者 * 单元矩阵
    modelViewMatrix.PushMatrix();
    
    modelViewMatrix.Scale(1.0f, -1.0f, 1.0f);
    // y 轴上移一定距离 翻转后 y轴下移0.8
    modelViewMatrix.Translate(0.0f, 0.8f, 0.0f);
    
    // 指定顺时针为正面
    glFrontFace(GL_CW);
    
    // 绘制镜面以下的部分 球体的绘制是相同的单独抽出来一个方法，其他的平移旋转操作放在外面
    drawSomething(yRot);
    
    //恢复 指定逆时针为背面
    glFrontFace(GL_CCW);
    
    // 出栈 结束当前状态
    modelViewMatrix.PopMatrix();
    
    
    // 绘制镜面 以及镜面上面的部分
    glEnable(GL_BLEND);
    // 设置混合方程式
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // 绑定纹理
    glBindTexture(GL_TEXTURE_2D, uiTextures[0]);
    /*
        纹理调整着色器
     参数1：GLT_SHADER_TEXTURE_MODULATE
     参数2：模型视图矩阵
     参数3：颜色
     参数4：纹理单元（第0层纹理单元）
     */
    shaderManager.UseStockShader(GLT_SHADER_TEXTURE_MODULATE,
                                 transformPipline.GetModelViewProjectionMatrix(),
                                 vFloorColor,
                                 0);
    floorBatch.Draw();
    glDisable(GL_BLEND);
    drawSomething(yRot);
    modelViewMatrix.PopMatrix();
    // 交换缓存去
    glutSwapBuffers();
    // 提交重新渲染
    glutPostRedisplay();
}

// 两个作用， 1.设置视图大小 2.设置投影矩阵
void changeSize(int w,int h) {
    if (h == 0) {
        h = 1;
    }
    // 设置视图窗口
    glViewport(0, 0, w, h);
    
    // 设置投影矩阵
    /*  SetPerspective
     参数1： 视角大小
     参数2： 宽高比
     参数3： 最近可视距离
     参数4： 最远可视距离
     
     */
    
    GLfloat fAspect = (float)w / (float)h;
    viewFrustum.SetPerspective(35.0f, fAspect, 1.0f, 100.0f);
    projectionMatrix.LoadMatrix(viewFrustum.GetProjectionMatrix());
    modelViewMatrix.LoadIdentity();
    transformPipline.SetMatrixStacks(modelViewMatrix, projectionMatrix);
}

void SpecialKeys(int key, int x, int y) {
    float linear = 0.1f;
    float angular = float(m3dDegToRad(5.0f));
    
    if (key == GLUT_KEY_UP) {
        cameraFrame.MoveForward(linear);
    }
    
    if (key == GLUT_KEY_DOWN) {
        cameraFrame.MoveForward(-linear);
    }
    
    if (key == GLUT_KEY_LEFT) {
        cameraFrame.RotateWorld(angular, 0.0f, 1.0f, 0.0f);
    }
    
    if (key == GLUT_KEY_RIGHT) {
        cameraFrame.RotateWorld(-angular, 0.0f, 1.0f, 0.0f);
    }
    
    // 更新窗口 即刻回调到RenderScene方法中进行渲染
    glutPostRedisplay();
}

int main(int argc,char *argv[])  {
    // argv 存储的项目目录路径
    gltSetWorkingDirectory(argv[0]);
    
    // 标准初始化
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Tunnel");
    
    glutReshapeFunc(changeSize);
    glutSpecialFunc(SpecialKeys);
    glutDisplayFunc(RenderScene);
    
    GLenum error = glewInit();
    if (error != GLEW_OK) {
        fprintf(stderr, "GLEW Error: %s\n", glewGetErrorString(error));
        return 1;
    }
    
    setupRC();
    glutMainLoop();
    shutDownRC();
    return 0;
}
