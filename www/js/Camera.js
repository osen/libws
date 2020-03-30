function Camera()
{
  var self = Component();

  self.onInitialize = function()
  {
    self.getPosition().y = 5;
    self.getRotation().x = -45;
  };

  self.getProjection = function()
  {
    return Mat4Perspective(45,
      self.core.getWindow().getWidth() / self.core.getWindow().getHeight(),
      0.1, 100);
  };

  self.getView = function()
  {
    return self.getModel().inverse();
  };

  return self;
}
