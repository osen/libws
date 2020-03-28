function Vec3(a, b, c)
{
  var self = {};

  if(c != null)
  {
    self.x = a;
    self.y = b;
    self.z = c;
  }
  else if(b != null)
  {
    self.x = a.x;
    self.y = a.y;
    self.z = b;
  }
  else if(a != null)
  {
    self.x = a.x;
    self.y = a.y;
    self.z = a.z;
  }
  else
  {
    self.x = 0;
    self.y = 0;
    self.z = 0;
  }

  if(self.x == null || self.y == null || self.z == null)
  {
    throw "Invalid vector";
  }

  self.add = function(a, b, c)
  {
    var rtn = Vec3(a, b, c);

    rtn.x += self.x;
    rtn.y += self.y;
    rtn.z += self.z;

    return rtn;
  }

  return self;
}

