function Component()
{
  var self = {};

  self.position = Vec3();
  self.rotation = Vec3();
  self.scale = Vec3();

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
