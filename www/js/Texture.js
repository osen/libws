function Texture()
{
  var self = Resource();
  self.id = null;

  self.loadDefault = function()
  {
    var gl = self.getCore().getGl();

    var pixels = [
      255, 0, 0, 255,
      0, 255, 0, 255,
      0, 0, 255, 255,
      0, 0, 0, 255
    ];

    self.id = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, self.id);

    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA,
      2, 2, 0, gl.RGBA, gl.UNSIGNED_BYTE, new Uint8Array(pixels));

    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);

    gl.bindTexture(gl.TEXTURE_2D, null);
  };

  return self;
}
