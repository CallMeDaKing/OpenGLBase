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
GLMatrixStack           projectionMatrix;     // 模型视图矩阵  堆栈  记录变化 上下左右移动
GLFrame                 cameraFrame;          // 投影矩阵     堆栈  记录投影方式的
GLFrame                 objectFrame;          // 本质上是矩阵，操作当前对象的变化的矩阵， 上下左右移动。
GLFrustum               viewFrustum;          // 投影矩阵，设置图元绘制时的投影方式

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
 
    在OPenGL 世界中，Y 是指向天空的UP，XOZ 构成地面， Z 是Forward
 
    总结: 实际上GLFrame 是一系列变化，有GLFrame可以导出变换矩阵，只要与该变换矩阵相乘，任何物体都可以进行GLFrame相应的变化。 比如两个物体AB， 经过A 的GLFrame 导出变换矩阵，让B乘以变换矩阵，本来B 的坐标系是相对于世界坐标系的， 现在变变为了相对于A的坐标系。
 */

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

void DrawWireFramedBatch(GLBatch* pBatch) {
    /*------------画绿色部分----------------*/
    /* GLShaderManager 中的Uniform 值——平面着色器
     参数1：平面着色器
     参数2：运行为几何图形变换指定一个 4 * 4变换矩阵
          --transformPipeline 变换管线（指定了2个矩阵堆栈）
     参数3：颜色值
    */
    shaderManager.UseStockShader(GLT_SHADER_FLAT, transformPipline.GetModelViewProjectionMatrix(), vGreen);
    pBatch->Draw();
    
    /*-----------边框部分-------------------*/
    /*
        glEnable(GLenum mode); 用于启用各种功能。功能由参数决定
        参数列表：http://blog.csdn.net/augusdi/article/details/23747081
        注意：glEnable() 不能写在glBegin() 和 glEnd()中间
        GL_POLYGON_OFFSET_LINE  根据函数glPolygonOffset的设置，启用线的深度偏移
        GL_LINE_SMOOTH          执行后，过虑线点的锯齿
        GL_BLEND                启用颜色混合。例如实现半透明效果
        GL_DEPTH_TEST           启用深度测试 根据坐标的远近自动隐藏被遮住的图形（材料
     
     
        glDisable(GLenum mode); 用于关闭指定的功能 功能由参数决定
     
     */
    
    //画黑色边框
    glPolygonOffset(-1.0f, -1.0f);// 偏移深度，在同一位置要绘制填充和边线，会产生z冲突，所以要偏移
    glEnable(GL_POLYGON_OFFSET_LINE);
    
    // 画反锯齿，让黑边好看些
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    //绘制线框几何黑色版 三种模式，实心，边框，点，可以作用在正面，背面，或者两面
    //通过调用glPolygonMode将多边形正面或者背面设为线框模式，实现线框渲染
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    //设置线条宽度
    glLineWidth(2.5f);
    
    /* GLShaderManager 中的Uniform 值——平面着色器
     参数1：平面着色器
     参数2：运行为几何图形变换指定一个 4 * 4变换矩阵
         --transformPipeline.GetModelViewProjectionMatrix() 获取的
          GetMatrix函数就可以获得矩阵堆栈顶部的值
     参数3：颜色值（黑色）
     */
    
    shaderManager.UseStockShader(GLT_SHADER_FLAT, transformPipline.GetModelViewProjectionMatrix(), vBlack);
    pBatch->Draw();

    // 复原原本的设置
    //通过调用glPolygonMode将多边形正面或者背面设为全部填充模式
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_POLYGON_OFFSET_LINE);
    glLineWidth(1.0f);
    glDisable(GL_BLEND);
    glDisable(GL_LINE_SMOOTH);
    
    
}


// 当屏幕进行刷新的时候调用多次，系统在刷新的时候主动调用 比如60帧 相当于每秒刷新60次， 调用60 次
void RenderScene(void) {
    
    //MARK: 方法2  使用矩阵实现
    // 清除上一次缓存
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    
    // 压栈操作， modelViewMatrix 作为个堆栈用于记录中间变换状态的矩阵
    modelViewMatrix.PushMatrix();
    
    // 获取观察者矩阵 并将单元矩阵和观察者矩阵相乘
    M3DMatrix44f mCameraMatrix;
    cameraFrame.GetCameraMatrix(mCameraMatrix);
    modelViewMatrix.MultMatrix(mCameraMatrix);
    
    // 获取物体矩阵
    M3DMatrix44f mObjectMatrix;
    objectFrame.GetMatrix(mObjectMatrix);
    // 矩阵相乘 newCamera 矩阵 和 物体矩阵相乘 最终结果放到放到模型视图矩阵堆栈中
    modelViewMatrix.MultMatrix(mObjectMatrix);
    
    // 使用着色器绘制 transformPipline获取模型定点数据，这个定点是mult（观察者矩阵，物体矩阵） 混合后的矩阵数据  mvp (modelViewProject) Matrix 模型视图矩阵
    shaderManager.UseStockShader(GLT_SHADER_FLAT, transformPipline.GetModelViewProjectionMatrix(), vBlack);

    switch (nStep) {
        case 0:
            glPointSize(4.0f);
            pointBatch.Draw();
            glPointSize(1.0f); // 需要恢复默认大小
            break;
        case 1:
            glLineWidth(4.0f);
            lineBatch.Draw();
            glLineWidth(1.0f);
            break;
        case 2:
            glLineWidth(4.0f);
            lineStripBatch.Draw();
            glLineWidth(1.0f);
            break;
        case 3:
            glLineWidth(4.0f);
            lineLoopBatch.Draw();
            glLineWidth(1.0f);
            break;
        case 4:
            DrawWireFramedBatch(&triangleBatch);
            break;
        case 5:
            DrawWireFramedBatch(&triangleStripBatch);
            break;
        case 6:
            DrawWireFramedBatch(&triangleFanBatch);
            break;
            
            
            
        default:
            break;
    }
    
    // 出栈
    modelViewMatrix.PopMatrix();
        
    // 进行缓冲区交换
    glutSwapBuffers();

}

