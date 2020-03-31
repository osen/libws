function Input(core)
{
  var self = {};
  self.core = core;
  self.pressedKeys = [];
  self.downKeys = [];
  self.upKeys = [];

  self.onKeyPress = function(event)
  {
    self.pressedKeys.push(event.key);
  };

  self.onKeyDown = function(event)
  {
    self.downKeys.push(event.key);
  };

  self.onKeyUp = function(event)
  {
    self.upKeys.push(event.key);
  };

  self.tick = function()
  {
    var connection = self.core.getConnection();
    var connected = connection.isConnected();

    if(connected)
    {
      for(var ki = 0; ki < self.pressedKeys.length; ki++)
      {
        connection.send(connection.KEY_PRESSED + "\t" + self.pressedKeys[ki]);
      }

      for(var ki = 0; ki < self.downKeys.length; ki++)
      {
        connection.send(connection.KEY_DOWN + "\t" + self.downKeys[ki]);
      }

      for(var ki = 0; ki < self.upKeys.length; ki++)
      {
        connection.send(connection.KEY_UP + "\t" + self.upKeys[ki]);
      }
    }

    if(self.pressedKeys.length > 0) self.pressedKeys = [];
    if(self.downKeys.length > 0) self.downKeys = [];
    if(self.upKeys.length > 0) self.upKeys = [];
  };

  window.addEventListener("keypress", self.onKeyPress);
  window.addEventListener("keydown", self.onKeyDown);
  window.addEventListener("keyup", self.onKeyUp);

  return self;
}
