function Core()
{
  var self = {};

  self.components = [];
  self.window = Window();

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
    for(var i = 0; i < self.components.length; i++)
    {
      self.components[i].tick();
    }
  };

  self.display = function()
  {
    var gl = self.getGl();

    gl.clearColor(1.0, 0.0, 0.0, 1.0);
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

  self.getGl = function()
  {
    return self.getWindow().getGl();
  };

  return self;
}
