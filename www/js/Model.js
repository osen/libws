function Model()
{
  var self = Resource();
  self.meshes = [];

  self.onLoadDefault = function()
  {
    var mesh = Mesh(self.getCore().getGl());

    var positions = [];
    positions.push(Vec2(0, 0.5));
    positions.push(Vec2(-0.5, -0.5));
    positions.push(Vec2(0.5, -0.5));

    var coords = [];
    coords.push(Vec2(0.5, 1));
    coords.push(Vec2(0, 0));
    coords.push(Vec2(1, 0));

    mesh.setData(positions, coords);
    self.meshes.push(mesh);
  };

  return self;
}
