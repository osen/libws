function Mesh(gl)
{
  var self = {};
  self.gl = gl;
  self.components = null;
  self.vertCount = 0;
  self.positionVbo = null;
  self.coordVbo = null;
  self.normalVbo = null;
  self.texture = null;

  self.setData = function(positions, coords, normals)
  {
    var gl = self.gl;
    self.positionVbo = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, self.positionVbo);
    var buffer = [];

    self.vertCount = positions.length;
    self.coordVbo = null;
    self.normalVbo = null;
    self.components = 2;

    for(var pi = 0; pi < positions.length; pi++)
    {
      buffer.push(positions[pi].x);
      buffer.push(positions[pi].y);

      if(positions[pi].z != null)
      {
        self.components = 3;
        buffer.push(positions[pi].z);
      }
    }

    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(buffer), gl.STATIC_DRAW);

    if(coords)
    {
      if(coords.length != positions.length)
      {
        throw "Vertex buffer sizes do not match";
      }

      self.coordVbo = gl.createBuffer();
      gl.bindBuffer(gl.ARRAY_BUFFER, self.coordVbo);
      buffer = [];

      for(var pi = 0; pi < coords.length; pi++)
      {
        buffer.push(coords[pi].x);
        buffer.push(coords[pi].y);
      }

      gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(buffer), gl.STATIC_DRAW);
    }

    if(normals)
    {
      if(normals.length != positions.length)
      {
        throw "Vertex buffer sizes do not match";
      }

      self.normalVbo = gl.createBuffer();
      gl.bindBuffer(gl.ARRAY_BUFFER, self.normalVbo);
      buffer = [];

      for(var pi = 0; pi < normals.length; pi++)
      {
        buffer.push(normals[pi].x);
        buffer.push(normals[pi].y);

        if(self.components > 2)
        {
          buffer.push(normals[pi].z);
        }
      }

      gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(buffer), gl.STATIC_DRAW);
    }

    gl.bindBuffer(gl.ARRAY_BUFFER, null);
  };

  return self;
}
