function TriangleRenderer()
{
  var self = Component();

  self.mesh = null;
  self.shader = null;

  self.onDisplay = function()
  {
    var r = self.getResources();

    if(!self.mesh) self.mesh = r.loadDefaultMesh();
    if(!self.shader) self.shader = r.loadDefaultShader();
    if(!self.shader || !self.mesh) return;
  };

  return self;
}
