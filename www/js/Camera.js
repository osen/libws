function Camera(core)
{
  var self = {};
  self.core = core;

  self.getProjection = function()
  {
    return Mat4Perspective(45,
      self.core.getWindow().getWidth() / self.core.getWindow().getHeight(),
      0.1, 100);
  };

  return self;
}
