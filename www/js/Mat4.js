//  0  1  2  3
//  4  5  6  7
//  8  9 10 11
// 12 13 14 15

function Mat4(a)
{
  var self = {};

  //self.m = new Array(16);
  self.m = new Float32Array(16);

  if(a)
  {
    self.m[0] = a;
    self.m[5] = a;
    self.m[10] = a;
    self.m[15] = a;
  }

  self.translate = function(a, b, c)
  {
    var v = Vec3(a, b, c);
    var rtn = Mat4();

    var a00 = self.m[0];
    var a01 = self.m[1];
    var a02 = self.m[2];
    var a03 = self.m[3];
    var a10 = self.m[4];
    var a11 = self.m[5];
    var a12 = self.m[6];
    var a13 = self.m[7];
    var a20 = self.m[8];
    var a21 = self.m[9];
    var a22 = self.m[10];
    var a23 = self.m[11];

    rtn.m[0] = a00;
    rtn.m[1] = a01;
    rtn.m[2] = a02;
    rtn.m[3] = a03;
    rtn.m[4] = a10;
    rtn.m[5] = a11;
    rtn.m[6] = a12;
    rtn.m[7] = a13;
    rtn.m[8] = a20;
    rtn.m[9] = a21;
    rtn.m[10] = a22;
    rtn.m[11] = a23;

    rtn.m[12] = a00 * v.x + a10 * v.y + a20 * v.z + self.m[12];
    rtn.m[13] = a01 * v.x + a11 * v.y + a21 * v.z + self.m[13];
    rtn.m[14] = a02 * v.x + a12 * v.y + a22 * v.z + self.m[14];
    rtn.m[15] = a03 * v.x + a13 * v.y + a23 * v.z + self.m[15];

    return rtn;
  };

  self.rotate = function(angle, a, b, c)
  {
    var rtn = Mat4();
    var ax = Vec3(a, b, c);

    var len = Math.hypot(ax.x, ax.y, ax.z);

    //if (len < glMatrix.EPSILON) {
    //  return null;
    //}

    len = 1 / len;
    ax.x *= len;
    ax.y *= len;
    ax.z *= len;

    var s = Math.sin(angle);
    var c = Math.cos(angle);
    var t = 1 - c;

    var a00 = self.m[0];
    var a01 = self.m[1];
    var a02 = self.m[2];
    var a03 = self.m[3];
    var a10 = self.m[4];
    var a11 = self.m[5];
    var a12 = self.m[6];
    var a13 = self.m[7];
    var a20 = self.m[8];
    var a21 = self.m[9];
    var a22 = self.m[10];
    var a23 = self.m[11];

    // Construct the elements of the rotation matrix
    var b00 = ax.x * ax.x * t + c;
    var b01 = ax.y * ax.x * t + ax.z * s;
    var b02 = ax.z * ax.x * t - ax.y * s;
    var b10 = ax.x * ax.y * t - ax.z * s;
    var b11 = ax.y * ax.y * t + c;
    var b12 = ax.z * ax.y * t + ax.x * s;
    var b20 = ax.x * ax.z * t + ax.y * s;
    var b21 = ax.y * ax.z * t - ax.x * s;
    var b22 = ax.z * ax.z * t + c;

    // Perform rotation-specific matrix multiplication
    rtn.m[0] = a00 * b00 + a10 * b01 + a20 * b02;
    rtn.m[1] = a01 * b00 + a11 * b01 + a21 * b02;
    rtn.m[2] = a02 * b00 + a12 * b01 + a22 * b02;
    rtn.m[3] = a03 * b00 + a13 * b01 + a23 * b02;
    rtn.m[4] = a00 * b10 + a10 * b11 + a20 * b12;
    rtn.m[5] = a01 * b10 + a11 * b11 + a21 * b12;
    rtn.m[6] = a02 * b10 + a12 * b11 + a22 * b12;
    rtn.m[7] = a03 * b10 + a13 * b11 + a23 * b12;
    rtn.m[8] = a00 * b20 + a10 * b21 + a20 * b22;
    rtn.m[9] = a01 * b20 + a11 * b21 + a21 * b22;
    rtn.m[10] = a02 * b20 + a12 * b21 + a22 * b22;
    rtn.m[11] = a03 * b20 + a13 * b21 + a23 * b22;

    rtn.m[12] = self.m[12];
    rtn.m[13] = self.m[13];
    rtn.m[14] = self.m[14];
    rtn.m[15] = self.m[15];

    return rtn;
  };

  return self;
}

function Mat4Perspective(fovy, aspect, near, far)
{
  var self = Mat4();

  var f = 1.0 / Math.tan(fovy / 2);
  var nf = 1 / (near - far);

  self.m[0] = f / aspect;
  self.m[1] = 0;
  self.m[2] = 0;
  self.m[3] = 0;
  self.m[4] = 0;
  self.m[5] = f;
  self.m[6] = 0;
  self.m[7] = 0;
  self.m[8] = 0;
  self.m[9] = 0;
  self.m[10] = (far + near) * nf;
  self.m[11] = -1;
  self.m[12] = 0;
  self.m[13] = 0;
  self.m[14] = 2 * far * near * nf;
  self.m[15] = 0;

  return self;
}
