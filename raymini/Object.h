// *********************************************************
// Object Class
// Author : Tamy Boubekeur (boubek@gmail.com).
// Copyright (C) 2010 Tamy Boubekeur.
// All rights reserved.
// *********************************************************

#ifndef OBJECT_H
#define OBJECT_H

#include <iostream>
#include <vector>

#include "Mesh.h"
#include "Material.h"
#include "BoundingBox.h"
#include "KDtree.h"

class Object {
public:
    inline Object () : tree(nullptr){}
    inline Object (const Mesh & mesh, const Material & mat) : mesh (mesh), mat (mat), tree(nullptr) {
        updateBoundingBox ();
    }
    inline Object (const Object & o) : mesh (o.mesh), mat (o.mat), bbox (o.bbox), trans (o.trans), tree(nullptr) {}
    virtual ~Object () {
        delete tree;
    }

    inline Object & operator= (const Object & o) {
        mesh = o.mesh;
        mat = o.mat;
        bbox = o.bbox;
        trans = o.trans;
        tree = nullptr;
        return (*this);
    }

    inline const Vec3Df & getTrans () const { return trans;}
    inline void setTrans (const Vec3Df & t) { trans = t; }

    inline const Mesh & getMesh () const { return mesh; }
    inline Mesh & getMesh () { return mesh; }
    
    inline const Material & getMaterial () const { return mat; }
    inline Material & getMaterial () { return mat; }

    inline const KDtree & getKDtree () {
        if(!tree)
            tree = new KDtree(*this);
        return *tree;
    }

    inline const BoundingBox & getBoundingBox () const { return bbox; }
    void updateBoundingBox ();
    
private:
    Mesh mesh;
    Material mat;
    BoundingBox bbox;
    Vec3Df trans;
    KDtree *tree;
};


#endif // Scene_H

// Some Emacs-Hints -- please don't remove:
//
//  Local Variables:
//  mode:C++
//  tab-width:4
//  End:
