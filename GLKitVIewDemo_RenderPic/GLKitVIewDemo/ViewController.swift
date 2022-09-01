//
//  ViewController.swift
//  GLKitVIewDemo
//
//  Created by Li King on 2022/9/1.
//

import UIKit

import GLKit.GLKView
import GLKit.GLKViewController
import OpenGLES.ES3.gl
import OpenGLES.ES3.glext

class ViewController: GLKViewController  {

    var context: EAGLContext?
    var cEffect: GLKBaseEffect?   // 着色器
    
    override func viewDidLoad() {
        super.viewDidLoad()
        //1
        setUpConfig()
        
        //
        
    }
    
    func setupVertexData() {
        //1.设置顶点数组(顶点坐标,纹理坐标)
        /*
         纹理坐标系取值范围[0,1];原点是左下角(0,0);
         故而(0,0)是纹理图像的左下角, 点(1,1)是右上角.
         */
        var vertexData = [
        0.5, -0.5, 0.0, 1.0, 0.0//右下
        , 0.5, 0.5, 0.0, 1.0, 1.0//右上
        , -0.5, 0.5, 0.0, 0.0, 1.0//左上
        , 0.5, -0.5, 0.0, 1.0, 0.0//右下
        , -0.5, 0.5, 0.0, 0.0, 1.0//左上
        , -0.5, -0.5, 0.0, 0.0, 0.0]
        
        var bufferID: GLuint = 0
        // 顶点缓存区   -->  GPU
        glGenBuffers(1, &bufferID)
        
        // 2. 绑定顶点缓存去
        glBindBuffer(GLenum(GL_ARRAY_BUFFER), bufferID)
        
        // 3. copy vertex数据到GPU 显存
        glBufferData(GLenum(GL_ARRAY_BUFFER), MemoryLayout.size(ofValue: vertexData), &vertexData, GLenum(GL_STATIC_DRAW))
        
        //4.打开读取通道.
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
            stride,指定连续顶点属性之间的偏移量。如果为0，那么顶点属性会被理解为：它们是紧密排列在一起的。初始值为0
            ptr指定一个指针，指向数组中第一个顶点属性的第一个组件。初始值为0
         */
        
        glEnableVertexAttribArray(<#T##index: GLuint##GLuint#>)

    }
    
    func setUpConfig() {
        // 初始化上下文
        context = EAGLContext(api: .openGLES3)
        // 2.
        if context == nil {
            print("context 为空 创建失败")
        }
        
        // 3.可以多个上下文 当前上下文只有一个 设置当前上下文
        EAGLContext.setCurrent(context)
        
        // 4 GLKView
        let view = self.view as? GLKView
        view?.delegate = self
        if let context = context {
            view?.context = context
        }
        // 设置颜色 深度 模板 数据类型
        view?.drawableColorFormat = .RGBA8888
        view?.drawableDepthFormat = .format24
        view?.drawableStencilFormat = .format8
        
        // 设置背景颜色
        glClearColor(1, 0, 0, 1)
    }
    
    override func glkView(_ view: GLKView, drawIn rect: CGRect) {
        glClear(GLbitfield(GL_COLOR_BUFFER_BIT))
    }
}

