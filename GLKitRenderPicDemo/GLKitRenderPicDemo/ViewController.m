//
//  ViewController.m
//  GLKitRenderPicDemo
//
//  Created by Li King on 2022/9/6.
//

#import "ViewController.h"
#import <OpenGLES/ES3/gl.h>
#import <OpenGLES/ES3/glext.h>

@interface ViewController ()
{
    EAGLContext *context;
    // 固定着色器
    GLKBaseEffect *cEffect;
    
}
@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    
    // 初始化配置
    [self setupConfig];
    // 架子顶点数据
    [self setupVertexData];
    // 加载纹理图片
    [self setupTexture];
    
}

- (void)setupConfig{
    //1 初始化上下文
    context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
    if (context == nil) {
        NSLog(@"create EAGLContext Failed");
    }
    [EAGLContext setCurrentContext:context];
    
    //创建GLKView 渲染的载体
    GLKView *view = (GLKView *)self.view;
    view.context = context;
    view.drawableColorFormat = GLKViewDrawableColorFormatRGBA8888;
    view.drawableDepthFormat = GLKViewDrawableDepthFormat24;
    
    // 设置背景颜色
    glClearColor(1, 1, 1, 1);
}

- (void)setupVertexData {
    //1.设置顶点数组(顶点坐标,纹理坐标)
    /*
     纹理坐标系取值范围[0,1];原点是左下角(0,0);
     故而(0,0)是纹理图像的左下角, 点(1,1)是右上角.
     */
    GLfloat vertexData[] = { // 数据在内存
        
        0.5, -0.5, 0.0f,    1.0f, 0.0f, //右下
        0.5, 0.5,  0.0f,    1.0f, 1.0f, //右上
        -0.5, 0.5, 0.0f,    0.0f, 1.0f, //左上
        
        0.5, -0.5, 0.0f,    1.0f, 0.0f, //右下
        -0.5, 0.5, 0.0f,    0.0f, 1.0f, //左上
        -0.5, -0.5, 0.0f,   0.0f, 0.0f, //左下
    };
    
    // 2 创建顶点缓存区  在GPU
    GLuint bufferID;
    glGenBuffers(1, &bufferID);
    // 绑定顶点缓冲区
    glBindBuffer(GL_ARRAY_BUFFER, bufferID);
    // 将顶点数据从CPU copy 到GPU
    /*
        第一个参数： 目标缓存去
        第二个参数： 顶点数组大小
        第三个参数： 顶点数组指针
        第四个参数： 对缓存区数据的操作， 读、写、copy、draw
     */
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), &vertexData, GL_STATIC_DRAW);
    
    //3.打开读取通道.
    /*
     (1)在iOS中, 默认情况下，出于性能考虑，所有顶点着色器的属性（Attribute）变量都是关闭的.
     意味着,顶点数据在着色器端(服务端)是不可用的. 即使你已经使用glBufferData方法,将顶点数据从内存拷贝到顶点缓存区中(GPU显存中).
     所以, 必须由glEnableVertexAttribArray 方法打开通道.指定访问属性.才能让顶点着色器能够访问到从CPU复制到GPU的数据.
     注意: 数据在GPU端是否可见，即，着色器能否读取到数据，由是否启用了对应的属性决定，这就是glEnableVertexAttribArray的功能，允许顶点着色器读取GPU（服务器端）数据。
   
    (2)方法简介
    glVertexAttribPointer (GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr)
   
    功能: 上传顶点数据到显存的方法（设置合适的方式从buffer里面读取数据）
    参数列表:
        index,指定要修改的顶点属性的索引值,例如
        size, 每次读取数量。（如position是由3个（x,y,z）组成，而颜色是4个（r,g,b,a）,纹理则是2个.）
        type,指定数组中每个组件的数据类型。可用的符号常量有GL_BYTE, GL_UNSIGNED_BYTE, GL_SHORT,GL_UNSIGNED_SHORT, GL_FIXED, 和 GL_FLOAT，初始值为GL_FLOAT。
        normalized,指定当被访问时，固定点数据值是否应该被归一化（GL_TRUE）或者直接转换为固定点值（GL_FALSE）
        stride,步长 指定连续顶点属性之间的偏移量。如果为0，那么顶点属性会被理解为：它们是紧密排列在一起的。初始值为0
        ptr指定一个指针，指向数组中第一个顶点属性的第一个组件。初始值为0 可以直接写0
     */
    
    // 需要手动开启顶点数据读取开关
    glEnableVertexAttribArray(GLKVertexAttribPosition);
    // 读取数据 （显存中数据读取到顶点着色器）
    glVertexAttribPointer(GLKVertexAttribPosition, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, (GLfloat *)NULL + 0);
    
    // 需要手动开启纹理数据读取开关
    glEnableVertexAttribArray(GLKVertexAttribTexCoord0);  // GLKVertexAttribTexCoord0 GLKVertexAttribTexCoord1 最多支持两个纹理
    // 读取纹理数据到片元着色器 纹理坐标从第三个索引开始读取
    glVertexAttribPointer(GLKVertexAttribTexCoord0, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, (GLfloat *)NULL + 3);
}

- (void)setupTexture {
    // 1图片路径
    NSString *filePath = [[NSBundle mainBundle] pathForResource:@"sun" ofType:@"jpeg"];
    // 设置纹理参数  纹理坐标在左下角 图片远点在左上
    NSDictionary *option = [NSDictionary dictionaryWithObjectsAndKeys:@(1),GLKTextureLoaderOriginBottomLeft,nil];
    GLKTextureInfo *info = [GLKTextureLoader textureWithContentsOfFile:filePath options:option error:nil];
    
    // 使用着色器加载图片
    cEffect = [[GLKBaseEffect alloc] init];
    cEffect.texture2d0.enabled = GL_TRUE;
    cEffect.texture2d0.name = info.name;
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect {
    // 清理颜色缓冲区
    glClear(GL_COLOR_BUFFER_BIT);
    
    // 准备绘制
    [cEffect prepareToDraw];
    // 开始绘制 图元链接方式 从0 开始 一共6个顶点
    glDrawArrays(GL_TRIANGLES, 0, 6);
}


@end
