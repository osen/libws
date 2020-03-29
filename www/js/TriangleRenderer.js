function TriangleRenderer()
{
  var self = Component();

  self.model = null;
  self.shader = null;
  self.texture = null;

  self.onInitialize = function()
  {
    var r = self.getResources();

    self.model = r.loadDefaultModel();
    self.shader = r.loadDefaultShader();
    self.texture = r.loadDefaultTexture();
  }

  self.onDisplay = function()
  {

  };

  return self;
}
