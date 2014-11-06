/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "Renderer.h"

namespace ospray {
  namespace sg {
    using std::cout;
    using std::endl;

    Renderer::Renderer()
      // : ospFrameBuffer(NULL)
    {}

    int Renderer::renderFrame()
    { 
      if (!integrator) return 1;
      if (!frameBuffer) return 2;
      if (!camera) return 3;
      if (!world) return 4;

      assert(frameBuffer->ospFrameBuffer);
      assert(integrator->ospRenderer);

      if (!world->ospModel) {
        world->render();
        assert(world->ospModel);
      }

      integrator->setWorld(world);
      integrator->setCamera(camera);
      integrator->commit();

      ospRenderFrame(frameBuffer->ospFrameBuffer,
                     integrator->getOSPHandle(),
                     OSP_FB_COLOR|OSP_FB_ACCUM);
      return 0;
    }

    /*! re-start accumulation (for progressive rendering). make sure
      that this function gets called at lesat once every time that
      anything changes that might change the appearance of the
      converged image (e.g., camera position, scene, frame size,
      etc) */
    void Renderer::resetAccumulation()
    {
      accumID = 0;
      if (frameBuffer) 
        frameBuffer->clearAccum(); 
    }

    //! create a default camera
    Ref<sg::Camera> Renderer::createDefaultCamera()
    {
      // create a default camera
      Ref<sg::PerspectiveCamera> camera = new sg::PerspectiveCamera;

      // now, determine world bounds to automatically focus the camera
      box3f worldBounds = world->getBounds();
      if (worldBounds == box3f(empty)) {
        cout << "#osp:qtv: world bounding box is empty, using default camera pose" << endl;
      } else {
        cout << "#osp:qtv: found world bounds " << worldBounds << endl;
        cout << "#osp:qtv: focussing default camera on world bounds" << endl;

        camera->setAt(center(worldBounds));
        camera->setUp(vec3f(0,0,1));
        camera->setFrom(center(worldBounds) + .8f*vec3f(-1,-3,+1.5)*worldBounds.size());
        camera->commit();
      }

      return camera.cast<sg::Camera>();
    }

    void Renderer::setCamera(const Ref<sg::Camera> &camera) 
    {
      this->camera = camera;
      if (camera) 
        this->camera->commit();
      // if (this->camera) {
      //   this->camera->commit();
      // }
      if (integrator)
        integrator->setCamera(camera); 
// camera && integrator && integrator->ospRenderer) {
//         ospSetObject(integrator->ospRenderer,"camera",camera->ospCamera);
//         ospCommit(integrator->ospRenderer);
//       }
      resetAccumulation();
    }

    void Renderer::setIntegrator(const Ref<sg::Integrator> &integrator) 
    {      
      this->integrator = integrator;
      if (integrator) {
        integrator.ptr->commit();
      }
      resetAccumulation();
    }

    void Renderer::setWorld(const Ref<World> &world)
    {
      this->world = world;
      allNodes.clear();
      uniqueNodes.clear();
      if (world) {
        allNodes.serialize(world,sg::Serialization::DONT_FOLLOW_INSTANCES);
        uniqueNodes.serialize(world,sg::Serialization::DO_FOLLOW_INSTANCES);
      } else 
        std::cout << "#ospQTV: no world defined, yet\n#ospQTV: (did you forget to pass a scene file name on the command line?)" << std::endl;

      resetAccumulation();
    }

    //! find the last camera in the scene graph
    sg::Camera *Renderer::getLastDefinedCamera() const
    {
      for (size_t i=0;i<uniqueNodes.size();i++) {
        sg::Camera *asCamera
          = dynamic_cast<sg::Camera*>(uniqueNodes.object[i]->node.ptr);
        if (asCamera != NULL)
          return asCamera;
      }
      return NULL;
    }
    
    //! find the last integrator in the scene graph
    sg::Integrator *Renderer::getLastDefinedIntegrator() const
    {
      for (size_t i=0;i<uniqueNodes.size();i++) {
        sg::Integrator *asIntegrator
          = dynamic_cast<sg::Integrator*>(uniqueNodes.object[i]->node.ptr);
        if (asIntegrator != NULL)
          return asIntegrator;
      }
      return NULL;
    }
    
  }
}
