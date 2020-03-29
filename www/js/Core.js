function Core()
{
  var self = {};

  self.components = [];
  self.window = Window(self);
  self.resources = Resources(self);

  self.start = function()
  {
    window.requestAnimationFrame(self.frame);
  };

  self.addComponent = function(ctor)
  {
    var c = ctor();

    self.components.push(c);
    c.core = self;
    c.initialize();

    return c;
  };

  self.tick = function()
  {
    self.window.tick();

    for(var i = 0; i < self.components.length; i++)
    {
      self.components[i].tick();
    }
  };

  self.display = function()
  {
    var gl = self.getGl();

    gl.viewport(0, 0, self.window.getWidth(), self.window.getHeight());
    gl.clearColor(0.5, 0.5, 0.5, 1.0);
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

    for(var i = 0; i < self.components.length; i++)
    {
      self.components[i].display();
    }
  };

  self.frame = function()
  {
    try
    {
      self.tick();
      self.display();
      window.requestAnimationFrame(self.frame);
    }
    catch(err)
    {
      alert("Unhandled Exception: " + err);
    }
  };

  self.getWindow = function()
  {
    return self.window;
  };

  self.getResources = function()
  {
    return self.resources;
  };

  self.getGl = function()
  {
    return self.getWindow().getGl();
  };

  return self;
}
