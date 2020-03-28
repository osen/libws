function Model()
{
  var self = Resource();

  self.onLoadDefault = function()
  {
    alert("Loading default model");
  };

  return self;
}
