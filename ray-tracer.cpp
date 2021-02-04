#include <qapplication.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "Viewer.h"
#include "Scene.h"
#include "Sphere.h"
#include "Material.h"
#include "PointLight.h"

using namespace std;
using namespace rt;

void addBubble( Scene& scene, Point3 c, Real r, Material transp_m )
{
  Material revert_m = transp_m;
  std::swap( revert_m.in_refractive_index, revert_m.out_refractive_index );
  Sphere* sphere_out = new Sphere( c, r, transp_m );
  Sphere* sphere_in  = new Sphere( c, r-0.02f, revert_m );
  scene.addObject( sphere_out );
  scene.addObject( sphere_in );
}
int main(int argc, char** argv)
{
  // Read command lines arguments.
  QApplication application(argc,argv);
  
  // Creates a 3D scene
  Scene scene;
  
  // Light at infinity
Light* light0 = new PointLight( GL_LIGHT0, Point4( 0,0,1,0 ),
                                  Color( 1.0, 1.0, 1.0 ) );
Light* light1 = new PointLight( GL_LIGHT1, Point4( -10,-4,2,1 ),
                                Color( 1.0, 1.0, 1.0 ) );
scene.addLight( light0 );
scene.addLight( light1 );
// Objects
Sphere* sphere1 = new Sphere( Point3( 0, 0, 0), 2.0, Material::bronze() );
Sphere* sphere2 = new Sphere( Point3( 0, 4, 0), 1.0, Material::emerald() );
Sphere* sphere3 = new Sphere( Point3( 6, 6, 0), 3.0, Material::whitePlastic() );
scene.addObject( sphere1 );
scene.addObject( sphere2 );
scene.addObject( sphere3 );
addBubble( scene, Point3( -5, 4, -1 ), 2.0, Material::glass() );
addBubble( scene, Point3( -10, 6, -2 ), 2.0, Material::glass() );
addBubble( scene, Point3( -10, 8, -5 ), 2.0, Material::glass() );
addBubble( scene, Point3( -14, 2, -3 ), 2.0, Material::glass() );
addBubble( scene, Point3( -20, 1, -10 ), 2.0, Material::glass() );
  
  // Instantiate the viewer.
  Viewer viewer;
  // Give a name
  viewer.setWindowTitle("Ray-tracer preview");

  // Sets the scene
  viewer.setScene( scene );

  // Make the viewer window visible on screen.
  viewer.show();
  // Run main loop.
  application.exec();
  return 0;
}



