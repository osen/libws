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

    //self.getPosition().z = -5;
  }

  self.onDisplay = function()
  {
    self.shader.setProjection(self.getCamera().getProjection());
    self.shader.setView(self.getCamera().getView());

    self.getRotation().y += 180 * self.getDeltaTime();
    self.shader.setModel(self.getModel());

    self.shader.setTexture(self.texture);

    self.shader.render(self.model);
  };

  return self;
}
