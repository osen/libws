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

    self.getPosition().z = -5;
  }

  self.onDisplay = function()
  {
    self.shader.setProjection(self.getCamera().getProjection());

    self.getRotation().y += 0.1;

    var model = Mat4(1);
    model = model.translate(self.getPosition());
    model = model.rotate(self.getRotation().y, Vec3(0, 1, 0));
    model = model.rotate(self.getRotation().x, Vec3(1, 0, 0));
    // TODO: Scale
    self.shader.setModel(model);

    self.shader.render(self.model);
  };

  return self;
}
