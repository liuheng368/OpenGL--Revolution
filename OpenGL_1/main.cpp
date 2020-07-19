#include "GLShaderManager.h"
#include "GLTools.h"
#include "GLFrustum.h"
#include "GLMatrixStack.h"
#include "GLGeometryTransform.h"
#include "GLBatch.h"
#include <math.h>
#include "StopWatch.h"


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

//绘制内容初始化
void setupRC() {
    //使用RGB颜色清空背景
    glClearColor(0, 0, 0, 1.0);
    //初始化固定着色器管理类
    shaderManager.InitializeStockShaders();
    //打开深度测试
    glEnable(GL_DEPTH_TEST);
    
    //地板
    //该批次类绘制模式为线
    //顶点为324个
    floorBatch.Begin(GL_LINES, 324);
    for(GLfloat x = -20.0; x <= 20.0f; x+= 0.5) {
        floorBatch.Vertex3f(x, -0.55f, 20.0f);
        floorBatch.Vertex3f(x, -0.55f, -20.0f);
        
        floorBatch.Vertex3f(20.0f, -0.55f, x);
        floorBatch.Vertex3f(-20.0f, -0.55f, x);
    }
    floorBatch.End();
    
    //大球
    //iSlices 将图形分为多少片
    //iStacks 将每一层分为多少的图元三角形
    //这两个参数越大图像越细腻；一般规律iStacks是iSlices的两倍
    gltMakeSphere(sphereBatch, 0.4f, 40, 80);
    
    //小球
    gltMakeSphere(sphereSmallBatch, 0.1f, 13, 26);
    //计算小球随机位置
    for (int i=0; i<NUM_SPHERES; i++) {
        //y轴不变，X,Z产生随机值
        GLfloat x = ((GLfloat)((rand() % 400) - 200 ) * 0.1f);
        GLfloat z = ((GLfloat)((rand() % 400) - 200 ) * 0.1f);
        
        spheresLocation[i].SetOrigin(x, 0.0f, z);
    }
    
    //视角默认远离
    cameraFrame.MoveForward(-3.0f);
}

void RenderScene(void) {
    //重置颜色、深度缓存区
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    //1. 颜色(地板,大球颜色,小球颜色)
    static GLfloat vFloorColor[] = {0.0f,1.0f,0.0f,1.0f};
    static GLfloat vTorusColor[] = {1.0f,0.0f,0.0f,1.0f};
    static GLfloat vSpereColor[] = {0.0f,0.0f,1.0f,1.0f};
    
    //2. 定时器
    static CStopWatch rotTimer;
    // 旋转角度
    float yRot = rotTimer.GetElapsedSeconds()*60.0f;
    
    
    M3DMatrix44f cameraMatrix;
    cameraFrame.GetCameraMatrix(cameraMatrix);
    modelViewMatrix.PushMatrix(cameraMatrix);
//    modelViewMatrix.MultMatrix(objctFrame);
    
    //地面
    //确定着色器、MVP矩阵，颜色
    shaderManager.UseStockShader(GLT_SHADER_FLAT, transform.GetModelViewProjectionMatrix(), vFloorColor);
    floorBatch.Draw();
    
    //大球
    //固定光源位置
    M3DVector4f vLightPos = {0,10,5,1};
    modelViewMatrix.PushMatrix();
    //使得整个大球往里平移3.0
    modelViewMatrix.Rotate(yRot, 0, 1, 0);
    shaderManager.UseStockShader(GLT_SHADER_POINT_LIGHT_DIFF, transform.GetModelViewMatrix(),transform.GetProjectionMatrix(),vLightPos,vTorusColor);
    sphereBatch.Draw();
    modelViewMatrix.PopMatrix();
    
    //小球
    for (int i=0; i<NUM_SPHERES; i++) {
        //应用每一个随机位置
        modelViewMatrix.PushMatrix();
        modelViewMatrix.MultMatrix(spheresLocation[i]);
        shaderManager.UseStockShader(GLT_SHADER_POINT_LIGHT_DIFF, transform.GetModelViewMatrix(),transform.GetProjectionMatrix(),vLightPos,vSpereColor);
        sphereSmallBatch.Draw();
        modelViewMatrix.PopMatrix();
    }

    //公转小球
    modelViewMatrix.PushMatrix();
    //先旋转在移动
    modelViewMatrix.Rotate(yRot * -2.0f, 0, 1, 0);
    modelViewMatrix.Translate(0.8f, 0, 0);
    shaderManager.UseStockShader(GLT_SHADER_POINT_LIGHT_DIFF, transform.GetModelViewMatrix(),transform.GetProjectionMatrix(),vLightPos,vSpereColor);
    sphereSmallBatch.Draw();
    modelViewMatrix.PopMatrix();

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
    return 0;
}
