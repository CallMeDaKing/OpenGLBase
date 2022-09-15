//
//  KingView.m
//  GLKitRenderPicDemo
//
//  Created by Li King on 2022/9/14.
//

#import "KingView.h"
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
// 把顶点着色器和片元着色器进行编译链接生成一个 Programe ID

@end

@implementation KingView

- (void)layoutSubviews {
    [self setupLayer];
    [self setupContext];
    [self deleteRenderAndFrameBuffer];
    [self createRenderBuffer];
    [self setupFrameBuffer];
}

- (void)renderLayer {
    glClearColor(0.3, 0.45, 0.5, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    
    CGFloat scale = [[UIScreen mainScreen] scale];
    // 设置视口
    glViewport(self.frame.origin.x * scale, self.frame.origin.y * scale, self.frame.size.width * scale, self.frame.size.height *scale);
    // 读取片元着色器和顶点着色器
    NSString *vertFile = [[NSBundle mainBundle] pathForResource:@"shaderv" ofType:@"vsh"];
    NSString *fragFile = [[NSBundle mainBundle] pathForResource:@"shaderf" ofType:@"fsh"];
    
    // 拿到program
    self.myPrograme = [self loaderShader:vertFile frag:fragFile];
    
    // 链接program
    glLinkProgram(self.myPrograme);
    
    // 获取链接的状态
    GLint linkStatus;
    glGetProgramiv(self.myPrograme, GL_LINK_STATUS, &linkStatus);
    if (linkStatus == GL_FALSE) {
        GLchar meesage[512];
        glGetProgramInfoLog(self.myPrograme, sizeof(meesage), 0, &meesage[0]);
        NSString *messageStr = [NSString stringWithUTF8String:meesage];
        NSLog(@"program link error %@", messageStr);
    }
    
    // link成功 使用program
    glUseProgram(self.myPrograme);
    
    // 准备顶点数据 2 个三角形 6个顶点（取决于顶点装配方式）
    GLfloat attrArr[] = {
        
    };
    
    // 把顶点数据copy到 GPU
    GLuint attbuffer;
    glGenBuffers(1, &attbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, attbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(attrArr), attrArr, GL_DYNAMIC_DRAW);
    
    // 开启顶点和纹理 数据开关 打开通道 首先获取通道id
    GLuint posoition = glGetAttribLocation(self.myPrograme, "position");
    glEnableVertexAttribArray(posoition);
    glVertexAttribPointer(posoition, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, 0);
    
    GLuint texture = glGetAttribLocation(self.myPrograme, "textCoordinate");
    glEnableVertexAttribArray(texture);
    glVertexAttribPointer(texture, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5,  NULL + 3);
    
    // 加载纹理
    
}

// 纹理解压缩
- (void)setupTexture:(NSString *)fileName {
    // 1 纹理解压缩 OpenGL ES
    CGImageRef spriteImage = [UIImage imageNamed:fileName].CGImage;
    
    if (!spriteImage) {
        NSLog(@"Faile to Load image~");
        return;  // exit(1)
    }
    
    // 2 创建上下文
    size_t width = CGImageGetWidth(spriteImage);
    size_t height = CGImageGetHeight(spriteImage);
    GLubyte *spriteData = (GLubyte *)calloc(width * height * 4, sizeof(GLubyte));
    
    CGContextRef spriteContext = CGBitmapContextCreate(spriteData, width, height, 8, width * 4, <#CGColorSpaceRef  _Nullable space#>, <#uint32_t bitmapInfo#>)
}

+ (Class)layerClass {
    return [CAEAGLLayer class];
}

- (void)setupFrameBuffer {
    GLuint buffer;
    glGenFramebuffers(1, &buffer);
    self.myColorFrameBuffer = buffer;
    glBindFramebuffer(GL_FRAMEBUFFER, self.myColorFrameBuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, self.myColorRenderBuffer);
}

// 创建渲染缓冲区 Render Buffer
- (void)createRenderBuffer {
    // 1 定义bufferID
    GLuint buffer;
    glGenRenderbuffers(1, &buffer);
    self.myColorRenderBuffer = buffer;
    // 2. 绑定Rende buffer
    glBindRenderbuffer(GL_RENDERBUFFER, self.myColorRenderBuffer);
    // 3. 将context 和 EagLayer 绑定
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
    EAGLContext *contex = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
    if (contex == nil) {
        return;
    }
    
    BOOL ok = [EAGLContext setCurrentContext:contex];
    if (ok == NO) {
        return;
    }
    self.myContext = contex;
}

// 1 设置图层
- (void)setupLayer {
    // 1.创建图层
    self.myEagLayer = (CAEAGLLayer *)self.layer;
    // 2.设置缩放因子 和屏幕相同
    [self setContentScaleFactor:[[UIScreen mainScreen]scale]];
//    kEAGLDrawablePropertyRetainedBacking ： 绘图完成显示之后 是否保存该内容
//    kEAGLDrawablePropertyColorFormat: 数据类型
    // 3.设置颜色 深度缓冲区
    self.myEagLayer.drawableProperties = @{kEAGLDrawablePropertyRetainedBacking : @(NO),kEAGLDrawablePropertyColorFormat : kEAGLColorFormatRGBA8};
}

#pragma mark -- shader
- (GLuint)loaderShader:(NSString *)vert frag:(NSString *)frag {
    
    // 1. 定义顶点着色器 、 片元着色器对象
    GLuint verShader, fragShader;
    // 2. program对象
    GLuint program = glCreateProgram();
    // 3. 编译顶点、片元着色器
    [self compileShader:&verShader type:GL_VERTEX_SHADER file:vert];
    [self compileShader:&fragShader type:GL_FRAGMENT_SHADER file:frag];
    // 4.编译好的program shader附着到program
    glAttachShader(program, verShader);
    glAttachShader(program, fragShader);
    // 5. 将shader对象删除
    glDeleteShader(verShader);
    glDeleteShader(fragShader);
    
    return program;
}

- (void)compileShader:(GLuint *)shader type:(GLenum)type file:(NSString *)file{
    // 1. 读取文件路径
    NSString *content = [NSString stringWithContentsOfFile:file encoding:NSUTF8StringEncoding error:nil];
    // 转成C字符串
    const GLchar *source = [content UTF8String];
    // 2.创建对应类型shader
    *shader = glCreateShader(type);
    // 3.将着色器代码读取并附着到着色器上
    glShaderSource(*shader, 1, &source, NULL);
    // 4.编译
    glCompileShader(*shader);
}

@end
