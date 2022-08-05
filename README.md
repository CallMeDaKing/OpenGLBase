# OpenGLBase
OpenGL 基础环境搭建

#相关概念笔记

CPU GPU  区别
CPU 核心计算 并发，时间切片。 包含计算单元和控制单元等，依赖性比较强，逻辑复杂， 综合类型的处理器
GPU 高并发，依赖性低，绘图运算的微处理器


显示器显示 随机扫描 光栅化扫描 1s 16帧即可保证视觉上的流畅性

视觉停留 
观察事务时，光信号传入大脑神经需要短暂时间，光信号消失后，视觉效果并不会立刻消失，这种残留视觉成为视觉停留

显示器 显示内容，数据是直接从 显存（帧缓存区）直接获取
帧缓存区 存储位图，又叫显存
视频控制器 控制刷新部件，用于控制显示器和帧缓存区数据交换，控制画面的显示
内存 连续的计算机存储器，存储刷新图像信息
GPU渲染,将数据放入帧缓存区，视频控制器读取帧缓存区，进行数模转化（数字信号转模拟信号），逐行扫描显示

垂直信号同步  双缓存 
双缓存： 
1、用于处理数据丢失，以防处理慢的一方，数据丢失，比如数据的传输
2、用于提高一方的处理效率，比如在CPU 和内存之间 有高级缓存区， CPU 处理效率高，但是内存读取相对比较低,中间使用高级缓存区 作为协调两者的处理效率
3、解决画面撕裂效果 配合垂直信号同步 ，但是会有掉帧的情况存在

三缓存： 也不能完全接解决掉帧， 但概率较低

CALayer: 渲染和动画

UIView:  对CALayer的封装 有三个作用 ：用于处理交互事件 、 用于管理子view、 用于绘制和动画 监听了layer的 delegate

之所以Lazyer 只负责渲染和动画， 因为UIKit和APPKit 都是基于CALyer， 做了封装，所有的布局和事件处理不同，所以分别有这两个UI框架去处理，而底层的渲染绘制逻辑相同， 这块体现了职责分离，单一性处理原则

图像显示的大概流程

CoreAnimation 层工作
UI 层事件hand event -> 将图片在CPU解码，提交到Render Server,注册Layer 绘制代理回调，

GPU的工作
coreAnimation 将解码后的数据提交到OpenGL ，OpenGL 驱动GPU 进行渲染（提交到顶点着色器，处理定点数据，光栅化图元装配，构建骨架，填充像素，再经过片元着色器计算像素，经下一个runloop 进行显示）

离屏渲染： 
    在处理包含多个layer图层的图片时，要设置圆角或者裁剪， 需要将每个单一的图层进行分批次绘制，存放到开辟的缓冲区，然后等所有的图层绘制完毕，一次性展示到屏幕上， 这个存储中间图层的缓冲区，叫offScreen buffer。 这种绘制方式 就是离屏渲染
    
离屏显然的本质是：需要绘制多个图层来组合出一张图片，需要开辟缓冲区存放中间图层

maskToBounds 和 clipsToBounds 一定会触发离屏渲染吗？ 

答案是否定的。 还是要考虑到本质， 需要绘制多个图层时才会触发， 如果仅是一个图层 即使使用这两个属性 ，也不会出现离屏渲染。

触发离屏渲染的方式：
1、设置圆角 maskToBounds 和 clipsToBounds
2、设置图层layer.mask
3、设置阴影 shadow （增加了投影）
4、绘制了文字的layer, 富文本
5、开启光栅化 shouldRasterization  
  （补充 光栅化两个步骤： 绘制图形骨骼 填充的对应像素）
    什么情况下不需要开启光栅化？
    1> 如果layer 不能被复用，没必要开启光栅化
    2> 如果layer 不是静态的，需要频繁修改，比如动画之中，开启反而影响效率
    3> 离屏渲染缓存内容有时间限制，缓存内容100ms，超过这个时间会被丢弃
    4> 离屏渲染缓存空间有限， 超过屏幕2.5倍像素大小，也会失效无法复用

圆角的处理方案：
1.设置laye.cornerRadius  clipsToBounds
2.贝塞尔绘制圆角
3.使用UI切好的圆角图片
4.在图层上添加一个圆角图层


出现屏幕撕裂 和 掉帧的根本原因  https://segmentfault.com/a/1190000023213674
