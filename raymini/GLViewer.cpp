// *********************************************************
// OpenGL Viewer Class, based on LibQGLViewer, compatible
// with most hardware (OpenGL 1.2).
// Author : Tamy Boubekeur (boubek@gmail.com).
// Copyright (C) 2012 Tamy Boubekeur.
// All rights reserved.
// *********************************************************

#include "GLViewer.h"

#include <iostream>
#include <QMouseEvent>

#include "RayTracer.h"
#include "Controller.h"
#include "Light.h"
#include "KDtree.h"
#include "Object.h"
#include "PBGI.h"

using namespace std;

static const GLuint OpenGLLightID[] = {GL_LIGHT0, GL_LIGHT1, GL_LIGHT2, GL_LIGHT3, GL_LIGHT4, GL_LIGHT5, GL_LIGHT6, GL_LIGHT7};

GLViewer::GLViewer(Controller *c):
    QGLViewer(),
    controller(c),
    focusBlinkOn(true)
{}

GLViewer::~GLViewer ()
{}

QString GLViewer::helpString() const {
    QString text("<h2>Raymini</h2>");
    text += "Author: <b>Tamy Boubekeur</b> (boubek@gmail.com)<br>Version: 0.1<br<br>";
    text += "<h3>Disclaimer</h3>";
    text += "This code is not bug-free, use it at your own risks.";
    text += "<h3>Controls</h3>";
    text += "Use the right control panel to setup rendering options.";
    text += "You can respectively zoom and translate with the left and middle mouse buttons. ";
    text += "Pressing <b>Alt</b> and one of the function keys (<b>F1</b>..<b>F12</b>) defines a camera keyFrame. ";
    text += "Simply press the function key again to restore it. Several keyFrames define a ";
    text += "camera path. Paths are saved when you quit the application and restored at next start.<br><br>";
    text += "Press <b>F</b> to display the frame rate, <b>A</b> for the world axis, ";
    text += "<b>Alt+Return</b> for full screen mode and <b>Control+S</b> to save a snapshot. ";
    text += "See the <b>Keyboard</b> tab in this window for a complete shortcut list.<br><br>";
    text += "Double clicks automates single click actions: A left button double click aligns the closer axis with the camera (if close enough). ";
    text += "A middle button double click fits the zoom of the camera and the right button re-centers the scene.<br><br>";
    text += "A left button double click while holding right button pressed defines the camera <i>Revolve Around Point</i>. ";
    text += "See the <b>Mouse</b> tab and the documentation web pages for details.<br><br>";
    text += "Press <b>Escape</b> to exit the viewer.";
    return text;
}

void GLViewer::keyPressEvent (QKeyEvent * /*event*/) {

}

void GLViewer::keyReleaseEvent (QKeyEvent * /*event*/) {

}

void GLViewer::getCameraInformation(float &fov, float &aspectRatio, float &width, float &height, Vec3Df &camPos, Vec3Df &viewDirection, Vec3Df &upVector, Vec3Df &rightVector) {
    qglviewer::Camera * cam = camera ();
    qglviewer::Vec p = cam->position ();
    qglviewer::Vec d = cam->viewDirection ();
    qglviewer::Vec u = cam->upVector ();
    qglviewer::Vec r = cam->rightVector ();
    camPos = Vec3Df(p[0], p[1], p[2]);
    viewDirection = Vec3Df(d[0], d[1], d[2]);
    upVector = Vec3Df(u[0], u[1], u[2]);
    rightVector = Vec3Df(r[0], r[1], r[2]);
    fov = cam->fieldOfView ();
    aspectRatio = cam->aspectRatio ();
    width = cam->screenWidth ();
    height = cam->screenHeight ();
}