// 两个作用， 1.设置视图大小 2.设置投影矩阵
void changeSize(int w,int h) {
    
    glViewport(0, 0, w, h);
    //创建投影矩阵，并将它载入投影矩阵堆栈中
    viewFrustum.SetPerspective(35.0f, float(w) / float(h), 1.0f, 500.0f);
    projectionMatrix.LoadMatrix(viewFrustum.GetProjectionMatrix());
    
    //调用顶部载入单元矩阵
    modelViewMatrix.LoadIdentity();
}

/*
    手动仅调用一次
    处理业务：  1、 设置窗口背景颜色  2、初始化存储着色器 shaderManager  3、 设置图形顶点数据  4、 利用GLBatch 三角形批次类 将数据传递到着色器
 */
void setupRC() {

    //设置清屏颜色（背景颜色 灰色）
    glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
    shaderManager.InitializeStockShaders();
    
    // 使用平面着色器
    // 投影矩阵 、移动变换矩阵  --> 变换管道，快速的进行矩阵相乘 这个transforPipline 是需要先在changeSize 函数中设置
    transformPipline.SetMatrixStacks(modelViewMatrix, projectionMatrix);
    
    // 设置观察者距离物体的位置 z  + - 可以理解为移动的方向 竖直代表距离
    cameraFrame.MoveForward(-15.0f);
    
    // 设置定点数据（ 物体坐标系 转为 规范坐标系），然后render scene
    GLfloat vCoast[9] = {
        3,3,0,0,3,0,3,0,0
    };
    pointBatch.Begin(GL_POINTS, 3);
    pointBatch.CopyVertexData3f(vCoast);
    pointBatch.End();
    
    lineBatch.Begin(GL_LINES, 3);
    lineBatch.CopyVertexData3f(vCoast);
    lineBatch.End();
    
    lineStripBatch.Begin(GL_LINE_STRIP, 3);
    lineStripBatch.CopyVertexData3f(vCoast);
    lineStripBatch.End();
    
    lineLoopBatch.Begin(GL_LINE_LOOP, 3);
    lineLoopBatch.CopyVertexData3f(vCoast);
    lineLoopBatch.End();
    
    
    // 通过三角形创建金字塔
    GLfloat vPyramid[12][3] = {
        -2.0f, 0.0f, -2.0f,
        2.0f, 0.0f, -2.0f,
        0.0f, 4.0f, 0.0f,

        2.0f, 0.0f, -2.0f,
        2.0f, 0.0f, 2.0f,
        0.0f, 4.0f, 0.0f,

        2.0f, 0.0f, 2.0f,
        -2.0f, 0.0f, 2.0f,
        0.0f, 4.0f, 0.0f,

        -2.0f, 0.0f, 2.0f,
        -2.0f, 0.0f, -2.0f,
        0.0f, 4.0f, 0.0f
    };
    
    triangleBatch.Begin(GL_TRIANGLES, 12);
    triangleBatch.CopyVertexData3f(vPyramid);
    triangleBatch.End();
    
    // 三角形扇形--六边形
    GLfloat vPoints[100][3];
    int nVerts = 0;
    //半径
    GLfloat r = 3.0f;
    //原点(x,y,z) = (0,0,0);
    vPoints[nVerts][0] = 0.0f;
    vPoints[nVerts][1] = 0.0f;
    vPoints[nVerts][2] = 0.0f;
    
    
    //M3D_2PI 就是2Pi 的意思，就一个圆的意思。 绘制圆形
    for(GLfloat angle = 0; angle < M3D_2PI; angle += M3D_2PI / 6.0f) {
        
        //数组下标自增（每自增1次就表示一个顶点）
        nVerts++;
        /*
         弧长=半径*角度,这里的角度是弧度制,不是平时的角度制
         既然知道了cos值,那么角度=arccos,求一个反三角函数就行了
         */
        //x点坐标 cos(angle) * 半径
        vPoints[nVerts][0] = float(cos(angle)) * r;
        //y点坐标 sin(angle) * 半径
        vPoints[nVerts][1] = float(sin(angle)) * r;
        //z点的坐标
        vPoints[nVerts][2] = -0.5f;
    }
    
    // 结束扇形 前面一共绘制7个顶点（包括圆心）
    //添加闭合的终点
    //课程添加演示：屏蔽177-180行代码，并把绘制节点改为7.则三角形扇形是无法闭合的。
    nVerts++;
    vPoints[nVerts][0] = r;
    vPoints[nVerts][1] = 0;
    vPoints[nVerts][2] = 0.0f;
    
    // 加载！
    //GL_TRIANGLE_FAN 以一个圆心为中心呈扇形排列，共用相邻顶点的一组三角形
    triangleFanBatch.Begin(GL_TRIANGLE_FAN, 8);
    triangleFanBatch.CopyVertexData3f(vPoints);
    triangleFanBatch.End();
    
    //三角形条带，一个小环或圆柱段
    //顶点下标
    int iCounter = 0;
    //半径
    GLfloat radius = 3.0f;
    //从0度~360度，以0.3弧度为步长
    for(GLfloat angle = 0.0f; angle <= (2.0f*M3D_PI); angle += 0.3f)
    {
        //或许圆形的顶点的X,Y
        GLfloat x = radius * sin(angle);
        GLfloat y = radius * cos(angle);
        
        //绘制2个三角形（他们的x,y顶点一样，只是z点不一样）
        vPoints[iCounter][0] = x;
        vPoints[iCounter][1] = y;
        vPoints[iCounter][2] = -0.5;
        iCounter++;
        
        vPoints[iCounter][0] = x;
        vPoints[iCounter][1] = y;
        vPoints[iCounter][2] = 0.5;
        iCounter++;
    }
    
    // 关闭循环
    printf("三角形带的顶点数：%d\n",iCounter);
    //结束循环，在循环位置生成2个三角形
    vPoints[iCounter][0] = vPoints[0][0];
    vPoints[iCounter][1] = vPoints[0][1];
    vPoints[iCounter][2] = -0.5;
    iCounter++;
    
    vPoints[iCounter][0] = vPoints[1][0];
    vPoints[iCounter][1] = vPoints[1][1];
    vPoints[iCounter][2] = 0.5;
    iCounter++;
    
    // GL_TRIANGLE_STRIP 共用一个条带（strip）上的顶点的一组三角形
    triangleStripBatch.Begin(GL_TRIANGLE_STRIP, iCounter);
    triangleStripBatch.CopyVertexData3f(vPoints);
    triangleStripBatch.End();
}

void keyPressFunc(unsigned char key, int x, int y) {
    if (key == 32) { // ASCII 码 回车 13  空格 32  换行 10
        nStep ++;
        if (nStep > 6) {
            nStep = 0;
        }
    }
    
    switch (nStep) {
        case 0:
            glutSetWindowTitle("GL_POINTS");
            break;
        case 1:
            glutSetWindowTitle("GL_LINES");
            break;
        case 2:
            glutSetWindowTitle("GL_LINE_STRIP");
            break;
        case 3:
            glutSetWindowTitle("GL_LINE_LOOP");
            break;
        case 4:
            glutSetWindowTitle("GL_TRIANGLES");
            break;
        case 5:
            glutSetWindowTitle("GL_TRIANGLE_STRIP");
            break;
        case 6:
            glutSetWindowTitle("GL_TRIANGLE_FAN");
            break;
            
        default:
            break;
    }
    glutPostRedisplay();
}



void SpecialKeys(int key, int x, int y) {
    
    // 1.旋转物体
    if (key == GLUT_KEY_UP) { // 围绕x 轴转
        objectFrame.RotateWorld(m3dDegToRad(-5), 1.0f, 0.0f, 0.0f);
    }

    if (key == GLUT_KEY_DOWN) {
        objectFrame.RotateWorld(m3dDegToRad(5), 1.0f, 0.0f, 0.0f);
    }
    
    if (key == GLUT_KEY_LEFT) {
        objectFrame.RotateWorld(m3dDegToRad(-5), 0.0f, 1.0f, 0.0f);
    }
    
    if (key == GLUT_KEY_RIGHT) {
        objectFrame.RotateWorld(m3dDegToRad(5), 0.0f, 1.0f, 0.0f);
    }
    
    // 强制刷新
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
    glutInitWindowSize(800, 600);
    // 创建窗口，并设置标题
    glutCreateWindow("GL_POINTS");

    /*

    GLUT 内部运行一个本地消息循环 ，glutMainLoop，拦截适当的消息。然后调用我们不同时间注册的回调函数。

    */
    
    //注册重塑函数
    glutReshapeFunc(changeSize);
    
    // 获取键盘事件
    glutKeyboardFunc(keyPressFunc);
    
    // 获取特殊键位事件
    glutSpecialFunc(SpecialKeys);

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
