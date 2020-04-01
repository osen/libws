function Input(core)
{
  var self = {};
  self.core = core;
  self.pressedKeys = [];
  self.downKeys = [];
  self.upKeys = [];
  self.keyTable = [];

  self.pupulateTables = function()
  {
    var keys =
      " !\"#$%&'()*+,-./0123456789:;<=>?@" +
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ" +
      "[\\]^_`" +
      "abcdefghijklmnopqrstuvwxyz" +
      "{|}~";

    for(var i = 0; i < keys.length; i++)
    {
      self.keyTable.push(keys[i]);
    }

    self.keyTable.push("Enter");
    self.keyTable.push("Backspace");
    self.keyTable.push("Delete");
  };

  self.convertKey = function(key)
  {
    for(var kti = 0; kti < self.keyTable.length; kti++)
    {
      if(self.keyTable[kti] == key)
      {
        return kti;
      }
    }

    console.log("Unhandled Key: " + key);
    return -1;
  };

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
        var kc = self.convertKey(self.pressedKeys[ki]);
        if(kc == -1) continue;
        connection.send(connection.KEY_PRESSED + "\t" + kc);
      }

      for(var ki = 0; ki < self.downKeys.length; ki++)
      {
        var kc = self.convertKey(self.downKeys[ki]);
        if(kc == -1) continue;
        connection.send(connection.KEY_DOWN + "\t" + kc);
      }

      for(var ki = 0; ki < self.upKeys.length; ki++)
      {
        var kc = self.convertKey(self.upKeys[ki]);
        if(kc == -1) continue;
        connection.send(connection.KEY_UP + "\t" + kc);
      }
    }

    if(self.pressedKeys.length > 0) self.pressedKeys = [];
    if(self.downKeys.length > 0) self.downKeys = [];
    if(self.upKeys.length > 0) self.upKeys = [];
  };

  self.pupulateTables();

  window.addEventListener("keypress", self.onKeyPress);
  window.addEventListener("keydown", self.onKeyDown);
  window.addEventListener("keyup", self.onKeyUp);

  return self;
}
