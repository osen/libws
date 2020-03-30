function Component()
{
  var self = {};

  self.alive = true;
  self.id = 0;
  self.position = Vec3();
  self.rotation = Vec3();
  self.scale = Vec3(1, 1, 1);

  self.set = function(variable, value)
  {
    if(self.onSet) self.onSet(variable, value);

    if(variable == "position")
    {
      var pos = splitString(value, ",");
      self.position.x = parseFloat(pos[0]);
      self.position.y = parseFloat(pos[1]);
      self.position.z = parseFloat(pos[2]);
    }
  };

  self.getModel = function()
  {
    var rtn = Mat4(1);

    rtn = rtn.translate(self.position);
    rtn = rtn.rotate(self.rotation.y, Vec3(0, 1, 0));
    rtn = rtn.rotate(self.rotation.x, Vec3(1, 0, 0));
    // TODO: Scale

    return rtn;
  };

  self.getId = function()
  {
    return self.id;
  };

  self.isAlive = function()
  {
    return self.alive;
  };

  self.kill = function()
  {
    self.alive = false;
  };

  self.initialize = function()
  {
    if(self.onInitialize) self.onInitialize();
  };

  self.begin = function()
  {
    if(self.onBegin) self.onBegin();
  };

  self.tick = function()
  {
    if(self.onTick) self.onTick();
  };

  self.display = function()
  {
    if(self.onDisplay) self.onDisplay();
  };

  self.getPosition = function()
  {
    return self.position;
  };

  self.getRotation = function()
  {
    return self.rotation;
  }

  self.getScale = function()
  {
    return self.scale;
  }

  self.getCore = function()
  {
    return self.core;
  };

  self.getResources = function()
  {
    return self.getCore().getResources();
  };

  self.getWindow = function()
  {
    return self.getCore().getWindow();
  };

  self.getCamera = function()
  {
    return self.getCore().getCamera();
  };

  return self;
}
