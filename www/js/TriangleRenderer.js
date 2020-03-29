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
    self.shader.setProjection(self.getCamera().getProjection());

    var model = Mat4(1);
    model = model.translate(Vec3(0, 0, -5));
    self.shader.setModel(model);

    self.shader.render(self.model);
  };

  return self;
}
