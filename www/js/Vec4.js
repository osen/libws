function Vec4(a, b, c, d)
{
  var self = {};

  if(d != null)
  {
    self.x = a;
    self.y = b;
    self.z = c;
    self.w = d;
  }
  else if(c != null)
  {
    self.x = a.x;
    self.y = a.y;
    self.z = b;
    self.w = c;
  }
  else if(b != null)
  {
    self.x = a.x;
    self.y = a.y;
    self.z = a.z;
    self.w = b;
  }
  else if(a != null)
  {
    self.x = a.x;
    self.y = a.y;
    self.z = a.z;
    self.w = a.w;
  }
  else
  {
    self.x = 0;
    self.y = 0;
    self.z = 0;
    self.w = 0;
  }

  if(self.x == null || self.y == null || self.z == null || self.w == null)
  {
    throw "Invalid vector";
  }

  self.add = function(a, b, c, d)
  {
    var rtn = Vec4(a, b, c, d);

    rtn.x += self.x;
    rtn.y += self.y;
    rtn.z += self.z;
    rtn.w += self.w;

    return rtn;
  }

  return self;
}