void GLViewer::mousePressEvent(QMouseEvent * event) {
    const WindowModel *windowModel = controller->getWindowModel();
    if (windowModel->isDragEnabled()) {
        float fov, ar;
        float screenWidth;
        float screenHeight;
        Vec3Df camPos;
        Vec3Df viewDirection;
        Vec3Df upVector;
        Vec3Df rightVector;
        getCameraInformation(fov, ar, screenWidth, screenHeight, camPos, viewDirection, upVector, rightVector);
        float tanX = tan(fov)*ar;
        float tanY = tan(fov);
        Vec3Df stepX = (float (event->x()) - screenWidth/2.f)/screenWidth * tanX * rightVector;
        Vec3Df stepY = (float (screenHeight-event->y()) - screenHeight/2.f)/screenHeight * tanY * upVector;
        Vec3Df step = stepX + stepY;
        Vec3Df dir = viewDirection + step;
        float distanceCameraScreen = dir.getLength();
        dir.normalize();
        Ray ray;
        if (controller->getRayTracer()->intersect(dir, camPos, ray)) {
            Object *o=ray.getIntersectedObject();
            QPoint p = event->globalPos();
            Vec3Df oPos = o->getTrans();
            float ratio = distanceCameraScreen/sqrt(ray.getIntersectionDistance());
            controller->viewerStartsDragging(o, oPos, p, ratio);
        }
    }
    if (!controller->getWindowModel()->isRealTime()) {
        controller->viewerSetDisplayMode(WindowModel::OpenGLDisplayMode);
    }
    else {
        controller->forceThreadUpdate();
    }
    QGLViewer::mousePressEvent(event);
}

void GLViewer::mouseReleaseEvent (QMouseEvent *event) {
    const RayTracer *rayTracer = controller->getRayTracer();
    const WindowModel *windowModel = controller->getWindowModel();
    if (rayTracer->getTypeFocus() != Focus::NONE && windowModel->isFocusMode()) {
        controller->viewerSetFocusPoint(currentFocusPoint);
    }
    controller->viewerStopsDragging();
    QGLViewer::mouseReleaseEvent(event);
}

void GLViewer::mouseMoveEvent(QMouseEvent *event) {
    const WindowModel *windowModel = controller->getWindowModel();
    if (windowModel->isDragging()) {
        controller->viewerMovesWhileDragging(event->globalPos());
    } else {
        QGLViewer::mouseMoveEvent(event);
        controller->viewerMovesMouse();
    }
}


void GLViewer::wheelEvent (QWheelEvent * e) {
    if (!controller->getWindowModel()->isRealTime()) {
        controller->viewerSetDisplayMode(WindowModel::OpenGLDisplayMode);
    }
    else {
        controller->forceThreadUpdate();
    }
    QGLViewer::wheelEvent (e);
}

void GLViewer::updateLights(const Observable *observable) {
    const Scene * scene = controller->getScene();
    if (observable == scene &&
            scene->isChanged(Scene::LIGHT_CHANGED)) {
        for (unsigned int i = 0; i < scene->getLights ().size () && i < 8; i++) {
            const Light * light = scene->getLights() [i];
            GLuint glID = OpenGLLightID[i];
            if (!light->isEnabled()) {
                glDisable(glID);
            }
            else {
                glEnable (glID);
                const Vec3Df & p = light->getPos ();
                float intensity = light->getIntensity ();
                const Vec3Df & c = intensity * light->getColor ();
                GLfloat glPos[4] = {p[0], p[1], p[2], 1};
                GLfloat glColor[4] = {c[0], c[1], c[2], 0};
                glLightfv (glID, GL_POSITION, glPos);
                glLightfv (glID, GL_DIFFUSE, glColor);
            }
        }
    }
}

void GLViewer::updateWireframe(const Observable *observable) {
    const WindowModel *windowModel = controller->getWindowModel();
    if (observable == windowModel &&
            (windowModel->isChanged(WindowModel::WIREFRAME_CHANGED) ||
             windowModel->isChanged(WindowModel::DISPLAY_MODE_CHANGED))) {
        if (windowModel->getDisplayMode() != WindowModel::RayDisplayMode) {
            if (windowModel->isWireframe()) {
                glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
            }
            else {
                glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
            }
        }
    }
}

void GLViewer::updateBackground(const Observable *observable) {
    const RayTracer *rayTracer = controller->getRayTracer();
    if (observable == rayTracer &&
            rayTracer->isChanged(RayTracer::BACKGROUND_CHANGED)) {
        Vec3Df bgc = 255.0*rayTracer->getBackgroundColor();
        QColor c(bgc[0], bgc[1], bgc[2]);
        setBackgroundColor(c);
    }
}

