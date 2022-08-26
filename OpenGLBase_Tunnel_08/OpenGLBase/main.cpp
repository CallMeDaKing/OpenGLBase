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
// 变换管道
GLGeometryTransform transformPipline;

// 批次类容器
GLBatch         floorBatch;         // 地面
GLBatch         ceilingBatch;       // 天花板
GLBatch         leftWallBatch;      // 左侧墙面
GLBatch         rightWallBatch;     // 右侧墙面

GLfloat         viewZ = -65.0f;     // 初始深度

// 纹理标识符
#define TEXTURE_BRICK       0   // 墙
#define TEXTURE_FLOOR       1   // 地板
#define TEXTURE_CELLINg     2   // 天花板
#define TEXTURE_COUNT       3   // 纹理个数

// 纹理数组
GLuint          textures[TEXTURE_COUNT];

// tga 文件数组
const char *szTextureFiles[TEXTURE_COUNT] = { "brick.tga", "floor.tga", "ceiling.tga" };


// 当屏幕进行刷新的时候调用多次，系统在刷新的时候主动调用 比如60帧 相当于每秒刷新60次， 调用60 次
void RenderScene(void) {
    
    // 清空当前视图窗口
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // 模型视图压栈 压的是单元矩阵
    modelViewMatrix.PushMatrix();
    // z 轴平移viewZ 的距离
    modelViewMatrix.Translate(0.0f, 0.0f, viewZ);
    
    /*  纹理替换矩阵着色器
        参数1： GLT_SHADER_TEXTURE_REPLACE（着色器标签）
        参数2： 模型视图矩阵
        参数3： 纹理层个数
     */
    shaderManager.UseStockShader(GLT_SHADER_TEXTURE_REPLACE,transformPipline.GetModelViewProjectionMatrix(),0);
    
    /*
     绑定纹理
        参数1: 纹理模式（维度）
        参数2：需要绑定的纹理
     */
    glBindTexture(GL_TEXTURE_2D, textures[TEXTURE_FLOOR]);
    floorBatch.Draw();
    
    glBindTexture(GL_TEXTURE_2D, textures[TEXTURE_CELLINg]);
    ceilingBatch.Draw();
    
    glBindTexture(GL_TEXTURE_2D, textures[TEXTURE_BRICK]);
    leftWallBatch.Draw();
    rightWallBatch.Draw();
    
    modelViewMatrix.PopMatrix();
    
    glutSwapBuffers();
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
    
    GLfloat fAspect = (GLfloat)w / (GLfloat)h;
    viewFrustum.SetPerspective(80.0f, fAspect, 1.0f, 120.0f);
    projectionMatrix.LoadMatrix(viewFrustum.GetProjectionMatrix());
    transformPipline.SetMatrixStacks(modelViewMatrix, projectionMatrix);
}

// 清理…例如删除纹理对象 关闭渲染环境
void ShutdownRC(void) {
    glDeleteTextures(TEXTURE_COUNT, textures);
}


/*
    手动仅调用一次
    处理业务：  1、 设置窗口背景颜色  2、初始化存储着色器 shaderManager 3、加载纹理数据 4、 设置图形顶点数据  5、 利用GLBatch 三角形批次类 将数据传递到着色器
 */
