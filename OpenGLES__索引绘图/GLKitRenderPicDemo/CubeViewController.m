//
//  ViewController.m
//  GLKitRenderPicDemo
//
//  Created by Li King on 2022/9/6.
//


#import <GLKit/GLKit.h>
#import "CubeViewController.h"


@interface CubeViewController ()

@property(nonnull,strong)KingView *myView;

@end

@implementation CubeViewController


- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
    
    self.myView = (KingView *)self.view;
}



@end
