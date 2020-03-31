function Core()
{
  var self = {};

  self.components = [];
  self.window = Window(self);
  self.resources = Resources(self);
  self.connection = Connection(self);
  self.input = Input(self);

  self.start = function()
  {
    window.requestAnimationFrame(self.frame);
  };

  self.getComponentById = function(id)
  {
    for(var i = 0; i < self.components.length; i++)
    {
      if(self.components[i].getId() == id)
      {
        return self.components[i];
      }
    }

    throw "Component with the specified ID does not exist";
  };

  self.addComponent = function(id, ctor)
  {
    var c = ctor();

    self.components.push(c);
    c.id = id;
    c.core = self;
    c.initialize();

    return c;
  };

  self.addComponentByName = function(id, name)
  {
    var ctor = window[name];

    if(!ctor)
    {
      throw "Invalid component type";
    }

    if(typeof(ctor) != "function")
    {
      throw "Specified type is not a component";
    }

    self.addComponent(id, ctor);
  };

  self.tick = function()
  {
    self.window.tick();
    self.connection.tick();
    self.input.tick();

    for(var i = 0; i < self.components.length; i++)
    {
      if(self.components[i].isAlive() == false)
      {
        self.components.splice(i, 1);
        i--;
      }
    }

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

  self.getConnection = function()
  {
    return self.connection;
  };

  self.getInput = function()
  {
    return self.input;
  };

  self.camera = self.addComponent(-1, Camera);

  return self;
}
