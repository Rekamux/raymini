// *********************************************************
// Ray Tracer Class
// Author : Tamy Boubekeur (boubek@gmail.com).
// Copyright (C) 2012 Tamy Boubekeur.
// All rights reserved.
// *********************************************************

#include <QProgressDialog>

#include "RayTracer.h"
#include "Ray.h"
#include "Scene.h"
#include "Brdf.h"
#include "Noise.h"
#include "Model.h"
#include "AntiAliasing.h"
#include "Color.h"

using namespace std;

static RayTracer * instance = NULL;

RayTracer * RayTracer::getInstance () {
    if (instance == NULL)
        instance = new RayTracer ();
    return instance;
}

void RayTracer::destroyInstance () {
    if (instance != NULL) {
        delete instance;
        instance = NULL;
    }
}

inline int clamp (float f, int inf, int sup) {
    int v = static_cast<int> (f);
    return (v < inf ? inf : (v > sup ? sup : v));
}

// POINT D'ENTREE DU PROJET.
// Le code suivant ray trace uniquement la boite englobante de la scene.
// Il faut remplacer ce code par une veritable raytracer
QImage RayTracer::render (const Vec3Df & camPos,
                          const Vec3Df & direction,
                          const Vec3Df & upVector,
                          const Vec3Df & rightVector,
                          float fieldOfView,
                          float aspectRatio,
                          unsigned int screenWidth,
                          unsigned int screenHeight) {
    QImage image (QSize (screenWidth, screenHeight), QImage::Format_RGB888);
    Scene * scene = Scene::getInstance ();

    QProgressDialog progressDialog ("Raytracing...", "Cancel", 0, 100);
    progressDialog.show ();

    vector<pair<float, float>> offsets = AntiAliasing::generateOffsets();

    // For each pixel
    for (unsigned int i = 0; i < screenWidth; i++) {
        progressDialog.setValue ((100*i)/screenWidth);
        for (unsigned int j = 0; j < screenHeight; j++) {

            Color c (backgroundColor);

            // For each ray in each pixel
            for (const pair<float, float> &offset : offsets) {
                float tanX = tan (fieldOfView)*aspectRatio;
                float tanY = tan (fieldOfView);
                Vec3Df stepX = (float(i)+offset.first - screenWidth/2.f)/screenWidth * tanX * rightVector;
                Vec3Df stepY = (float(j)+offset.second - screenHeight/2.f)/screenHeight * tanY * upVector;
                Vec3Df step = stepX + stepY;
                Vec3Df dir = direction + step;
                dir.normalize ();

                bool hasIntersection = false;
                Object *intersectedObject;
                Vertex closestIntersection;
                float smallestIntersectionDistance = 1000000.f;

                // For each object
                for (Object & o : scene->getObjects()) {
                    Ray ray (camPos-o.getTrans (), dir);

                    if (o.getKDtree().intersect(ray)) {
                        float intersectionDistance = ray.getIntersectionDistance();
                        const Vertex &intersection = ray.getIntersection();

                        if(intersectionDistance < smallestIntersectionDistance) {
                            smallestIntersectionDistance = intersectionDistance;

                            hasIntersection = true;
                            intersectedObject = &o;
                            closestIntersection = intersection;
                        }
                    }
                }
                if(hasIntersection)
                    c += getColor(intersectedObject, closestIntersection, camPos);
            }

            image.setPixel (i, j, qRgb (clamp (c[0], 0, 255), clamp (c[1], 0, 255), clamp (c[2], 0, 255)));
        }
    }
    progressDialog.setValue (100);
    return image;
}

Color RayTracer::getColor(const Object *intersectedObject,
                          const Vertex & closestIntersection,
                          const Vec3Df & camPos) const {
    Scene * scene = Scene::getInstance ();
    static const Perlin perlin(0.5f, 4, 10);
    float noise = perlin(closestIntersection.getPos());

    Brdf brdf(scene->getLights(),
              noise*intersectedObject->getMaterial().getColor(),
              Vec3Df(1.0,1.0,1.0),
              Vec3Df(0.5,0.5,0.0),
              intersectedObject->getMaterial().getDiffuse(),
              intersectedObject->getMaterial().getSpecular(),
              0.1,
              1.5);

    Vec3Df color = brdf.getColor(closestIntersection.getPos(), closestIntersection.getNormal(), camPos) * 255.0;
    float visibilite = 1.f;

    // TODO: do it for every light sources in the scene
    if(rayMode == Shadow) {
        unsigned int nb_impact = 0;
        vector<Vec3Df> pulse_light = scene->getLights()[0].generateImpulsion();

        for(const Vec3Df & impulse_l : pulse_light) {
            for(Object & o : scene->getObjects()) {
                Vec3Df dir = impulse_l - closestIntersection.getPos() - intersectedObject->getTrans() + o.getTrans();
                dir.normalize();
                Ray ray_light(closestIntersection.getPos() + intersectedObject->getTrans() - o.getTrans() + 0.000001*dir, dir);
                if(o.getKDtree().intersect(ray_light) &&
                   ray_light.getIntersectionDistance() > 0.000001) {
                    nb_impact++;
                }
            }
        }
        visibilite = (float)(Light::NB_IMPULSE - nb_impact) / (float)Light::NB_IMPULSE;
    }

    if(rayMode == Mirror) {
        const Vec3Df & impulse_l = scene->getLights()[0].getPos();
        for(Object & o : scene->getObjects()) {

            Vec3Df dir = impulse_l - closestIntersection.getPos() - intersectedObject->getTrans() + o.getTrans();
            dir.normalize();
            Ray ray_light(closestIntersection.getPos() + intersectedObject->getTrans() - o.getTrans() + 0.000001*dir, dir);
            if(o.getKDtree().intersect(ray_light) &&
               ray_light.getIntersectionDistance() > 0.000001 &&
               &o !=intersectedObject) {
                const Vertex &intersection = ray_light.getIntersection();
                brdf.lights = {scene->getLights()[0]};
                brdf.colorDif = o.getMaterial().getColor();
                brdf.Kd = o.getMaterial().getDiffuse();
                brdf.Ks = o.getMaterial().getSpecular();

                color = brdf.getColor(intersection.getPos(), intersection.getNormal(), camPos) * 255.0;
            }
        }
    }

    return visibilite*color;
}