void GLViewer::updateFocus(const Observable *observable) {
    const WindowModel *windowModel = controller->getWindowModel();
    if (windowModel == observable &&
            windowModel->isChanged(WindowModel::FOCUS_MODE_CHANGED)) {
        if (windowModel->isFocusMode()) {
            startAnimation();
        }
        else {
            stopAnimation();
        }
    }
}

void GLViewer::updateView(const Observable *observable) {
    bool haveToRedraw = false;

    if (!haveToRedraw &&
            observable == controller->getRayTracer() &&
            (observable->isChanged(RayTracer::BACKGROUND_CHANGED))) {
        haveToRedraw = true;
    }

    if (!haveToRedraw &&
            observable == controller->getScene() &&
            (observable->isChanged(Scene::OBJECT_CHANGED) ||
             observable->isChanged(Scene::LIGHT_CHANGED) ||
             observable->isChanged(Scene::MATERIAL_CHANGED) ||
             observable->isChanged(Scene::COLOR_TEXTURE_CHANGED) ||
             observable->isChanged(Scene::NORMAL_TEXTURE_CHANGED))) {
        haveToRedraw = true;
    }

    if (!haveToRedraw &&
            observable == controller->getWindowModel() &&
            (observable->isChanged(WindowModel::SELECTED_LIGHT_CHANGED) ||
             observable->isChanged(WindowModel::WIREFRAME_CHANGED) ||
             observable->isChanged(WindowModel::SHOW_SURFELS_CHANGED) ||
             observable->isChanged(WindowModel::SHOW_KDTREE_CHANGED) ||
             observable->isChanged(WindowModel::RENDERING_MODE_CHANGED) ||
             observable->isChanged(WindowModel::DISPLAY_MODE_CHANGED) ||
             observable->isChanged(WindowModel::RAY_IMAGE_CHANGED) ||
             observable->isChanged(WindowModel::FOCUS_POINT_CHANGED) ||
             observable->isChanged(WindowModel::FOCUS_MODE_CHANGED))) {
        haveToRedraw = true;
    }

    if (!haveToRedraw &&
            observable == controller->getPBGI() &&
            (observable->isChanged(PBGI::PBGI_CHANGED))) {
        haveToRedraw = true;
    }

    if (haveToRedraw) {
        updateGL();
    }
}

void GLViewer::update(const Observable *o) {
    updateLights(o);
    updateBoundingBox(o);
    updateWireframe(o);
    updateFocus(o);
    updateBackground(o);
    updateView(o);
}

void GLViewer::updateBoundingBox(const Observable *observable) {
    const Scene *scene = controller->getScene();
    if (observable == scene &&
            scene->isChanged(Scene::BOUNDING_BOX_CHANGED)) {
        const BoundingBox & sceneBBox = scene->getBoundingBox();
        Vec3Df c = sceneBBox.getCenter ();
        float r = sceneBBox.getRadius ();
        setSceneCenter(qglviewer::Vec (c[0], c[1], c[2]));
        setSceneRadius(r);
    }
}

void GLViewer::changeFocusPoint() {
    const WindowModel *windowModel = controller->getWindowModel();
    bool focusMode = windowModel->isFocusMode();
    if (focusMode) {
        const RayTracer * rayTracer = controller->getRayTracer();
        qglviewer::Camera * cam = camera ();
        qglviewer::Vec p = cam->position ();
        qglviewer::Vec d = cam->viewDirection ();
        qglviewer::Vec u = cam->upVector ();
        qglviewer::Vec r = cam->rightVector ();
        Vec3Df rightVector (r[0], r[1], r[2]);
        Vec3Df upVector (u[0], u[1], u[2]);
        Vec3Df camPos (p[0], p[1], p[2]);
        Vec3Df viewDirection (d[0], d[1], d[2]);
        Ray focusSelect = Ray(camPos, viewDirection);
        if (rayTracer->intersect(viewDirection, camPos, focusSelect)) {
            currentFocusPoint = focusSelect.getIntersection();
        }
    }
}

// -----------------------------------------------
// Drawing functions
// -----------------------------------------------

void GLViewer::animate() {
    if (time.elapsed() > msBetweenAnimation) {
        time.restart();
        focusBlinkOn = !focusBlinkOn;
    }
}

