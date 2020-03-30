function Shader()
{
  var self = Resource();

  self.id = null;
  self.positionAttrib = -1;
  self.coordAttrib = -1;
  self.normalAttrib = -1;
  self.projectionUniform = -1;
  self.viewUniform = -1;
  self.modelUniform = -1;

  self.setProjection = function(projection)
  {
    if(self.projectionUniform == -1) return;
    var gl = self.getCore().getGl();
    gl.useProgram(self.id);
    gl.uniformMatrix4fv(self.projectionUniform, false, projection.m);
    gl.useProgram(null);
  };

  self.setView = function(view)
  {
    if(self.viewUniform == -1) return;
    var gl = self.getCore().getGl();
    gl.useProgram(self.id);
    gl.uniformMatrix4fv(self.viewUniform, false, view.m);
    gl.useProgram(null);
  };

  self.setModel = function(model)
  {
    if(self.modelUniform == -1) return;
    var gl = self.getCore().getGl();
    gl.useProgram(self.id);
    gl.uniformMatrix4fv(self.modelUniform, false, model.m);
    gl.useProgram(null);
  };

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

      if(self.coordAttrib != -1 && mesh.coordVbo != null)
      {
        gl.bindBuffer(gl.ARRAY_BUFFER, mesh.coordVbo);
        gl.vertexAttribPointer(self.coordAttrib, 2, gl.FLOAT, false, 0, 0);
        gl.enableVertexAttribArray(self.coordAttrib);
      }

      if(self.normalAttrib != -1 && mesh.normalVbo != null)
      {
        gl.bindBuffer(gl.ARRAY_BUFFER, mesh.normalVbo);
        gl.vertexAttribPointer(self.normalAttrib, mesh.components, gl.FLOAT, false, 0, 0);
        gl.enableVertexAttribArray(self.normalAttrib);
      }

      gl.drawArrays(gl.TRIANGLES, 0, mesh.vertCount);

      if(self.coordAttrib != -1 && mesh.coordVbo != null)
      {
        gl.disableVertexAttribArray(self.coordAttrib);
      }

      if(self.normalAttrib != -1 && mesh.normalVbo != null)
      {
        gl.disableVertexAttribArray(self.normalAttrib);
      }

      gl.disableVertexAttribArray(self.positionAttrib);
    }

    gl.bindBuffer(gl.ARRAY_BUFFER, null);
    gl.useProgram(null);
  };

  self.prepareLocations = function()
  {
    var gl = self.getCore().getGl();

    self.positionAttrib = gl.getAttribLocation(self.id, 'a_Position')
    self.coordAttrib = gl.getAttribLocation(self.id, 'a_TexCoord')
    self.normalAttrib = gl.getAttribLocation(self.id, 'a_Normal')

    self.projectionUniform = gl.getUniformLocation(self.id, 'u_Projection');
    self.viewUniform = gl.getUniformLocation(self.id, 'u_View');
    self.modelUniform = gl.getUniformLocation(self.id, 'u_Model');
  };

  self.onLoadDefault = function()
  {
    var gl = self.getCore().getGl();
    var vertId = gl.createShader(gl.VERTEX_SHADER);

    gl.shaderSource(vertId,
      "attribute vec3 a_Position;                                    \n" +
      "                                                              \n" +
      "uniform mat4 u_Projection;                                    \n" +
      "uniform mat4 u_View;                                          \n" +
      "uniform mat4 u_Model;                                         \n" +
      "                                                              \n" +
      "void main()                                                   \n" +
      "{                                                             \n" +
      "  gl_Position = u_Projection * u_View * u_Model *             \n" +
      "    vec4(a_Position, 1);                                      \n" +
      "}                                                             \n"
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
