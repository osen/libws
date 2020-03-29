function Resources(core)
{
  var self = {};
  self.core = core;
  self.resources = [];
  self.defaultTexture = null;
  self.defaultShader = null;
  self.defaultModel = null;

  self.load = function(ctor, path)
  {
    for(var ri = 0; ri < self.resources.length; ri++)
    {
      if(path == self.resources[ri].path)
      {
        return self.resources[ri];
      }
    }

    var rtn = ctor();

    rtn.core = self.core;
    rtn.path = path;
    rtn.load();
    self.resources.push(rtn);

    return rtn;
  };

  self.loadDefaultModel = function()
  {
    var rtn = self.defaultModel;
    if(rtn) return rtn;

    rtn = Model();
    rtn.core = self.core;
    rtn.loadDefault();
    self.defaultModel = rtn;

    return rtn;
  };

  self.loadDefaultShader = function()
  {
    var rtn = self.defaultShader;
    if(rtn) return rtn;

    rtn = Shader();
    rtn.core = self.core;
    rtn.loadDefault();
    self.defaultShader = rtn;

    return rtn;
  };

  self.loadDefaultTexture = function()
  {
    var rtn = self.defaultTexture;
    if(rtn) return rtn;

/*
    rtn = Texture();
    rtn.core = self.core;
    rtn.loadDefault();
    self.defaultTexture = rtn;
*/

    return rtn;
  };

  return self;
}
