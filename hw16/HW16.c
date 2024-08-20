#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glut.h>
#include <GL/gl.h>

int WindW, WindH;
int alpha;

void Reshape(int width, int height) {
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1, 1, -1, 1, 0, 10);
    glMatrixMode(GL_MODELVIEW);

    WindW = width;
    WindH = height;
}

void Draw(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -6.0f); // Отодвигаем камеру дальше от куба

    glRotatef(alpha, 1.0f, 1.0f, 0.0f); // Вращение куба по трем осям
    alpha += 2;
    if (alpha > 359) alpha = 0;

    glBegin(GL_QUADS);

    // Front face
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-0.5f, -0.5f,  0.5f);
    glVertex3f( 0.5f, -0.5f,  0.5f);
    glVertex3f( 0.5f,  0.5f,  0.5f);
    glVertex3f(-0.5f,  0.5f,  0.5f);

    // Back face
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glVertex3f(-0.5f,  0.5f, -0.5f);
    glVertex3f( 0.5f,  0.5f, -0.5f);
    glVertex3f( 0.5f, -0.5f, -0.5f);

    // Top face
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(-0.5f,  0.5f, -0.5f);
    glVertex3f(-0.5f,  0.5f,  0.5f);
    glVertex3f( 0.5f,  0.5f,  0.5f);
    glVertex3f( 0.5f,  0.5f, -0.5f);

    // Bottom face
    glColor3f(1.0f, 1.0f, 0.0f);
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glVertex3f( 0.5f, -0.5f, -0.5f);
    glVertex3f( 0.5f, -0.5f,  0.5f);
    glVertex3f(-0.5f, -0.5f,  0.5f);

    // Right face
    glColor3f(1.0f, 0.0f, 1.0f);
    glVertex3f( 0.5f, -0.5f, -0.5f);
    glVertex3f( 0.5f,  0.5f, -0.5f);
    glVertex3f( 0.5f,  0.5f,  0.5f);
    glVertex3f( 0.5f, -0.5f,  0.5f);

    // Left face
    glColor3f(0.0f, 1.0f, 1.0f);
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glVertex3f(-0.5f, -0.5f,  0.5f);
    glVertex3f(-0.5f,  0.5f,  0.5f);
    glVertex3f(-0.5f,  0.5f, -0.5f);

    glEnd();
    
    glFlush();
    glutSwapBuffers();
}

void Visibility(int state) {
    if (state == GLUT_NOT_VISIBLE) printf("Window not visible!\n");
    if (state == GLUT_VISIBLE) printf("Window visible!\n");
}

void timf(int value) {
    glutPostRedisplay();
    glutTimerFunc(40, timf, 0);
}

void keyboard(unsigned char key, int x, int y) {
    if (key == 27) {  // Если нажата клавиша Esc (код 27)
        exit(0);      // Закрытие приложения
    }
}

int main(int argc, char *argv[]) {
    WindW = 400;
    WindH = 300;
    alpha = 0;

    glutInit(&argc, argv);
    glutInitWindowSize(WindW, WindH);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutCreateWindow("OTUS OpenGL");

    glEnable(GL_DEPTH_TEST); // Включаем тест глубины для корректного отображения объемных объектов

    glutReshapeFunc(Reshape);
    glutDisplayFunc(Draw);
    glutTimerFunc(40, timf, 0);
    glutVisibilityFunc(Visibility);
    glutKeyboardFunc(keyboard); // Обработка клавиатуры для выхода по Esc

    glClearColor(1, 1, 1, 0);

    glutMainLoop();

    return 0;
}
