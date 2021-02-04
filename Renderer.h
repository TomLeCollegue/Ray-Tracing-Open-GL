/**
@file Renderer.h
@author JOL
*/
#pragma once
#ifndef _RENDERER_H_
#define _RENDERER_H_

#include "Color.h"
#include "Image2D.h"
#include "Ray.h"
#include <math.h> 

/// Namespace RayTracer
namespace rt {

  inline void progressBar( std::ostream& output,
                           const double currentValue, const double maximumValue)
  {
    static const int PROGRESSBARWIDTH = 60;
    static int myProgressBarRotation = 0;
    static int myProgressBarCurrent = 0;
    // how wide you want the progress meter to be
    double fraction = currentValue /maximumValue;
    
    // part of the progressmeter that's already "full"
    int dotz = static_cast<int>(floor(fraction * PROGRESSBARWIDTH));
    if (dotz > PROGRESSBARWIDTH) dotz = PROGRESSBARWIDTH;
    
    // if the fullness hasn't changed skip display
    if (dotz == myProgressBarCurrent) return;
    myProgressBarCurrent = dotz;
    myProgressBarRotation++;
    
    // create the "meter"
    int ii=0;
    output << "[";
    // part  that's full already
    for ( ; ii < dotz;ii++) output<< "#";
    // remaining part (spaces)
    for ( ; ii < PROGRESSBARWIDTH;ii++) output<< " ";
    static const char* rotation_string = "|\\-/";
    myProgressBarRotation %= 4;
    output << "] " << rotation_string[myProgressBarRotation]
           << " " << (int)(fraction*100)<<"/100\r";
    output.flush();
  
  }

  
  struct Background {
    virtual Color backgroundColor( const Ray& ray ) = 0;
  };
  
  struct MyBackground : public Background {
     Color backgroundColor( const Ray& ray )
     {
      Color result;
      float z  = ray.direction.at(2);
      if(z >= 0.0f && z <=0.5f){
          return Color(1 - 2 * z, 1 - 2 * z,1);
      }
      if( z > 0.5f && z <= 1.0f){
          return Color(0,0,1.0f - (z - 0.5f) * 2);
      }
      else {

          Real x = -0.5f * ray.direction[ 0 ] / ray.direction[ 2 ];
          Real y = -0.5f * ray.direction[ 1 ] / ray.direction[ 2 ];
          Real d = sqrt( x*x + y*y );
          Real t = std::min( d, 30.0f ) / 30.0f;
          x -= floor( x );
          y -= floor( y );
          if ( ( ( x >= 0.5f ) && ( y >= 0.5f ) ) || ( ( x < 0.5f ) && ( y < 0.5f ) ) )
              result += (1.0f - t)*Color( 0.7f, 0.7f, 0.7f ) + t * Color( 1.0f, 1.0f, 1.0f );
          else
              result += (1.0f - t)*Color( 0.9f, 0.9f, 0.9f ) + t * Color( 1.0f, 1.0f, 1.0f );
          return result;
     }
    }
};

  /// This structure takes care of rendering a scene.
  struct Renderer {

    /// The scene to render
    Scene* ptrScene;
    /// The origin of the camera in space.
    Point3 myOrigin;
    /// (myOrigin, myOrigin+myDirUL) forms a ray going through the upper-left
    /// corner pixel of the viewport, i.e. pixel (0,0)
    Vector3 myDirUL;
    /// (myOrigin, myOrigin+myDirUR) forms a ray going through the upper-right
    /// corner pixel of the viewport, i.e. pixel (width,0)
    Vector3 myDirUR;
    /// (myOrigin, myOrigin+myDirLL) forms a ray going through the lower-left
    /// corner pixel of the viewport, i.e. pixel (0,height)
    Vector3 myDirLL;
    /// (myOrigin, myOrigin+myDirLR) forms a ray going through the lower-right
    /// corner pixel of the viewport, i.e. pixel (width,height)
    Vector3 myDirLR;

    Background* ptrBackground;

    int myWidth;
    int myHeight;

    Renderer() : ptrScene( 0 ) {}
    Renderer( Scene& scene ) : ptrScene( &scene ) {
      ptrBackground = new MyBackground();
    }
    void setScene( rt::Scene& aScene ) { ptrScene = &aScene; }
    
    void setViewBox( Point3 origin, 
                     Vector3 dirUL, Vector3 dirUR, Vector3 dirLL, Vector3 dirLR )
    {
      myOrigin = origin;
      myDirUL = dirUL;
      myDirUR = dirUR;
      myDirLL = dirLL;
      myDirLR = dirLR;
    }

    void setResolution( int width , int height )
    {
      myWidth  = width * 5;
      myHeight = height * 5;
    }


    /// The main rendering routine
    void render( Image2D<Color>& image, int max_depth )
    {
      std::cout << "Rendering into image ... might take a while." << std::endl;
      image = Image2D<Color>( myWidth, myHeight );
      for ( int y = 0; y < myHeight; ++y ) 
        {
          Real    ty   = (Real) y / (Real)(myHeight-1);
          progressBar( std::cout, ty, 1.0 );
          Vector3 dirL = (1.0f - ty) * myDirUL + ty * myDirLL;
          Vector3 dirR = (1.0f - ty) * myDirUR + ty * myDirLR;
          dirL        /= dirL.norm();
          dirR        /= dirR.norm();
          for ( int x = 0; x < myWidth; ++x ) 
            {
              Real    tx   = (Real) x / (Real)(myWidth-1);
              Vector3 dir  = (1.0f - tx) * dirL + tx * dirR;
              Ray eye_ray  = Ray( myOrigin, dir, max_depth );
              Color result = trace( eye_ray );
              image.at( x, y ) = result.clamp();
            }
        }
      std::cout << "Done." << std::endl;
    }

