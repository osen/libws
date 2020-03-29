function Shader()
{
  var self = Resource();

  self.id = null;
  self.positionAttrib = -1;
  self.coordAttrib = -1;
  self.normalAttrib = -1;

  self.render = function(model)
  {
    var gl = self.getCore().getGl();

    if(!self.id || self.positionAttrib == -1)
    {
      return;
    }

    gl.useProgram(self.id);

    for(var mi = 0; mi < model.meshes.length; mi++)
    {
      var mesh = model.meshes[mi];

      gl.bindBuffer(gl.ARRAY_BUFFER, mesh.positionVbo);
      gl.vertexAttribPointer(self.positionAttrib, mesh.components, gl.FLOAT, false, 0, 0);
      gl.enableVertexAttribArray(self.positionAttrib);
      gl.drawArrays(gl.TRIANGLES, 0, mesh.vertCount);
    }

    gl.useProgram(null);
  };

  self.prepareLocations = function()
  {
    var gl = self.getCore().getGl();

    self.positionAttrib = gl.getAttribLocation(self.id, 'a_Position')
    self.coordAttrib = gl.getAttribLocation(self.id, 'a_TexCoord')
    self.normalAttrib = gl.getAttribLocation(self.id, 'a_Normal')
  };

  self.onLoadDefault = function()
  {
    var gl = self.getCore().getGl();
    var vertId = gl.createShader(gl.VERTEX_SHADER);

    gl.shaderSource(vertId,
      "attribute vec3 a_Position;           \n" +
      "                                     \n" +
      "void main()                          \n" +
      "{                                    \n" +
      "  gl_Position = vec4(a_Position, 1); \n" +
      "}                                    \n"
    );

    gl.compileShader(vertId);

    if(!gl.getShaderParameter(vertId, gl.COMPILE_STATUS))
    {
      throw "Failed to compile vertex shader [" + gl.getShaderInfoLog(vertId) + "]";
    }

    var fragId = gl.createShader(gl.FRAGMENT_SHADER);

    gl.shaderSource(fragId,
      "void main()                          \n" +
      "{                                    \n" +
      "  gl_FragColor = vec4(1, 0, 0, 1);   \n" +
      "}                                    \n"
    );

    gl.compileShader(fragId);

    if(!gl.getShaderParameter(fragId, gl.COMPILE_STATUS))
    {
      throw "Failed to compile fragment shader [" + gl.getShaderInfoLog(fragId) + "]";
    }

    self.id = gl.createProgram();
    gl.attachShader(self.id, vertId);
    gl.attachShader(self.id, fragId);
    gl.linkProgram(self.id);

    if(!gl.getProgramParameter(self.id, gl.LINK_STATUS))
    {
      throw "Failed to link shader program [" + gl.getProgramInfoLog(self.id) + "]";
    }

    self.prepareLocations();
  };

  return self;
}