void setupRC() {
    // 1.
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    // 2.
    shaderManager.InitializeStockShaders();

    GLbyte *pBytes;
    GLint iWidth, iHeight, iComponents;
    GLenum eFformat;
    GLint iLoop;
    
    /*
       3. 生成纹理标记 glGenTextures
        参数1： 纹理对象数量
        参数2： 纹理对象数组
     */
    glGenTextures(TEXTURE_COUNT, textures);
    
    for (iLoop = 0; iLoop < TEXTURE_COUNT; iLoop++) {
        
        /*
            绑定纹理对象
            参数1：纹理模式（维度）
            参数2： 需要绑定纹理对象
         */
        glBindTexture(GL_TEXTURE_2D, textures[iLoop]);
        
        /*
         读取 tga文件
         参数1： 纹理文件名
         参数2： 文件宽度地址
         参数3： 文件高度地址
         参数4： 文件组件变量地址
         参数5： 文件格式变量地址
         参数6： pBytes, 指向图像数据的指针
         */
        pBytes = gltReadTGABits(szTextureFiles[iLoop], &iWidth, &iHeight, &iComponents, &eFformat);
        
        // 设置放大缩小过滤器， 临近过滤和线性过滤
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        
        // 设置环绕模式
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        /**载入纹理 glTexImage2D
         参数1：纹理维度，GL_TEXTURE_2D
         参数2：mip贴图层次
         参数3：纹理单元存储的颜色成分（从读取像素图中获得）
         参数4：加载纹理宽度
         参数5：加载纹理的高度
         参数6：加载纹理的深度
         参数7：像素数据的数据类型,GL_UNSIGNED_BYTE无符号整型
         参数8：指向纹理图像数据的指针
         */
        glTexImage2D(GL_TEXTURE_2D, 0, iComponents, iWidth, iHeight, 0, eFformat, GL_UNSIGNED_BYTE, pBytes);
        
        /**为纹理对象生成一组完整的mipmap glGenerateMipmap
         参数1：纹理维度，GL_TEXTURE_1D,GL_TEXTURE_2D,GL_TEXTURE_2D
         */
        glGenerateMipmap(GL_TEXTURE_2D);
        free(pBytes);
    }
    
    // 设置几何图形顶点/纹理坐标(上.下.左.右)
    GLfloat z;

    /*
        参数1：图元枚举值
        参数2：顶点个数
        参数3：纹理个数
     */
    
    // 绘制地板 将纹理和顶点绑定（纹理贴图坐标和顶点坐标对齐）
    floorBatch.Begin(GL_TRIANGLE_STRIP, 28, 1);
    for (z = 60.0f; z >= 0.0f; z -= 10.0f) {
        floorBatch.MultiTexCoord2f(0, 0.0f, 0.0f);
        floorBatch.Vertex3f(-10.0f, -10.0f, z);
        
        floorBatch.MultiTexCoord2f(0, 1.0f, 0.0f);
        floorBatch.Vertex3f(10.0f, -10.0f, z);
        
        floorBatch.MultiTexCoord2f(0, 0.0f, 1.0f);
        floorBatch.Vertex3f(-10.0f, -10.0f, z - 10.0f);
        
        floorBatch.MultiTexCoord2f(0, 1.0f, 1.0f);
        floorBatch.Vertex3f(10.0f, -10.0f, z - 10.0f);
    }
    floorBatch.End();
    
    // 同理 绘制天花板
    ceilingBatch.Begin(GL_TRIANGLE_STRIP, 28, 1);
    for(z = 60.0f; z >= 0.0f; z -=10.0f)
    {
        ceilingBatch.MultiTexCoord2f(0, 0.0f, 1.0f);
        ceilingBatch.Vertex3f(-10.0f, 10.0f, z - 10.0f);
        
        ceilingBatch.MultiTexCoord2f(0, 1.0f, 1.0f);
        ceilingBatch.Vertex3f(10.0f, 10.0f, z - 10.0f);
        
        ceilingBatch.MultiTexCoord2f(0, 0.0f, 0.0f);
        ceilingBatch.Vertex3f(-10.0f, 10.0f, z);
        
        ceilingBatch.MultiTexCoord2f(0, 1.0f, 0.0f);
        ceilingBatch.Vertex3f(10.0f, 10.0f, z);
    }
    ceilingBatch.End();
    
    // 左侧墙面
    leftWallBatch.Begin(GL_TRIANGLE_STRIP, 28, 1);
    for(z = 60.0f; z >= 0.0f; z -=10.0f)
    {
        leftWallBatch.MultiTexCoord2f(0, 0.0f, 0.0f);
        leftWallBatch.Vertex3f(-10.0f, -10.0f, z);
        
        leftWallBatch.MultiTexCoord2f(0, 0.0f, 1.0f);
        leftWallBatch.Vertex3f(-10.0f, 10.0f, z);
        
        leftWallBatch.MultiTexCoord2f(0, 1.0f, 0.0f);
        leftWallBatch.Vertex3f(-10.0f, -10.0f, z - 10.0f);
        
        leftWallBatch.MultiTexCoord2f(0, 1.0f, 1.0f);
        leftWallBatch.Vertex3f(-10.0f, 10.0f, z - 10.0f);
    }
    leftWallBatch.End();
    
    // 右侧墙面
    rightWallBatch.Begin(GL_TRIANGLE_STRIP, 28, 1);
    for(z = 60.0f; z >= 0.0f; z -=10.0f)
    {
        rightWallBatch.MultiTexCoord2f(0, 0.0f, 0.0f);
        rightWallBatch.Vertex3f(10.0f, -10.0f, z);
        
        rightWallBatch.MultiTexCoord2f(0, 0.0f, 1.0f);
        rightWallBatch.Vertex3f(10.0f, 10.0f, z);
        
        rightWallBatch.MultiTexCoord2f(0, 1.0f, 0.0f);
        rightWallBatch.Vertex3f(10.0f, -10.0f, z - 10.0f);
        
        rightWallBatch.MultiTexCoord2f(0, 1.0f, 1.0f);
        rightWallBatch.Vertex3f(10.0f, 10.0f, z - 10.0f);
    }
    rightWallBatch.End();
 }

