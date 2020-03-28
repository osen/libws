function Resource()
{
  var self = {};
  self.core = null;
  self.path = null;

  self.load = function()
  {
    if(!self.onLoad)
    {
      throw "Resource does not have a load";
    }

    self.onLoad();
  };

  self.loadDefault = function()
  {
    if(!self.onLoadDefault)
    {
      throw "Resource does not have a default";
    }

    self.onLoadDefault();
  };

  return self;
}
