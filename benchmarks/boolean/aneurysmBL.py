from dgpy import *

GmshSetOption('General', 'Verbosity', 99.0) 
GmshSetOption('Mesh', 'SaveAll', 1.0) 
GmshSetOption('Mesh', 'Algorithm', 1.0)

g =  GModel()
g.setFactory('Gmsh')

g.load("aneurysm.stl");
g.createTopologyFromMesh();

face = g.getFaceByTag(1);
newent  = g.extrudeBoundaryLayer(face, 4, 0.5, 0, -1)
newent2 = g.extrudeBoundaryLayer(face, 4, -0.5, 1, -1)

print "*** face = %d " % face.tag()
print "*** new face = %d newface2 = %d " % (newent[0].tag(), newent2[0].tag())

g.mesh(2)
g.save("aneurysmBL.msh")

