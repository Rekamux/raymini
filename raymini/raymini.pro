TEMPLATE = app
TARGET   = raymini
CONFIG  += qt opengl xml warn_on console release thread
QMAKE_CXXFLAGS += -std=c++0x -g -fopenmp
QMAKE_LFLAGS += -fopenmp
QT *= opengl xml
HEADERS = Window.h \
          GLViewer.h \
          QTUtils.h \
          Vertex.h \
          Triangle.h \
          Mesh.h \
          BoundingBox.h \
          Material.h \
          Object.h \
          Light.h \
          Scene.h \
          RayTracer.h \
          Ray.h \
          KDtree.h \
          Noise.h \
          AntiAliasing.h \
          Color.h \
          Shadow.h \
          Texture.h \
          Observer.h \
          Observable.h \
          Controller.h \
          WindowModel.h \
          Focus.h \
          Surfel.h \
          PointCloud.h \
          RenderThread.h \
          ProgressBar.h \
          NamedClass.h \
          NoiseUser.h \
          Brdf.h \
          PBGI.h \
          Octree.h

SOURCES = Window.cpp \
          GLViewer.cpp \
          QTUtils.cpp \
          Vertex.cpp \
          Triangle.cpp \
          Mesh.cpp \
          BoundingBox.cpp \
          Material.cpp \
          Object.cpp \
          Light.cpp \
          Scene.cpp \
          RayTracer.cpp \
          Ray.cpp \
          KDtree.cpp \
          Brdf.cpp \
          Noise.cpp \
          AntiAliasing.cpp \
          Texture.cpp \
          Color.cpp \
          Shadow.cpp \
          Observable.cpp \
          Controller.cpp \
          WindowModel.cpp \
          Focus.cpp \
          Surfel.cpp \
          PointCloud.cpp \
          Octree.cpp \
          RenderThread.cpp \
          ProgressBar.cpp \
          PBGI.cpp \
          Main.cpp

    DESTDIR=.

win32 {
    INCLUDEPATH += '%HOMEPATH%\libQGLViewer-2.3.4'
    LIBS += -L"%HOMEPATH%\libQGLViewer-2.3.4\QGLViewer\release" \
            -lQGLViewer2 \
            -lglew32
}
unix {
    LIBS += -lGLEW \
            -lQGLViewer
}

MOC_DIR = .tmp
OBJECTS_DIR = .tmp
