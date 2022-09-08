//
//  ViewController.m
//  GLKitRenderPicDemo
//
//  Created by Li King on 2022/9/6.
//


#import <GLKit/GLKit.h>
#import "CubeViewController.h"

typedef struct {
    GLKVector3 positionCoord;  // 顶点坐标
    GLKVector2 textureCoord;   // 纹理坐标
    GLKVector3 normal;         // 法线
} CCVectex;

static NSInteger const kCoordCount = 36;

@interface CubeViewController () <GLKViewDelegate>

@property (nonatomic, strong) GLKView* glkView;
@property (nonatomic, strong) GLKBaseEffect *baseEffect;
@property (nonatomic, assign) CCVectex *vertices;

@property (nonatomic, strong) CADisplayLink *dispalyLink;
@property (nonatomic, assign) NSInteger angle;
@property (nonatomic, assign) GLuint vertexBuffer;

@end

@implementation CubeViewController

- (void)dealloc {
    if ([EAGLContext currentContext] == self.glkView.context) {
        [EAGLContext setCurrentContext:nil];
    }
    
    if (_vertices) {
        free(_vertices);
        _vertices = nil;
    }
    
    if (_vertexBuffer) {
        glDeleteBuffers(1, &_vertexBuffer);
        _vertexBuffer = 0;
    }
    [self.dispalyLink invalidate];
}

- (void)viewDidLoad {
    [super viewDidLoad];
    
    self.view.backgroundColor = [UIColor blackColor];
    
    // 初始化OpenGL ES
    [self commonInit];
    
    [self addDispalyLink];
    
}

- (void)addDispalyLink {
    self.angle = 0;
    self.dispalyLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(update)];
    [self.dispalyLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
}

