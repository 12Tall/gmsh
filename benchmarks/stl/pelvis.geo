Mesh.CharacteristicLengthFactor=0.133;
Mesh.Algorithm3D = 4; //Frontal (4) Delaunay(1)

Merge "pelvisSMOOTH.stl";
CreateTopology;

Compound Surface(200)={1} ; //Conformal;

Surface Loop(300)={200};
Volume(301)={300};

Physical Surface (501)={200};
Physical Volume(502)={301};


