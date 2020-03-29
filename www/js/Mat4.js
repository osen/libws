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

    var a00;
    var a01;
    var a02;
    var a03;

    var a10;
    var a11;
    var a12;
    var a13;

    var a20;
    var a21;
    var a22;
    var a23;

    a00 = self.m[0];
    a01 = self.m[1];
    a02 = self.m[2];
    a03 = self.m[3];
    a10 = self.m[4];
    a11 = self.m[5];
    a12 = self.m[6];
    a13 = self.m[7];
    a20 = self.m[8];
    a21 = self.m[9];
    a22 = self.m[10];
    a23 = self.m[11];

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
  }

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