- (void)commonInit {
    // 初始化contex 并设置当前contenx
    EAGLContext *contex = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
    [EAGLContext setCurrentContext:contex];
    
    CGRect frame = CGRectMake(0, 0, self.view.frame.size.width, self.view.frame.size.width);
    self.glkView = [[GLKView alloc] initWithFrame:frame context:contex];
    self.glkView.backgroundColor = [UIColor clearColor];
    self.glkView.delegate = self;
    
    // 使用深度缓存
    self.glkView.drawableDepthFormat = GLKViewDrawableDepthFormat24;
    // 默认 0，1 这里反转z周 使正方形朝外
    glDepthRangef(1, 0);
    
    [self.view addSubview:self.glkView];
    
    // 获取纹理图片
    NSString *imagePath = [[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@"sun.jpeg"];
    UIImage *image = [UIImage imageWithContentsOfFile:imagePath];
    
    // 设置纹理参数
    NSDictionary *options = @{GLKTextureLoaderOriginBottomLeft : @(YES)};
    GLKTextureInfo *textureInfo = [GLKTextureLoader textureWithCGImage:[image CGImage] options:options error:NULL];
    
    // 使用固定着色器 baseEffect
    self.baseEffect = [[GLKBaseEffect alloc] init];
    self.baseEffect.texture2d0.enabled = YES;
    self.baseEffect.texture2d0.name = textureInfo.name;
    self.baseEffect.texture2d0.target = textureInfo.target;
    // 开启光照
    self.baseEffect.light0.enabled = YES;
    // 漫反射颜色
    self.baseEffect.light0.diffuseColor = GLKVector4Make(1, 1, 1, 1);
    // 光源位置
    self.baseEffect.light0.position = GLKVector4Make(-0.5, -0.5, 5, 1);
    
    /*
     解释一下:
     这里我们不复用顶点，使用每 3 个点画一个三角形的方式，需要 12 个三角形，则需要 36 个顶点
     以下的数据用来绘制以（0，0，0）为中心，边长为 1 的立方体
     */
    
    self.vertices = malloc(sizeof(CCVectex) * kCoordCount);
    // 前面
    self.vertices[0] = (CCVectex){{-0.5, 0.5, 0.5}, {0, 1}, {0, 0, 1}};
    self.vertices[1] = (CCVectex){{-0.5, -0.5, 0.5}, {0, 0}, {0, 0, 1}};
    self.vertices[2] = (CCVectex){{0.5, 0.5, 0.5}, {1, 1}, {0, 0, 1}};
    self.vertices[3] = (CCVectex){{-0.5, -0.5, 0.5}, {0, 0}, {0, 0, 1}};
    self.vertices[4] = (CCVectex){{0.5, 0.5, 0.5}, {1, 1}, {0, 0, 1}};
    self.vertices[5] = (CCVectex){{0.5, -0.5, 0.5}, {1, 0}, {0, 0, 1}};
    
    // 上面
    self.vertices[6] = (CCVectex){{0.5, 0.5, 0.5}, {1, 1}, {0, 1, 0}};
    self.vertices[7] = (CCVectex){{-0.5, 0.5, 0.5}, {0, 1}, {0, 1, 0}};
    self.vertices[8] = (CCVectex){{0.5, 0.5, -0.5}, {1, 0}, {0, 1, 0}};
    self.vertices[9] = (CCVectex){{-0.5, 0.5, 0.5}, {0, 1}, {0, 1, 0}};
    self.vertices[10] = (CCVectex){{0.5, 0.5, -0.5}, {1, 0}, {0, 1, 0}};
    self.vertices[11] = (CCVectex){{-0.5, 0.5, -0.5}, {0, 0}, {0, 1, 0}};
    
    // 下面
    self.vertices[12] = (CCVectex){{0.5, -0.5, 0.5}, {1, 1}, {0, -1, 0}};
    self.vertices[13] = (CCVectex){{-0.5, -0.5, 0.5}, {0, 1}, {0, -1, 0}};
    self.vertices[14] = (CCVectex){{0.5, -0.5, -0.5}, {1, 0}, {0, -1, 0}};
    self.vertices[15] = (CCVectex){{-0.5, -0.5, 0.5}, {0, 1}, {0, -1, 0}};
    self.vertices[16] = (CCVectex){{0.5, -0.5, -0.5}, {1, 0}, {0, -1, 0}};
    self.vertices[17] = (CCVectex){{-0.5, -0.5, -0.5}, {0, 0}, {0, -1, 0}};
    
    // 左面
    self.vertices[18] = (CCVectex){{-0.5, 0.5, 0.5}, {1, 1}, {-1, 0, 0}};
    self.vertices[19] = (CCVectex){{-0.5, -0.5, 0.5}, {0, 1}, {-1, 0, 0}};
    self.vertices[20] = (CCVectex){{-0.5, 0.5, -0.5}, {1, 0}, {-1, 0, 0}};
    self.vertices[21] = (CCVectex){{-0.5, -0.5, 0.5}, {0, 1}, {-1, 0, 0}};
    self.vertices[22] = (CCVectex){{-0.5, 0.5, -0.5}, {1, 0}, {-1, 0, 0}};
    self.vertices[23] = (CCVectex){{-0.5, -0.5, -0.5}, {0, 0}, {-1, 0, 0}};
    
    // 右面
    self.vertices[24] = (CCVectex){{0.5, 0.5, 0.5}, {1, 1}, {1, 0, 0}};
    self.vertices[25] = (CCVectex){{0.5, -0.5, 0.5}, {0, 1}, {1, 0, 0}};
    self.vertices[26] = (CCVectex){{0.5, 0.5, -0.5}, {1, 0}, {1, 0, 0}};
    self.vertices[27] = (CCVectex){{0.5, -0.5, 0.5}, {0, 1}, {1, 0, 0}};
    self.vertices[28] = (CCVectex){{0.5, 0.5, -0.5}, {1, 0}, {1, 0, 0}};
    self.vertices[29] = (CCVectex){{0.5, -0.5, -0.5}, {0, 0}, {1, 0, 0}};
    
    // 后面
    self.vertices[30] = (CCVectex){{-0.5, 0.5, -0.5}, {0, 1}, {0, 0, -1}};
    self.vertices[31] = (CCVectex){{-0.5, -0.5, -0.5}, {0, 0}, {0, 0, -1}};
    self.vertices[32] = (CCVectex){{0.5, 0.5, -0.5}, {1, 1}, {0, 0, -1}};
    self.vertices[33] = (CCVectex){{-0.5, -0.5, -0.5}, {0, 0}, {0, 0, -1}};
    self.vertices[34] = (CCVectex){{0.5, 0.5, -0.5}, {1, 1}, {0, 0, -1}};
    self.vertices[35] = (CCVectex){{0.5, -0.5, -0.5}, {1, 0}, {0, 0, -1}};
    
    // 获取缓存区
    glGenBuffers(1, &_vertexBuffer);
    // 绑定缓存区
    glBindBuffer(GL_ARRAY_BUFFER, _vertexBuffer);
    // 将cpu顶点数据copy到GPU
    GLsizeiptr bufferSizeBytes = sizeof(CCVectex) * kCoordCount;
    glBufferData(GL_ARRAY_BUFFER, bufferSizeBytes, self.vertices, GL_STATIC_DRAW);
    
    // 开启顶点数据开关
    glEnableVertexAttribArray(GLKVertexAttribPosition);
    glVertexAttribPointer(GLKVertexAttribPosition, 3, GL_FLOAT, GL_FALSE, sizeof(CCVectex), NULL + offsetof(CCVectex,positionCoord));
    
    // 开启纹理数据开关
    glEnableVertexAttribArray(GLKVertexAttribTexCoord0);
    glVertexAttribPointer(GLKVertexAttribTexCoord0, 2, GL_FLOAT, GL_FALSE, sizeof(CCVectex), NULL + offsetof(CCVectex,textureCoord));
    
    // 开启法线数据开关
    glEnableVertexAttribArray(GLKVertexAttribNormal);
    glVertexAttribPointer(GLKVertexAttribNormal, 3, GL_FLOAT, GL_FALSE, sizeof(CCVectex), NULL + offsetof(CCVectex,normal));
}

#pragma  mark -- GLKViewDelegate
- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect {
    
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    
    // 准备绘制
    [self.baseEffect prepareToDraw];
    // 开始绘图
    glDrawArrays(GL_TRIANGLES, 0, kCoordCount);
}

#pragma mark -- CADisplayLink Event
- (void)update {
    self.angle = (self.angle + 2) % 360;
    // 修改模型视图矩阵
    self.baseEffect.transform.modelviewMatrix = GLKMatrix4MakeRotation(GLKMathDegreesToRadians(self.angle), 0.3, 0.5, 0.7);
    [self.glkView display];
}

@end
