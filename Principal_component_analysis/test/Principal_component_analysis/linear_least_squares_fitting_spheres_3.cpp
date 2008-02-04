// Example program for the linear_least_square_fitting function

#include <CGAL/Cartesian.h>
#include <CGAL/linear_least_squares_fitting_3.h>

#include <list>

typedef double               FT;
typedef CGAL::Cartesian<FT>  Kernel;
typedef Kernel::Line_3       Line;
typedef Kernel::Plane_3      Plane;
typedef Kernel::Point_3      Point;
typedef Kernel::Sphere_3     Sphere;

int main(void)
{
  std::cout << "Test linear least squares fitting of 3D spheres"  << std::endl;

	// centers
	Point c1(0.0,0.0,0.0);
	Point c2(1.0,1.0,1.0);

	// radii
	FT sqr1 = 0.1;
	FT sqr2 = 0.5;

	// add two spheres
  std::list<Sphere> spheres;
  spheres.push_back(Sphere(c1,sqr1));
  spheres.push_back(Sphere(c2,sqr2));

  Line line;
  Plane plane;
  Point centroid;

  linear_least_squares_fitting_3(spheres.begin(),spheres.end(),line,CGAL::PCA_dimension_3_tag());
  linear_least_squares_fitting_3(spheres.begin(),spheres.end(),line,CGAL::PCA_dimension_2_tag());
  linear_least_squares_fitting_3(spheres.begin(),spheres.end(),line,centroid,CGAL::PCA_dimension_3_tag());
  linear_least_squares_fitting_3(spheres.begin(),spheres.end(),line,centroid,CGAL::PCA_dimension_2_tag());

  linear_least_squares_fitting_3(spheres.begin(),spheres.end(),plane,CGAL::PCA_dimension_3_tag());
  linear_least_squares_fitting_3(spheres.begin(),spheres.end(),plane,CGAL::PCA_dimension_2_tag());
  linear_least_squares_fitting_3(spheres.begin(),spheres.end(),plane,centroid,CGAL::PCA_dimension_3_tag());
  linear_least_squares_fitting_3(spheres.begin(),spheres.end(),plane,centroid,CGAL::PCA_dimension_2_tag());

  return 0;
}
