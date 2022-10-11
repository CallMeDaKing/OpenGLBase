//
//  KingView.m
//  GLKitRenderPicDemo
//
//  Created by Li King on 2022/9/14.
//

#import "KingView.h"
#import "GLESMath.h"
#import "GLESUtils.h"
#import <OpenGLES/ES2/gl.h>

/*
    不采用固定着色器 GLKBaseEffect 使用编译链接自定义的着色器，用简单的glsl语言来实现顶点、片元着色器，并对图形简单变换

    步骤
    1、创建图层
    2、创建上下文
    3、清空缓存区
    4、设置renderBuffer
    5、设置FrameBuffer
    6、开始绘制
 */

@interface KingView()
// 在iOS和tvOS上 绘制OpenGL ES 内容的图层，继承自CALayer
@property (nonatomic, strong) CAEAGLLayer *myEagLayer;
@property (nonatomic, strong) EAGLContext *myContext;
@property (nonatomic, assign) GLuint myColorRenderBuffer; // 渲染缓冲区
@property (nonatomic, assign) GLuint myColorFrameBuffer;  // 帧缓冲区
@property (nonatomic, assign) GLuint myPrograme;

@property (nonatomic, assign) GLuint myVertices;
// 把顶点着色器和片元着色器进行编译链接生成一个 Programe ID

@end

@implementation KingView {
    float xDegree;
    float yDegree;
    float zDegree;
    BOOL bX;
    BOOL bY;
    BOOL bZ;
    NSTimer *myTimer;
}

- (void)layoutSubviews {
    [self setupLayer];
    [self setupContext];
    [self deleteRenderAndFrameBuffer];
    [self createRenderBuffer];
    [self setupFrameBuffer];
    [self renderLayer];
}

