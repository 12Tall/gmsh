// This reimplements gmsh/demos/boolean/boolean.geo in C++.

#include <gmsh.h>

int main(int argc, char **argv)
{
  gmshInitialize(argc, argv);
  gmshOptionSetNumber("General.Terminal", 1);

  gmshModelCreate("boolean");

  // from http://en.wikipedia.org/wiki/Constructive_solid_geometry

  gmshOptionSetNumber("Mesh.Algorithm", 6);
  gmshOptionSetNumber("Mesh.CharacteristicLengthMin", 0.4);
  gmshOptionSetNumber("Mesh.CharacteristicLengthMax", 0.4);

  double R = 1.4, Rs = R*.7, Rt = R*1.25;

  int o;
  std::vector<std::pair<int, int> > ov;
  std::vector<std::vector<std::pair<int, int> > > ovv;
  gmshModelOccAddBox(1, -R,-R,-R, 2*R,2*R,2*R, o);
  gmshModelOccAddSphere(2, 0,0,0,Rt, o);
  gmshModelOccBooleanIntersection(3, {{3, 1}}, {{3, 2}}, ov, ovv);
  gmshModelOccAddCylinder(4, -2*R,0,0, 4*R,0,0, Rs, o);
  gmshModelOccAddCylinder(5, 0,-2*R,0, 0,4*R,0, Rs, o);
  gmshModelOccAddCylinder(6, 0,0,-2*R, 0,0,4*R, Rs, o);
  gmshModelOccBooleanUnion(7, {{3, 4}, {3, 5}}, {{3, 6}}, ov, ovv);
  gmshModelOccBooleanDifference(8, {{3, 3}}, {{3, 7}}, ov, ovv);

  gmshModelOccSynchronize();

  gmshModelMesh(3);

  gmshExport("boolean.msh");

  gmshFinalize();
  return 0;
}
