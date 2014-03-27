#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/wlop_simplify_and_regularize_point_set.h>
#include <CGAL/IO/read_xyz_points.h>
#include <CGAL/IO/write_xyz_points.h>
#include <CGAL/Timer.h>#include <CGAL/tags.h>
#include <CGAL/Simple_cartesian.h>

#include <vector>
#include <fstream>
#include <time.h>
// types
typedef CGAL::Simple_cartesian<double> Kernel;
typedef Kernel::Point_3 Point;

int main(void)
{
  const std::string INPUT_FILENAME_WITHOUT_EXT = "data/saint_jean_370K";

  // Reads a .xyz point set file in points[], *with normals*.
	// TOFIX: do we have an extension for points with normals?
  std::vector<Point> points;
  std::ifstream stream(INPUT_FILENAME_WITHOUT_EXT + ".xyz");

  if (!stream || !CGAL::read_xyz_points(stream, std::back_inserter(points)))
  {
    std::cerr << "Error: cannot read file "
              << INPUT_FILENAME_WITHOUT_EXT << ".xyz" << std::endl;
    return EXIT_FAILURE;
  }

  CGAL::Timer task_timer;
  task_timer.start();
  std::cout << "Run algorithm example: " << std::endl;  std::vector<Point> output;

  //with default parameters begins
  //CGAL::wlop_simplify_and_regularize_point_set<CGAL::Parallel_tag>(
  //                                             points.begin(), 
  //                                             points.end(),
  //                                             back_inserter(output));
  //with default parameters ends

  //with all parameters begins
  
  //Algorithm parameters
  const double retain_percentage = 2;       // percentage of points to retain.
  const double neighbor_radius = 0.03;      // neighbors size.
  const unsigned int iter_number = 30;      // number of iterations.
  const bool need_compute_density = true;  // if needed to compute density.
  CGAL::wlop_simplify_and_regularize_point_set<CGAL::Parallel_tag>(
                                               points.begin(),
                                               points.end(),
                                               back_inserter(output),
                                               retain_percentage, 
                                               neighbor_radius,
                                               iter_number,
                                               need_compute_density);
  
  //with all parameters ends
  
  std::ofstream out(INPUT_FILENAME_WITHOUT_EXT + "_WLOPED.xyz"); 
  long memory = CGAL::Memory_sizer().virtual_size();
  std::cout << "total done: " << task_timer.time() << " seconds, " 
            << (memory>>20) << " Mb allocated" << std::endl;    if (!out ||
    !CGAL::write_xyz_points(
    out, output.begin(), output.end()))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}