void GLViewer::drawCube(const Vec3Df min, const Vec3Df max) {
    glBegin(GL_LINES);
    glVertex3f(min[0], min[1], min[2]);
    glVertex3f(max[0], min[1], min[2]);
    glVertex3f(min[0], min[1], min[2]);
    glVertex3f(min[0], max[1], min[2]);
    glVertex3f(max[0], max[1], min[2]);
    glVertex3f(max[0], min[1], min[2]);
    glVertex3f(max[0], max[1], min[2]);
    glVertex3f(min[0], max[1], min[2]);

    glVertex3f(min[0], min[1], max[2]);
    glVertex3f(max[0], min[1], max[2]);
    glVertex3f(min[0], min[1], max[2]);
    glVertex3f(min[0], max[1], max[2]);
    glVertex3f(max[0], max[1], max[2]);
    glVertex3f(max[0], min[1], max[2]);
    glVertex3f(max[0], max[1], max[2]);
    glVertex3f(min[0], max[1], max[2]);

    glVertex3f(min[0], min[1], min[2]);
    glVertex3f(min[0], min[1], max[2]);
    glVertex3f(min[0], max[1], min[2]);
    glVertex3f(min[0], max[1], max[2]);
    glVertex3f(max[0], max[1], min[2]);
    glVertex3f(max[0], max[1], max[2]);
    glVertex3f(max[0], min[1], min[2]);
    glVertex3f(max[0], min[1], max[2]);
    glEnd();
}



void GLViewer::init() {
    glClearColor (0.f, 0.f, 0.f, 0.0);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LEQUAL);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_POINT_SMOOTH);

    // Set a FPS to 2
    setAnimationPeriod(msBetweenAnimation);

    glLoadIdentity();
    updateBoundingBox(controller->getScene());
    showEntireScene();
}


void GLViewer::draw_octree(const Octree * t) {
    glColor3f(0.f, 0.f, 0.f);
    drawCube(t->bBox.getMin(), t->bBox.getMax());
}

bool drawNode(const KDtree *t) {
    GLViewer::drawCube(t->bBox.getMin(), t->bBox.getMax());
    return true;
}

