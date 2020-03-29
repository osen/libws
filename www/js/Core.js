function Core()
{
  var self = {};

  self.components = [];
  self.window = Window(self);
  self.resources = Resources(self);
  self.camera = Camera(self);
  self.connection = Connection(self);

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
    self.connection.tick();

    for(var i = 0; i < self.components.length; i++)
    {
      self.components[i].tick();
    }
  };

  self.display = function()
  {
    var gl = self.getGl();

    gl.viewport(0, 0, gl.drawingBufferWidth, gl.drawingBufferHeight);
    gl.clearColor(0.5, 0.5, 0.5, 1.0);
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

    gl.enable(gl.DEPTH_TEST);
    //gl.enable(gl.CULL_FACE);

    for(var i = 0; i < self.components.length; i++)
    {
      self.components[i].display();
    }

    gl.disable(gl.DEPTH_TEST);

    //for(var i = 0; i < self.components.length; i++)
    //{
    //  self.components[i].ui();
    //}

    gl.disable(gl.CULL_FACE);
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

  self.getCamera = function()
  {
    return self.camera;
  };

  return self;
}
