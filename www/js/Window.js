function Window(core)
{
  var self = {};
  self.core = core;

  // The documentElement is the "html" css node.
  document.documentElement.style.width = "100%";
  document.documentElement.style.height = "100%";

  document.body.style.margin = "0em";
  //document.body.style.minWidth = "100%";
  //document.body.style.minHeight = "100%";
  document.body.style.width = "100%";
  document.body.style.height = "100%";
  document.body.style.display = "flex";

  self.canvas = document.createElement("canvas");
  document.body.appendChild(self.canvas);
  self.canvas.style.background = "orange";
  self.canvas.style.flexGrow = 1;

  self.gl = self.canvas.getContext("webgl");

  if(!self.gl)
  {
    throw "Failed to get WebGL context";
  }

  self.getGl = function()
  {
    return self.gl;
  };

  self.getWidth = function()
  {
    return self.canvas.clientWidth;
  }

  self.getHeight = function()
  {
    return self.canvas.clientHeight;
  }

  self.tick = function()
  {
    if(self.canvas.width != self.getWidth() ||
      self.canvas.height != self.getHeight())
    {
      self.canvas.width = self.getWidth();
      self.canvas.height = self.getHeight();
    }
  };

  self.onResize = function()
  {
    // Hack: Allow the container to shrink so we can request correct size.
    self.canvas.width = 150;
    self.canvas.height = 150;
  };

  window.addEventListener("resize", self.onResize);

  return self;
}
