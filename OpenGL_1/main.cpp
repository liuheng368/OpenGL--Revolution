#include "GLShaderManager.h"
#include "GLTools.h"
#include "GLFrustum.h"
#include "GLMatrixStack.h"
#include "GLGeometryTransform.h"
#include "GLBatch.h"
#include <math.h>
#include "StopWatch.h"
#include <stdio.h>

#ifdef __APPLE__
#include <glut/glut.h>
#else
#define FREEGLUT_STATIC
#include <GL/glut.h>
#endif


//投影视图矩阵堆栈
GLMatrixStack projectMatrix;
//模型视图矩阵堆栈
GLMatrixStack modelViewMatrix;
//mvp变换管道
GLGeometryTransform transform;
//固定着色器管理类
GLShaderManager shaderManager;

//参考帧
//观察者帧
GLFrame cameraFrame;
//模型帧
GLFrame objctFrame;

//地板
GLBatch floorBatch;
//大球
GLTriangleBatch sphereBatch;
//小球
GLTriangleBatch sphereSmallBatch;
#define NUM_SPHERES 50
//保存小球随机位置
GLFrame spheresLocation[NUM_SPHERES];

//纹理标记数组
//纹理对象
GLuint texture[3];

bool LoadTGATexture(const char *szFileName, GLenum minFilter, GLenum magFilter, GLenum wrapMode)
{
    GLbyte *pBits;
    int iWidth,iHeight,iComponents;
    GLenum eFormat;
    
    //1.读取纹理数据
    pBits = gltReadTGABits(szFileName, &iWidth, &iHeight, &iComponents, &eFormat);
    if (pBits == NULL) {return false;}
    
    //参数1：纹理维度
    //参数2：线性过滤
    //参数3：wrapMode,环绕模式
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
    
    //2、设置纹理参数
    //参数1：纹理维度
    //参数2：为S/T坐标设置模式
    //参数3：wrapMode,环绕模式
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
    
    //3.载入纹理
    //参数1：纹理维度
    //参数2：mip贴图层次
    //参数3：纹理单元存储的颜色成分（从读取像素图是获得）-将内部参数nComponents改为了通用压缩纹理格式GL_COMPRESSED_RGB
    //参数4：加载纹理宽
    //参数5：加载纹理高
    //参数6：加载纹理的深度
    //参数7：像素数据的数据类型（GL_UNSIGNED_BYTE，每个颜色分量都是一个8位无符号整数）
    //参数8：指向纹理图像数据的指针
    glTexImage2D(GL_TEXTURE_2D, 0, iComponents, iWidth, iHeight, 0, eFormat, GL_UNSIGNED_BYTE, pBits);
    free(pBits);
    
    //只有minFilter 等于以下四种模式，才可以生成Mip贴图
    //GL_NEAREST_MIPMAP_NEAREST具有非常好的性能，并且闪烁现象非常弱
    //GL_LINEAR_MIPMAP_NEAREST常常用于对游戏进行加速，它使用了高质量的线性过滤器
    //GL_LINEAR_MIPMAP_LINEAR 和GL_NEAREST_MIPMAP_LINEAR 过滤器在Mip层之间执行了一些额外的插值，以消除他们之间的过滤痕迹。
    //GL_LINEAR_MIPMAP_LINEAR 三线性Mip贴图。纹理过滤的黄金准则，具有最高的精度。
    if(minFilter == GL_LINEAR_MIPMAP_LINEAR ||
       minFilter == GL_LINEAR_MIPMAP_NEAREST ||
       minFilter == GL_NEAREST_MIPMAP_LINEAR ||
       minFilter == GL_NEAREST_MIPMAP_NEAREST)
    //4.加载Mip,纹理生成所有的Mip层
    //参数：GL_TEXTURE_1D、GL_TEXTURE_2D、GL_TEXTURE_3D
    glGenerateMipmap(GL_TEXTURE_2D);
    
    return true;
}