void GLViewer::draw() {
    const WindowModel *windowModel = controller->getWindowModel();
    const Scene * scene = controller->getScene();
    const RayTracer * rayTracer = controller->getRayTracer();

    // Display the rendered image
    const QImage &rayImage = windowModel->getRayImage();
    if (windowModel->getDisplayMode() == WindowModel::RayDisplayMode) {
        glDrawPixels (rayImage.width (),
                      rayImage.height (),
                      GL_RGB,
                      GL_UNSIGNED_BYTE,
                      rayImage.bits ());
        return;
    }

    // Draw the focus
    if (rayTracer->getTypeFocus() != Focus::NONE) {
        Vec3Df focusColor;
        bool isFocusMode = windowModel->isFocusMode();
        if (isFocusMode) {
            changeFocusPoint();
            if (focusBlinkOn) {
                focusColor = focusBlinkOnColor();
            } else {
                focusColor = focusBlinkOffColor();
            }
        } else {
            focusColor = focusFixedColor();
        }
        Vec3Df X, Y;
        currentFocusPoint.getNormal().getTwoOrthogonals(X,Y);
        glDisable (GL_LIGHTING);
        glColor3f(focusColor[0], focusColor[1], focusColor[2]);
        auto minidraw = [=](const Vec3Df & delta) {
            const Vec3Df pos = currentFocusPoint.getPos() + 0.01*currentFocusPoint.getNormal();
            Vec3Df tmp = pos + delta;
            glVertex3f(tmp[0], tmp[1], tmp[2]);
        };
        glBegin(GL_LINES);
        minidraw(-0.2*X);
        minidraw( 0.2*X);
        minidraw(-0.2*Y);
        minidraw( 0.2*Y);
        glEnd();
    }

    // Draw the light
    int lightIndex = windowModel->getSelectedLightIndex();
    if (lightIndex != -1) {
        const Light *light = scene->getLights()[lightIndex];
        Vec3Df lightPos = light->getPos();
        glDisable(GL_LIGHTING);
        Vec3Df delta = Vec3Df(0.1, 0.1, 0.1);
        Vec3Df min = lightPos - delta;
        Vec3Df max = lightPos + delta;
        Vec3Df color = light->getColor();
        glColor3f(color[0], color[1], color[2]);
        drawCube(min, max);
    }

    // Draw the scene
    vector<Object *> objectsToDraw;
    if (windowModel->isShowSurfel()) {
        objectsToDraw = controller->getPBGI()->getPointCloud()->getObjects();
    }
    else {
        objectsToDraw = scene->getObjects();
    }
    for (const Object *o : objectsToDraw) {
        if (!o->isEnabled()) {
            continue;
        }
        const Vec3Df & trans = o->getTrans ();
        glPushMatrix ();
        glTranslatef (trans[0], trans[1], trans[2]);
        const Material & mat = o->getMaterial ();
        const Vec3Df & color = mat.getColorTexture()->getRepresentativeColor();
        float dif = mat.getDiffuse ();
        float spec = mat.getSpecular ();
        static GLfloat glMatDiff[4];
        static GLfloat glMatSpec[4];
        static const GLfloat glMatAmb[4] = {0.f, 0.f, 0.f, 1.f};
        for (unsigned int j = 0; j < 3; j++) {
            glMatDiff[j] = dif*color[j];
            glMatSpec[j] = spec;
        }
        glMatDiff[3] = 1.0f;
        glMatSpec[3] = 1.0f;

        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, glMatDiff);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, glMatSpec);
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, glMatAmb);
        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 128);
        glDisable(GL_COLOR_MATERIAL);
        glEnable (GL_LIGHTING);
        o->getMesh().renderGL(windowModel->getRenderingMode() == WindowModel::FLAT);
        glPopMatrix ();
    }

    if (windowModel->isShowKDTree()) {
        glDisable(GL_LIGHTING);
        glColor3f(0, 0, 0);
        for (const Object *o : scene->getObjects()) {
            const Vec3Df & trans = o->getTrans ();
            glPushMatrix ();
            glTranslatef (trans[0], trans[1], trans[2]);
            o->getKDtree().exec(drawNode);
            glPopMatrix ();
        }
    }
}

MiniGLViewer::MiniGLViewer(Controller *c):
    controller(c),
    isWireframe(true)
{
    setAxisIsDrawn(true);
    setGridIsDrawn(true);
}

MiniGLViewer::~MiniGLViewer() {}

void MiniGLViewer::init() {
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    setBackgroundColor(QColor(0, 0, 0));
    glDisable(GL_LIGHTING);
    glColor3f(1, 0, 0);
}

void MiniGLViewer::update(const Observable *o) {
    auto windowModel = controller->getWindowModel();
    bool windowModelChanged = o == windowModel &&
        (o->isChanged(WindowModel::SELECTED_OBJECT_CHANGED));
    auto scene = controller->getScene();
    bool sceneChanged = o == scene &&
        (o->isChanged(Scene::OBJECT_CHANGED));
    int oi = windowModel->getSelectedObjectIndex();
    if (oi != -1 && (windowModelChanged || sceneChanged)) {
        const BoundingBox & sceneBBox = scene->getBoundingBox();
        Vec3Df c = sceneBBox.getCenter();
        float r = sceneBBox.getRadius();
        setSceneCenter(qglviewer::Vec(c[0], c[1], c[2]));
        setSceneRadius(r);
        // Move camera in order to see at least a polygon
        Vec3Df n = scene->getObjects()[oi]->getMesh().getVertices()[0].getNormal();
        camera()->setPosition(qglviewer::Vec(n[0], n[1], n[2]));
        camera()->lookAt(qglviewer::Vec(0, 0, 0));
        camera()->fitSphere(camera()->sceneCenter(), camera()->sceneRadius());
        updateGL();
    }
}

void MiniGLViewer::draw() {
    int index = controller->getWindowModel()->getSelectedObjectIndex();
    if (index != -1) {
        if (isWireframe) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        } else {
            glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
        }
        const Object *o = controller->getScene()->getObjects()[index];
        o->getMesh().renderGL(true);
    }
}

void MiniGLViewer::mouseDoubleClickEvent(QMouseEvent *) {
    isWireframe = !isWireframe;
}
