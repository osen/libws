function Window()
{
  var self = {};

  // The documentElement is the "html" css node.
  document.documentElement.style.width = "100%";
  document.documentElement.style.height = "100%";

  document.body.style.margin = "0em";
  document.body.style.minWidth = "100%";
  document.body.style.minHeight = "100%";
  document.body.style.display = "flex";

  self.canvas = document.createElement("canvas");
  document.body.appendChild(self.canvas);
  self.canvas.style.background = "orange";
  self.canvas.style.flexGrow = 1;

  return self;
}