//绘制内容初始化
void setupRC() {
    //使用RGB颜色清空背景
    glClearColor(0, 0, 0, 1.0);
    //初始化固定着色器管理类
    shaderManager.InitializeStockShaders();
    //打开深度测试
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    
    //地板
    //该批次类绘制模式为线
    GLfloat texSize = 10.0f;
    floorBatch.Begin(GL_TRIANGLE_FAN, 4, 1);
    floorBatch.MultiTexCoord2f(0, 0.0f, 0.0f);
    floorBatch.Vertex3f(-20.f, -0.41f, 20.0f);
    
    floorBatch.MultiTexCoord2f(0, texSize, 0.0f);
    floorBatch.Vertex3f(20.0f, -0.41f, 20.f);
    
    floorBatch.MultiTexCoord2f(0, texSize, texSize);
    floorBatch.Vertex3f(20.0f, -0.41f, -20.0f);
    
    floorBatch.MultiTexCoord2f(0, 0.0f, texSize);
    floorBatch.Vertex3f(-20.0f, -0.41f, -20.0f);

    floorBatch.End();
    
    //大球
    //iSlices 将图形分为多少片
    //iStacks 将每一层分为多少的图元三角形
    //这两个参数越大图像越细腻；一般规律iStacks是iSlices的两倍
    gltMakeSphere(sphereBatch, 0.4f, 40, 80);
    
    //小球
    gltMakeSphere(sphereSmallBatch, 0.1f, 26, 13);
    //计算小球随机位置
    for (int i=0; i<NUM_SPHERES; i++) {
        //y轴不变，X,Z产生随机值
        GLfloat x = ((GLfloat)((rand() % 400) - 200 ) * 0.1f);
        GLfloat z = ((GLfloat)((rand() % 400) - 200 ) * 0.1f);
        
        spheresLocation[i].SetOrigin(x, 0.0f, z);
    }
    
    //视角默认远离
//    cameraFrame.MoveForward(-3.0f);
    
    //绑定纹理对象
    glGenTextures(3, texture);
    
    glBindTexture(GL_TEXTURE_2D, texture[0]);
    //9.将TGA文件加载为2D纹理。
    //参数1：纹理文件名称
    //参数2&参数3：需要缩小&放大的过滤器
    //参数4：纹理坐标环绕模式
    LoadTGATexture("marble.tga", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_REPEAT);
    
    glBindTexture(GL_TEXTURE_2D, texture[1]);
    LoadTGATexture("marslike.tga", GL_LINEAR_MIPMAP_LINEAR,GL_LINEAR, GL_CLAMP_TO_EDGE);
    
    glBindTexture(GL_TEXTURE_2D, texture[2]);
    LoadTGATexture("moonlike.tga", GL_LINEAR_MIPMAP_LINEAR,GL_LINEAR, GL_CLAMP_TO_EDGE);
}