- (void)renderLayer {
    // 设置清屏颜色
    glClearColor(0, 0, 0, 1.0);
    // 清空颜色缓冲区
    glClear(GL_COLOR_BUFFER_BIT);
    // 设置视口
    CGFloat scale = [UIScreen mainScreen].scale;
    glViewport(self.frame.origin.x * scale, self.frame.origin.y * scale, self.frame.size.width * scale, self.frame.size.height * scale);
    // 加载和编译着色器 创建program 对象绑定着色器
    NSString *vertFile = [[NSBundle mainBundle] pathForResource:@"shaderv" ofType:@"vsh"];
    NSString *fragFile = [[NSBundle mainBundle] pathForResource:@"shaderf" ofType:@"fsh"];
    
    if (self.myPrograme) {
        glDeleteProgram(self.myPrograme);
        self.myPrograme = 0;
    }
    self.myPrograme = [self loadShaders:vertFile frag:fragFile];
    // 链接progr恩平-【【【【【【【【【【【am
    glLinkProgram(self.myPrograme);
    
    // 获取链接状态
    GLint linkSuccess;
    glGetProgramiv(self.myPrograme, GL_LINK_STATUS, &linkSuccess);
    if (linkSuccess == GL_FALSE) {
        GLchar message[256];
        
        glGetProgramInfoLog(self.myPrograme, sizeof(message), 0, &message[0]);
        NSString *messageString = [NSString stringWithUTF8String:message];
        NSLog(@"error --%@", messageString);
    } else {
        glUseProgram(self.myPrograme);
    }
    
    //8.创建顶点数组 & 索引数组
    //(1)顶点数组 前3顶点值（x,y,z），后3位颜色值(RGB)
    GLfloat attrArr[] =
    {
        -0.5f, 0.5f, 0.0f,      1.0f, 0.0f, 1.0f, //左上0
        0.5f, 0.5f, 0.0f,       1.0f, 0.0f, 1.0f, //右上1
        -0.5f, -0.5f, 0.0f,     1.0f, 1.0f, 1.0f, //左下2
        
        0.5f, -0.5f, 0.0f,      1.0f, 1.0f, 1.0f, //右下3
        0.0f, 0.0f, 1.0f,       0.0f, 1.0f, 0.0f, //顶点4
    };
    
    //(2).索引数组
    GLuint indices[] =
    {
        0, 3, 2,
        0, 1, 3,
        0, 2, 4,
        0, 4, 1,
        2, 3, 4,
        1, 4, 3,
    };
     
    // 处理顶点数据，判断顶点缓存区是否为空，如果为空申请一个缓冲区
    if (self.myVertices == 0) {
        glGenBuffers(1, &_myVertices);
    }
    // 将顶点数据和 GL_ARRAY_BUFFER 绑定
    glBindBuffer(GL_ARRAY_BUFFER, _myVertices);
    // 将顶点数据从CPU copy到GPU
    glBufferData(GL_ARRAY_BUFFER, sizeof(attrArr), attrArr, GL_DYNAMIC_DRAW);
    
    // 将顶点数据通过myProgram 传递到着色器程序的positon
    /*
        glGetAttribLocation() 用来获取vertex attribute 入口
        数据通过 glVertexAttribPointer 方法传递
        //注意 第二个参数 需要和shaderv中 的属性名position 保持一致 否则读取不到
     */
    GLuint position = glGetAttribLocation(self.myPrograme, "position");
    // 开启顶点数据读取的开关
    glEnableVertexAttribArray(position);
    // 设置顶点数据读取方式
    glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6, NULL);

    // 处理顶点颜色数据
    GLuint positionColor = glGetAttribLocation(self.myPrograme, "positionColor");
    glEnableVertexAttribArray(positionColor);
    glVertexAttribPointer(positionColor, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6, (float *)NULL + 3);
    
    
    // 获取 projectionMatrix 和 modelViewMatrix 索引
    GLuint projectionMatrixSlot = glGetUniformLocation(self.myPrograme, "projectionMatrix");
    GLuint modelViewMatrixSlot = glGetUniformLocation(self.myPrograme, "modelViewMatrix");
    
    float width = self.frame.size.width;
    float height = self.frame.size.height;
    
    // 创建44 矩阵
    KSMatrix4 projectionMatrix;
    ksMatrixLoadIdentity(&projectionMatrix);
    
    float aspect = width / height;  // 宽高比
    ksPerspective(&projectionMatrix, 30, aspect, 5.0f, 20.0f);
    // 将投影矩阵传递到顶点着色器 设置读取方式
    glUniformMatrix4fv(projectionMatrixSlot, 1, GL_FALSE, (GLfloat *)&projectionMatrix.m[0][0]);
    
    // 创建模型视图矩阵
    KSMatrix4 modelViewMatrix;
    ksMatrixLoadIdentity(&modelViewMatrix);
    ksTranslate(&modelViewMatrix, 0.0, 0.0, -10.0);
    
    // 创建旋转矩阵
    KSMatrix4 rotateMatrix;
    ksMatrixLoadIdentity(&rotateMatrix);
    ksRotate(&rotateMatrix, xDegree, 1.0, 0.0, 0.0);
    ksRotate(&rotateMatrix, yDegree, 0.0, 1.0, 0.0);
    ksRotate(&rotateMatrix, zDegree, 0.0, 0.0, 1.0);
    // 将模型视图矩阵和旋转矩阵相乘
    ksMatrixMultiply(&modelViewMatrix, &rotateMatrix, &modelViewMatrix);
    
    glUniformMatrix4fv(modelViewMatrixSlot, 1, GL_FALSE, (GLfloat *)&modelViewMatrix.m[0][0]);
    
    glEnable(GL_CULL_FACE);
    
    // 使用索引绘图
    glDrawElements(GL_TRIANGLES, sizeof(indices) / sizeof(indices[0]), GL_UNSIGNED_INT, indices);

    [self.myContext presentRenderbuffer:GL_RENDERBUFFER];
}
 
- (GLuint)loadShaders:(NSString *)vertFile frag:(NSString *)fragFile {
    // 创建2个临时变量， verShader fragShader;
    GLuint verShader, fragShader;
    GLuint program = glCreateProgram();
    // 编译shader文件
    [self compileShader:&verShader type:GL_VERTEX_SHADER file:vertFile];
    [self compileShader:&fragShader type:GL_FRAGMENT_SHADER file:fragFile];
    
    // 将shader和program 绑定
    glAttachShader(program, verShader);
    glAttachShader(program, fragShader);
    
    // 释放不需要的shader
    glDeleteShader(verShader);
    glDeleteShader(fragShader);
    return program;
}

