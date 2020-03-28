function TriangleRenderer()
{
  var self = Component();

  self.mesh = null;
  self.shader = null;
  self.texture = null;

  self.onInitialize = function()
  {
    var r = self.getResources();

    self.mesh = r.loadDefaultModel();
    self.shader = r.loadDefaultShader();
    self.texture = r.loadDefaultTexture();
  }

  self.onDisplay = function()
  {

  };

  return self;
}
