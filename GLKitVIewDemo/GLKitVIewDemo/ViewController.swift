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
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
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
        // 设置背景颜色
        glClearColor(1, 0, 0, 1)
        
    }
    
    override func glkView(_ view: GLKView, drawIn rect: CGRect) {
        glClear(GLbitfield(GL_COLOR_BUFFER_BIT))
    }
}

