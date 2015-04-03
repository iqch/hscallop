# Scallop point generator #

This is Scallop SOP - node, which generate pointsets laying in non-linear concurrent strange attractors pools. More information about them you may find at [Wikipedia article](http://en.wikipedia.org/wiki/Fractal_flame)

SOP Node produced next data :
  * Raw poinsets, which may be reused as geometric quantity
  * Baked point sets, spooled to file
  * Division map, stored as bgeo, for reusing it at slicing
  * Division atlas for custom executable, which can not use bgeo-based division maps (possible, it will reworked for JSON format using)

_Side projects for slicing and rendering of gigantic poinclouds are presented too, but since it is really hard problem - it is in active developement. I can't give you really good advise how to render such clouds - as volumetric or raw pointclouds etc. - you have to make decision about it each time according to your task._