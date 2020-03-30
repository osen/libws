function CommandStream(message)
{
  var self = {};

  self.message = message;
  self.curr = 0;
  self.finish = message.length;

  self.isEmpty = function()
  {
    if(self.curr >= self.finish)
    {
      return true;
    }

    return false;
  }

  self.getString = function()
  {
    if(self.isEmpty())
    {
      throw "No more data in the stream";
    }

    var start = self.curr;
    var end = self.message.indexOf("\t", start);

    if(end == -1)
    {
      end = self.finish;
      self.curr = end;
    }
    else
    {
      self.curr = end + 1;
    }

    return self.message.slice(start, end);
  };

  self.getInt = function()
  {
    return parseInt(self.getString());
  };

  self.getFloat = function()
  {
    return parseFloat(self.getString());
  };

  return self;
}