void SpecialKeys(int key, int x, int y) {
    if (key == GLUT_KEY_UP) {
        viewZ += 0.5;
    }
    
    if (key == GLUT_KEY_DOWN) {
        viewZ -= 0.5;
    }
    
    // 更新窗口 即刻回调到RenderScene方法中进行渲染
    glutPostRedisplay();
}

void ProcessMenu(int value) {
    GLint iLoop;
    
    for (iLoop = 0; iLoop < TEXTURE_COUNT; iLoop++) {
        // 绑定纹理
        glBindTexture(GL_TEXTURE_2D, textures[iLoop]);
        
        switch (value) {
            case 0:
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                break;
            case 1:
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                break;
            case 2:
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
                break;
            case 3:
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
                break;
            case 4:
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
                break;
            case 5:
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                break;
            case 6:
            
                //设置各向异性过滤
                GLfloat fLargest;
                //获取各向异性过滤的最大数量
                glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
                //设置纹理参数(各向异性采样)
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest);
                break;
        
            case 7:
                //设置各向同性过滤，数量为1.0表示(各向同性采样)
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
                break;
                

            default:
                break;
        }
        
    }
    glutPostRedisplay();
}

int main(int argc,char *argv[])  {
    // argv 存储的项目目录路径
    gltSetWorkingDirectory(argv[0]);
    
    // 标准初始化
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Tunnel");
    
    glutReshapeFunc(changeSize);
    glutSpecialFunc(SpecialKeys);
    glutDisplayFunc(RenderScene);
    
    // 加入菜单
    glutCreateMenu(ProcessMenu);
    glutAddMenuEntry("GL_NEAREST", 0);
    glutAddMenuEntry("GL_LINEAR", 1);
    glutAddMenuEntry("GL_NEAREST_MIPMAP_NEAREST", 2);
    glutAddMenuEntry("GL_NEAREST_MIPMAP_LINEAR", 3);
    glutAddMenuEntry("GL_LINEAR_MIPMAP_NEAREST", 4);
    glutAddMenuEntry("GL_LINEAR_MIPMAP_LINEAR", 5);
    glutAddMenuEntry("Anisotropic Filter", 6);
    glutAddMenuEntry("Anisotropic Off", 7);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
    
    GLenum error = glewInit();
    if (error != GLEW_OK) {
        fprintf(stderr, "GLEW Error: %s\n", glewGetErrorString(error));
        return 1;
    }
    
    setupRC();
    glutMainLoop();
    glutShowWindow();
    return 0;
}
