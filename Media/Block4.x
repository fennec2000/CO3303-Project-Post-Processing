xof 0303txt 0032

Mesh single_mesh_object {
 24;
 -5.000000;-5.000000;-1.000000;,
 -5.000000; 5.000000;-1.000000;,
  5.000000;-5.000000;-1.000000;,
  5.000000; 5.000000;-1.000000;,
  
  5.000000;-5.000000;-1.000000;,
  5.000000; 5.000000;-1.000000;,
  5.000000;-5.000000; 1.000000;,
  5.000000; 5.000000; 1.000000;,
  
  5.000000;-5.000000; 1.000000;,
  5.000000; 5.000000; 1.000000;,
 -5.000000;-5.000000; 1.000000;,
 -5.000000; 5.000000; 1.000000;,
 
 -5.000000;-5.000000; 1.000000;,
 -5.000000; 5.000000; 1.000000;,
 -5.000000;-5.000000;-1.000000;,
 -5.000000; 5.000000;-1.000000;,
 
 -5.000000; 5.000000;-1.000000;,
 -5.000000; 5.000000; 1.000000;,
  5.000000; 5.000000;-1.000000;,
  5.000000; 5.000000; 1.000000;,
  
 -5.000000;-5.000000; 1.000000;,
 -5.000000;-5.000000;-1.000000;,
  5.000000;-5.000000; 1.000000;;
  5.000000;-5.000000;-1.000000;,

 12;
 3;0,1,2;,
 3;2,1,3;,
 3;4,5,6;,
 3;6,5,7;,
 3;8,9,10;,
 3;10,9,11;,
 3;12,13,14;,
 3;14,13,15;,
 3;16,17,18;,
 3;18,17,19;,
 3;20,21,22;,
 3;22,21,23;;

 MeshNormals {
  24;
   0.000000; 0.000000;-1.000000;,
   0.000000; 0.000000;-1.000000;,
   0.000000; 0.000000;-1.000000;,
   0.000000; 0.000000;-1.000000;,
   1.000000; 0.000000; 0.000000;,
   1.000000; 0.000000; 0.000000;,
   1.000000; 0.000000; 0.000000;,
   1.000000; 0.000000; 0.000000;,
   0.000000; 0.000000; 1.000000;,
   0.000000; 0.000000; 1.000000;,
   0.000000; 0.000000; 1.000000;,
   0.000000; 0.000000; 1.000000;,
  -1.000000; 0.000000; 0.000000;,
  -1.000000; 0.000000; 0.000000;,
  -1.000000; 0.000000; 0.000000;,
  -1.000000; 0.000000; 0.000000;,
   0.000000; 1.000000; 0.000000;,
   0.000000; 1.000000; 0.000000;,
   0.000000; 1.000000; 0.000000;,
   0.000000; 1.000000; 0.000000;,
   0.000000;-1.000000; 0.000000;,
   0.000000;-1.000000; 0.000000;,
   0.000000;-1.000000; 0.000000;,
   0.000000;-1.000000; 0.000000;;
  12;
  3;0,1,2;,
  3;2,1,3;,
  3;4,5,6;,
  3;6,5,7;,
  3;8,9,10;,
  3;10,9,11;,
  3;12,13,14;,
  3;14,13,15;,
  3;16,17,18;,
  3;18,17,19;,
  3;20,21,22;,
  3;22,21,23;;
 }

 MeshTextureCoords {
  24;
  0.000000;1.000000;,
  0.000000;0.000000;,
  1.000000;1.000000;,
  1.000000;0.000000;,
  
  0.000000;1.000000;,
  0.000000;0.000000;,
  0.200000;1.000000;,
  0.200000;0.000000;,
  
  0.000000;1.000000;,
  0.000000;0.000000;,
  1.000000;1.000000;,
  1.000000;0.000000;,
  
  0.000000;1.000000;,
  0.000000;0.000000;,
  0.200000;1.000000;,
  0.200000;0.000000;,
  
  0.000000;0.200000;,
  0.000000;0.000000;,
  1.000000;0.200000;,
  1.000000;0.000000;,
  
  0.000000;0.200000;,
  0.000000;0.000000;,
  1.000000;0.200000;,
  1.000000;0.000000;,
 }

 MeshMaterialList {
  1;
  12;
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0;

  Material GreyNoise {
   1.000000;0.500000;0.500000;1.000000;;
   32.000000;
   2.000000;2.000000;2.000000;;
   0.000000;0.000000;0.000000;;

   TextureFilename {
    "Crystal.dds";
   }
  }
 }
}
