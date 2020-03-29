function Vec2(a, b)
{
  var self = {};

  if(b != null)
  {
    self.x = a;
    self.y = b;
  }
  else if(a != null)
  {
    self.x = a.x;
    self.y = a.y;
  }
  else
  {
    self.x = 0;
    self.y = 0;
  }

  if(self.x == null || self.y == null)
  {
    throw "Invalid vector";
  }

  self.add = function(a, b)
  {
    var rtn = Vec2(a, b);

    rtn.x += self.x;
    rtn.y += self.y;

    return rtn;
  }

  return self;
}

