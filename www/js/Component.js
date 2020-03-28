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

  return self;
}
