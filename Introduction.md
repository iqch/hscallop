# Scallop point generator #

This is Scallop SOP - node, which generate pointsets of non-linear concurrent strange attrctors pool. More information about thme placed at [Wikipedia article](http://en.wikipedia.org/wiki/Fractal_flame)


# Details #

SOP Node produced next data :
  * Raw poinsets, which may be reused as geometric quantity
  * Baked point sets, spooled to file
  * Division map, stored as bgeo, for reusing it at slicing
  * Division atlas for custom executable, which can not use bgeo-based division maps (possible, it will reworked for JSON format using)

---
> Side projects for slicing and rendering of gigantic poinclouds are presented too, but since it is really hard problem - it is in active developement. You may decide as you wish to render such clouds - as volumetric or raw pointclouds - I can't give you really good advise - you have to decide each time according to your task.