- (void)compileShader:(GLuint *)shader type:(GLenum)type file:(NSString *)file {
    // 根据文件路径获取shader字符内容
    NSString *content = [NSString stringWithContentsOfFile:file encoding:NSUTF8StringEncoding error:nil];
    // 将字符串转为C语言字符串
    const GLchar *source = (GLchar *)[content UTF8String];
    *shader = glCreateShader(type);
    glShaderSource(*shader, 1, &source, NULL);
    // 把着色器源码编译成目标代码
    glCompileShader(*shader);
}

// 纹理解压缩
- (GLuint)setupTexture:(NSString *)fileName {
    // 1 纹理解压缩: 就是使用 CoreGraphics 重绘一下
    CGImageRef spriteImage = [UIImage imageNamed:fileName].CGImage;
    
    if (!spriteImage) {
        NSLog(@"Faile to Load image~");
        return 0;  // exit(1)
    }
    
    // 2 创建上下文
    size_t width = CGImageGetWidth(spriteImage);
    size_t height = CGImageGetHeight(spriteImage);
    GLubyte *spriteData = (GLubyte *)calloc(width * height * 4, sizeof(GLubyte));
    
    CGContextRef spriteContext = CGBitmapContextCreate(spriteData, width, height, 8, width * 4, CGImageGetColorSpace(spriteImage), kCGImageAlphaPremultipliedLast);
    
    CGRect rect = CGRectMake(0, 0, width, height);
    
    // 使用默认的方式绘制
    CGContextDrawImage(spriteContext, rect, spriteImage);
    
    /*
     因为iOS坐标远点在左上角 而纹理的远点位于左下角，所以会出现渲染出来的图层时上下颠倒的。
     有6种解决方式 进行纹理翻转
     1、 直接修改顶点数据中的顶点对应的纹理坐标y值，在数据源的阶段的时候将纹理和顶点坐标对应，从而实现正常显示
     2、 修改shaderf中纹理坐标的y值 使用（1 - y） （但是会被调用多次， 不推荐）
     3、 修改shaderV中 纹理坐标的y值 这个和2 本质相同 无非修改位置不一样而已
     4、 修改shaderv中的坐标，本质上和修改纹理坐标相同 将Y 坐标进行翻转
        varyTextCoord = textCoordinate;
        vec4 vPos  = position;
        vPos *= vec4(1, -1, 1, 1);
        gl_Position = vPos;
     
     5、 使用矩阵翻转，在link完progrom 后 获取shaderf的矩阵属性使用 glUniformMatrix4fv给shader的矩阵赋值 实现翻转
     6、 在图片加载纹理方法中  该方法常用 实现翻转
     CGContextTranslateCTM(spriteContext, 0, rect.size.height);//向x,平移0,向y平移rect.size.height
     CGContextScaleCTM(spriteContext, 1.0, -1.0); //x,缩放1.0，y,缩放-1.0
     */
    // 翻转纹理
    CGContextTranslateCTM(spriteContext, 0, rect.size.height);
    CGContextScaleCTM(spriteContext, 1.0, -1.0);
    CGContextDrawImage(spriteContext, rect, spriteImage);

    
    // 绘制完毕释放上下文
    CGContextRelease(spriteContext);
    
    // 绑定纹理到默认纹理id  如果只有一个纹理的时候 纹理id为0 可以省略glGenBuffers 该方法
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // 设置纹理属性
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    float fw = width, fh = height;
    
    // 载入纹理
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fw, fh, 0, GL_RGBA, GL_UNSIGNED_BYTE, spriteData);
    
    // 释放 spriteData
    free(spriteData);
    return 0;
}

- (void)setupFrameBuffer {
    
    GLuint buffer;
    glGenFramebuffers(1, &buffer);
    self.myColorFrameBuffer = buffer;
    glBindFramebuffer(GL_FRAMEBUFFER, self.myColorFrameBuffer);
    // 将renderBuffer 绑定到frameBuffer 才可以正常的将渲染内容存放到缓存区
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, self.myColorRenderBuffer);

}