void drawSomething(GLfloat yRot)
{
    //1. 颜色(地板,大球颜色,小球颜色)
    
    static GLfloat vWhite[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    static GLfloat vLightPos[] = { 0.0f, 3.0f, 0.0f, 1.0f };
    
    //小球
    glBindTexture(GL_TEXTURE_2D, texture[2]);
    for (int i=0; i<NUM_SPHERES; i++) {
        //应用每一个随机位置
        modelViewMatrix.PushMatrix();
        modelViewMatrix.MultMatrix(spheresLocation[i]);
        shaderManager.UseStockShader(GLT_SHADER_TEXTURE_POINT_LIGHT_DIFF,
                                     modelViewMatrix.GetMatrix(),
                                     transform.GetProjectionMatrix(),
                                     vLightPos,
                                     vWhite,
                                     0);
        sphereSmallBatch.Draw();
        modelViewMatrix.PopMatrix();
    }
    
    //大球
    modelViewMatrix.Translate(0.0f, 0.2f, -2.5f);
    modelViewMatrix.PushMatrix();
    modelViewMatrix.Rotate(yRot, 0, 1, 0);
    glBindTexture(GL_TEXTURE_2D, texture[1]);
    shaderManager.UseStockShader(GLT_SHADER_TEXTURE_POINT_LIGHT_DIFF,
                                 modelViewMatrix.GetMatrix(),
                                 transform.GetProjectionMatrix(),
                                 vLightPos,
                                 vWhite,
                                 0);
    sphereBatch.Draw();
    modelViewMatrix.PopMatrix();

    //公转小球
    modelViewMatrix.PushMatrix();
    //先旋转在移动
    modelViewMatrix.Rotate(yRot * -2.0f, 0, 1, 0);
    modelViewMatrix.Translate(0.8f, 0, 0);
    glBindTexture(GL_TEXTURE_2D, texture[2]);
    shaderManager.UseStockShader(GLT_SHADER_TEXTURE_POINT_LIGHT_DIFF,
                                 modelViewMatrix.GetMatrix(),
                                 transform.GetProjectionMatrix(),
                                 vLightPos,
                                 vWhite,
                                 0);
    sphereSmallBatch.Draw();
    modelViewMatrix.PopMatrix();
}

void RenderScene(void) {
    //重置颜色、深度缓存区
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    static GLfloat vFloorColor[] = { 1.0f, 1.0f, 0.0f, 0.75f};
    
    //2. 定时器
    static CStopWatch rotTimer;
    // 旋转角度
    float yRot = rotTimer.GetElapsedSeconds()*60.0f;
    
    
    M3DMatrix44f cameraMatrix;
    cameraFrame.GetCameraMatrix(cameraMatrix);
    modelViewMatrix.PushMatrix(cameraMatrix);
    
    //绘制镜面
    modelViewMatrix.PushMatrix();
    modelViewMatrix.Scale(1.0f, -1.0f, 1.0f);
    modelViewMatrix.Translate(0, 0.8, 0);
    
    //指定顺时针为正面
    //由于沿y轴旋转，为了保证绘制正常，需要重新定义正反面
    glFrontFace(GL_CW);
    drawSomething(yRot);
    glFrontFace(GL_CCW);
    
    modelViewMatrix.PopMatrix();
    
    //地面
    //确定着色器、MVP矩阵，颜色
    //开启混合功能(绘制地板)
    glEnable(GL_BLEND);
    //指定glBlendFunc 颜色混合方程式
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glBindTexture(GL_TEXTURE_2D, texture[0]);
    /*
    纹理调整着色器(将一个基本色乘以一个取自纹理的单元nTextureUnit的纹理)
    参数1：GLT_SHADER_TEXTURE_MODULATE
    参数2：模型视图投影矩阵
    参数3：颜色
    参数4：纹理单元（第0层的纹理单元）
    */
    shaderManager.UseStockShader(GLT_SHADER_TEXTURE_MODULATE, transform.GetModelViewProjectionMatrix(), vFloorColor,0);
    floorBatch.Draw();
    glDisable(GL_BLEND);
    
    //绘制上层
    drawSomething(yRot);
    
    modelViewMatrix.PopMatrix();
    glutSwapBuffers();
    glutPostRedisplay();
}

void KeyBoards(int key, int x, int y) {
    float linear = 0.1f;
    float angular = float(m3dDegToRad(5.0f));
    
    if (key == GLUT_KEY_UP) {
        cameraFrame.MoveForward(linear);
    }
    if (key == GLUT_KEY_DOWN) {
        cameraFrame.MoveForward(-linear);
    }
    if (key == GLUT_KEY_F3) {
        cameraFrame.MoveUp(linear);
    }
    if (key == GLUT_KEY_F4) {
        cameraFrame.MoveUp(-linear);
    }
    
    if (key == GLUT_KEY_LEFT) {
        cameraFrame.RotateWorld(angular, 0, 1, 0);
    }
    if (key == GLUT_KEY_RIGHT) {
        cameraFrame.RotateWorld(-angular, 0, 1, 0);
    }
    if(key == GLUT_KEY_F1) {
        cameraFrame.RotateWorld(-angular, 1, 0, 0);
    }
    if(key == GLUT_KEY_F2) {
        cameraFrame.RotateWorld(angular, 1, 0, 0);
    }
}

void ChangeSize(int width, int height) {
    glViewport(0, 0, width, height);
    
    GLFrustum viewFrustum;
    viewFrustum.SetPerspective(35.0f, float(width)/float(height), 1, 100);
    projectMatrix.LoadMatrix(viewFrustum.GetProjectionMatrix());
    
    transform.SetMatrixStacks(modelViewMatrix, projectMatrix);
}

//删除纹理
void ShutdownRC(void)
{
    glDeleteTextures(3, texture);
}

int main(int argc, char* argv[]) {
    //确定环境本地路径
    gltSetWorkingDirectory(argv[0]);
    //glut初始化
    glutInit(&argc, argv);
    //显示模式初始化
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    //窗口大小初始化
    glutInitWindowSize(800, 600);
    //窗口标题初始化
    glutCreateWindow("123");
    glutSetWindowTitle("大小球公、自转");
    
    //设置回调
    //窗口创建、改变
    glutReshapeFunc(ChangeSize);
    //渲染
    glutDisplayFunc(RenderScene);
    //特殊事件
    glutSpecialFunc(KeyBoards);

    //glew初始化
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        fprintf(stderr, "GLEW Error: %s\n", glewGetErrorString(err));
        return 1;
    }
    
    //绘制内容初始化
    setupRC();
    glutMainLoop();
    ShutdownRC();
    return 0;
}
