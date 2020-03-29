function Model()
{
  var self = Resource();
  self.meshes = [];

  self.onLoadDefault = function()
  {
    var mesh = Mesh(self.getCore().getGl());

    var positions = [];
    positions.push(Vec3(-0.5, 0.5, 0));
    positions.push(Vec3(-0.5, -0.5, 0));
    positions.push(Vec3(0.5, -0.5, 0));

    var coords = [];
    coords.push(Vec3(0, 1, 0));
    coords.push(Vec3(0, 0, 0));
    coords.push(Vec3(1, 0, 0));

    mesh.setData(positions, coords);
    self.meshes.push(mesh);
  };

  return self;
}
