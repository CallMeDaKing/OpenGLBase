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

//定义一个，着色管理器管理类
GLShaderManager         shaderManager;
GLMatrixStack           modelViewMatrix;      // 模型视图矩阵堆栈 管理模型矩阵变化
GLMatrixStack           projectionMatrix;     // 投影矩阵堆栈 管理投影矩阵变化
GLFrustum               viewFrustum;          // 投影矩阵，设置图元绘制时的投影方式  透视投影 perspective  正投影 orthographics
GLGeometryTransform    transformPipeline;     //几何变换管线

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

GLBatch             floorBatch;     // 地面
GLBatch             ceilingBatch;   // 天花板
GLBatch             leftWallBatch;  // 左墙面
GLBatch             rightWallBatch; // 右墙面

// 初始深度
GLfloat             viewZ = -65.0f;

#define TEXTURE_BRICK   0    // 墙面
#define TEXTURE_FLOOR   1    // 地板
#define TEXTURE_CEILING 2    // 纹理天花板
#define TEXTURE_COUNT   3    // 纹理个数
GLuint textures[TEXTURE_COUNT]; // 纹理数组

const char *szTextureFiles[TEXTURE_COUNT] = {  "brick.tga", "floor.tga", "ceiling.tga"  };

// 当屏幕进行刷新的时候调用多次，系统在刷新的时候主动调用 比如60帧 相当于每秒刷新60次， 调用60 次
void RenderScene(void) {

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
void ShutdownRC(void) {
    
}


/*
    手动仅调用一次
    处理业务：  1、 设置窗口背景颜色  2、初始化存储着色器 shaderManager  3、 设置图形顶点数据  4、 利用GLBatch 三角形批次类 将数据传递到着色器
 */
void setupRC() {
    //1.
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    
    shaderManager.InitializeStockShaders();
    
    GLbyte *pBytes;
    GLint iWidth, iHeight, iCompenents;
    GLenum eFormat;
    GLint iLoop;
    
    glGenTextures(TEXTURE_COUNT, textures);
    
    for (iLoop = 0; iLoop < TEXTURE_COUNT; iLoop++) {
        glBindTexture(GL_TEXTURE_2D, textures[iLoop]);
        
        // 读取tga 文件
        pBytes = gltReadTGABits(szTextureFiles[iLoop], &iWidth, &iHeight, &iCompenents, &eFormat);
        
        // 设置图片缩小放大的过滤方式
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        
        // 设置环绕模式 ST  对应 x y
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        
        // 加载纹理
        glTexImage2D(GL_TEXTURE_2D, 0, iCompenents, iWidth, iHeight, 0, eFormat, GL_UNSIGNED_BYTE, pBytes);
        
        /*
            为纹理对象 生成一组完整的mipmap
         参数： 纹理维度  1D  2D 3D
         */
        glGenerateMipmap(GL_TEXTURE_2D);
        
        // 释放原始纹理数据
        free(pBytes);
    }
    
    // 设置顶点数据
    
    // 绘制地板
    
    
 }

void SpecialKeys(int key, int x, int y) {
    
}

void ProcessMenue(int value) {
    
}

int main(int argc,char *argv[])  {
    
    // argv[0] "/Users/liking/Library/Developer/Xcode/DerivedData/01_OpenGL_环境搭建-cwwgkzykdrwdzqfjiveckxxyklvj/Build/Products/Debug/01 OpenGL 环境搭建.app/Contents/MacOS/01 OpenGL 环境搭建" 路径
    gltSetWorkingDirectory(argv[0]);
    
    glutInit(&argc, argv);
    glutInitDisplayMode(GL_DOUBLE | GL_RGBA);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Tunnel");
    
    // 注册函数
    glutSpecialFunc(SpecialKeys);
    glutReshapeFunc(changeSize);
    glutDisplayFunc(RenderScene);
    
    // 创建menue
    glutCreateMenu(ProcessMenue);
    glutAddMenuEntry("GL_NEAREST", 0);
    glutAddMenuEntry("GL_LINEAR", 1);
    glutAddMenuEntry("GL_NEAREST_MIPMAP_NEAREST", 2);
    glutAddMenuEntry("GL_NEAREST_MIPMAP_LINEAR", 3);
    glutAddMenuEntry("GL_LINEAR_MIPMAP_NEAREST", 4);
    glutAddMenuEntry("GL_LINEAR_MIPMAP_LINEAR", 5);
    glutAddMenuEntry("Anisotropic Filter", 6);
    glutAddMenuEntry("Anisotropic Off", 7);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
    
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        fprintf(stderr, "GLEW Error: %s\n", glewGetErrorString(err));
        return 1;
    }
    
    // 启动循环 关闭纹理
    setupRC();
    glutMainLoop();
    ShutdownRC();
    
    return 0;

}