// 创建渲染缓冲区 Render Buffer
- (void)createRenderBuffer {
    
    //1 定义bufferID
    GLuint buffer;
    //2 获取buffer
    glGenRenderbuffers(1, &buffer);
    //3 将buffer和属性绑定
    glBindRenderbuffer(GL_RENDERBUFFER, self.myColorRenderBuffer);
    // 将context 和 CAEAGLayer绑定
    [self.myContext renderbufferStorage:GL_RENDERBUFFER fromDrawable:self.myEagLayer];
}

// 3 清空缓冲区
- (void)deleteRenderAndFrameBuffer {
    // 1 Frame Buffer Object  FBO
    // 2 Render Buffer 可以分为三个类别 颜色缓冲区 深度缓冲区 魔板缓冲区
    glDeleteRenderbuffers(1, &_myColorFrameBuffer);
    self.myColorFrameBuffer = 0;
    
    glDeleteFramebuffers(1, &_myColorFrameBuffer);
    self.myColorFrameBuffer = 0;
}

// 2 设置上下文
- (void)setupContext {
    EAGLContext *context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
    if (context == nil) {
        NSLog(@"set context failed");
        return;
    }
    
    BOOL ok = [EAGLContext setCurrentContext:context];
    
    if (ok == NO) {
        NSLog(@"set current context failed");
        return;
    }
    self.myContext = context;
}

// 1 设置图层
- (void)setupLayer {
//    // 1.创建图层
//    self.myEagLayer = (CAEAGLLayer *)self.layer;
//    // 2.设置缩放因子 和屏幕相同
//    [self setContentScaleFactor:[[UIScreen mainScreen]scale]];
////    kEAGLDrawablePropertyRetainedBacking ： 绘图完成显示之后 是否保存该内容
////    kEAGLDrawablePropertyColorFormat: 数据类型
//    // 3.设置颜色 深度缓冲区
//    self.myEagLayer.drawableProperties = @{kEAGLDrawablePropertyRetainedBacking : @(NO),kEAGLDrawablePropertyColorFormat : kEAGLColorFormatRGBA8};
    
    self.myEagLayer = (CAEAGLLayer *)self.layer;
    [self setContentScaleFactor:[[UIScreen mainScreen] scale]];
    self.myEagLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:[NSNumber numberWithBool:NO], kEAGLDrawablePropertyRetainedBacking, kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat, nil];
    
}

+ (Class)layerClass {
    return [CAEAGLLayer class];
}



#pragma mark --shader
//加载shader
//-(GLuint)loadShaders:(NSString *)vert Withfrag:(NSString *)frag
//{
//    //1.定义2个零时着色器对象
//    GLuint verShader, fragShader;
//    //创建program
//    GLint program = glCreateProgram();
//
//    //2.编译顶点着色程序、片元着色器程序
//    //参数1：编译完存储的底层地址
//    //参数2：编译的类型，GL_VERTEX_SHADER（顶点）、GL_FRAGMENT_SHADER(片元)
//    //参数3：文件路径
//    [self compileShader:&verShader type:GL_VERTEX_SHADER file:vert];
//    [self compileShader:&fragShader type:GL_FRAGMENT_SHADER file:frag];
//
//    //3.创建最终的程序
//    glAttachShader(program, verShader);
//    glAttachShader(program, fragShader);
//
//    //4.释放不需要的shader
//    glDeleteShader(verShader);
//    glDeleteShader(fragShader);
//
//    return program;
//}
//
////编译shader
//- (void)compileShader:(GLuint *)shader type:(GLenum)type file:(NSString *)file{
//
//    //1.读取文件路径字符串
//    NSString* content = [NSString stringWithContentsOfFile:file encoding:NSUTF8StringEncoding error:nil];
//    const GLchar* source = (GLchar *)[content UTF8String];
//
//    //2.创建一个shader（根据type类型）
//    *shader = glCreateShader(type);
//
//    //3.将着色器源码附加到着色器对象上。
//    //参数1：shader,要编译的着色器对象 *shader
//    //参数2：numOfStrings,传递的源码字符串数量 1个
//    //参数3：strings,着色器程序的源码（真正的着色器程序源码）
//    //参数4：lenOfStrings,长度，具有每个字符串长度的数组，或NULL，这意味着字符串是NULL终止的
//    glShaderSource(*shader, 1, &source,NULL);
//
//    //4.把着色器源代码编译成目标代码
//    glCompileShader(*shader);
//
//
//}
@end