    Color background( const Ray& ray )
    {
      Color result = Color( 0.0, 0.0, 0.0 );
      for ( Light* light : ptrScene->myLights )
        {
          Real cos_a = light->direction( ray.origin ).dot( ray.direction );
          if ( cos_a > 0.99f )
            {
              Real a = acos( cos_a ) * 360.0 / M_PI / 8.0;
              a = std::max( 1.0f - a, 0.0f );
              result += light->color( ray.origin ) * a * a;
            }
        }
      if ( ptrBackground != 0 ) result += ptrBackground->backgroundColor( ray );
      return result;
    }



    /// The rendering routine for one ray.
    /// @return the color for the given ray.
    Color trace( const Ray& ray )
    {
      assert( ptrScene != 0 );
      Color result = Color( 0.0, 0.0, 0.0 );
      GraphicalObject* obj_i = 0; // pointer to intersected object
      Point3           p_i;       // point of intersection

      // Look for intersection in this direction.
      Real ri = ptrScene->rayIntersection( ray, obj_i, p_i );
      // Nothing was intersected
      if ( ri >= 0.0f ){
        return this->background(ray); //some background color
      }

      Material m = obj_i->getMaterial(p_i);

      if(ray.depth > 0 && m.coef_reflexion != 0){
          int profondeur = ray.depth - 1;
          Vector3 directionReflect = reflect(ray.direction, obj_i->getNormal(p_i));
          Vector3 pt = p_i + directionReflect * 0.01f; //On ne veut pas un point pile dessus pour éviter dessus.
          Ray rayRefl = Ray(pt, directionReflect, profondeur);
          Color C_refl = trace(rayRefl);
          result += C_refl * m.specular * m.coef_reflexion;
      }

      if(ray.depth > 0 && m.coef_refraction != 0){
          Ray rayRefr = refractionRay(ray, p_i, obj_i->getNormal(p_i),m );
          Color c_refract = trace(rayRefr);
          result += c_refract * m.diffuse * m.coef_refraction;
      }

      Color finalColor = illumination(ray, obj_i, p_i);
      finalColor = finalColor * obj_i->getMaterial(p_i).coef_diffusion;

      result += finalColor;

      return result;
    }

    Vector3 reflect( const Vector3& V, Vector3 N ) const{
      Vector3 W = V - 2 * V.dot(N) * N;
      return W;
    }

    Color illumination( const Ray& ray, GraphicalObject* obj, Point3 p ){
      Material m = obj->getMaterial(p);
      Color C = Color(0.0, 0.0, 0.0);
      

      for(std::vector<Light*>::const_iterator it = this->ptrScene->myLights.begin() , itE=this->ptrScene->myLights.end();it!=itE;it++){
        
        Vector3 lightDirection = (*it)->direction(ray.direction);
        Vector3 normalP = obj->getNormal(p);
        Color lightColor = (*it)->color(p); //B

        //calcul des ombres
        Ray ObjLight = Ray(p, lightDirection,1);
        Color colorShadow = shadow(ObjLight, lightColor);


        //Diffuse
        Real coeffDiffuse = lightDirection.dot(normalP) / (lightDirection.norm() * normalP.norm()); //kd
        if(coeffDiffuse < 0){
          coeffDiffuse = 0;
        }
        C += (m.diffuse * lightColor * coeffDiffuse * colorShadow); // C <-- C +kdD * B    //(+ les ombres)

        //Specular
        Vector3 W = reflect(ray.direction, normalP);
        Real cosBeta = W.dot(lightDirection) / (lightDirection.norm() * W.norm());
        if(cosBeta < 0){
           cosBeta = 0;
        } 
        Real coeffSpecular = std::pow(cosBeta, m.shinyness);
        C+= (lightColor * m.specular * coeffSpecular);

        //C += shadow(ray,C);
        
      }
      C += m.ambient; //On ajoute à C la couleur ambiente 

      return C;
    }

    /// Calcule la couleur de la lumière (donnée par light_color) dans la
    /// direction donnée par le rayon. Si aucun objet n'est traversé,
    /// retourne light_color, sinon si un des objets traversés est opaque,
    /// retourne du noir, et enfin si les objets traversés sont
    /// transparents, attenue la couleur.
    Color shadow( const Ray& ray, Color light_color ){
      GraphicalObject *obj; 
      Point3 p;
      Point3 p2 = ray.origin;


      while (light_color.max() > 0.003f){
        Point3 pDecale = p2 + ray.direction * 0.0001f;
        Ray ray2 = Ray(pDecale, ray.direction, ray.depth);
        Real r = ptrScene->rayIntersection(ray2, obj, p);
        if(r>= 0.0f){
          return light_color;
        }
        Material m = obj->getMaterial(p);
        light_color = light_color * m.diffuse * m.coef_refraction;
        p2 = p;

      }
      return light_color;
    }

    Ray refractionRay( const Ray& aRay, const Point3& p, Vector3 N, const Material& m ){
      Real r;
      Real c = - N.dot(aRay.direction);

      if(aRay.direction.dot(N) <= 0 ) {
          r = m.out_refractive_index / m.in_refractive_index ;
      }
      else{

          r = m.in_refractive_index / m.out_refractive_index ;
      }

      Real tmp;
      if(c>0)
          tmp = r*c - sqrt(1 - ( (r*r) * (1 - (c*c) )));
      else {
          tmp = r * c + sqrt(1 - ((r * r) * (1 - (c * c))));
      }

      Vector3 Vrefract = r*aRay.direction + tmp * N;
      return Ray(p + aRay.direction * 0.0001f, Vrefract, aRay.depth - 1);
    }

  };

} // namespace rt

#endif // #define _RENDERER_H_
