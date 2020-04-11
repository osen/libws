function Connection(core)
{
  var self = {};
  self.core = core;
  self.connected = false;
  self.messages = [];

  self.ADD_COMPONENT = 1;
  self.REMOVE_COMPONENT = 2;
  self.SET = 3;

  self.CONTINUE = 1;
  self.KEY_PRESSED = 2;
  self.KEY_DOWN = 3;
  self.KEY_UP = 4;

  self.onMessage = function(event)
  {
    self.messages.push(event.data);
  };

  self.onClose = function()
  {
    self.connected = false;
  };

  self.onOpen = function()
  {
    self.connected = true;
  };

  self.onError = function()
  {
    self.connected = false;
  };

  self.connect = function()
  {
    var prot = "";
    var url = "" + window.location;

    for(var ci = 0; ci < url.length; ci++)
    {
      if(url[ci] == ':')
      {
        prot = url.substring(0, ci);
        url = url.substring(ci);
        break;
      }
    }

    if(prot == "https")
    {
      url = "wss" + url;
    }
    else if(prot == "http")
    {
      url = "ws" + url;
    }
    else
    {
      throw "Failed to determine websocket protocol";
    }

    self.socket = new WebSocket(url);
  
    self.socket.onclose = self.onClose;
    self.socket.onopen = self.onOpen;
    self.socket.onerror = self.onError;
    self.socket.onmessage = self.onMessage;
  };

  self.isConnected = function()
  {
    return self.connected;
  };

  self.processStream = function(message)
  {
    var m0 = message.getString();

    if(m0 == self.SET)
    {
      self.core.getComponentById(message.getInt()).set(
        message.getString(), message.getString());
    }
    else if(m0 == self.ADD_COMPONENT)
    {
      self.core.addComponentByName(message.getInt(), message.getString());
    }
    else if(m0 == self.REMOVE_COMPONENT)
    {
      self.core.getComponentById(message.getInt()).kill();
    }
  };

  self.send = function(message)
  {
    if(!self.connected)
    {
      throw "Connection is not established";
    };

    self.socket.send(message);
    //console.log(message);
  };

  self.tick = function()
  {
    if(!self.connected) return;
    if(self.messages.length < 1) return;

    for(var mi = 0; mi < self.messages.length; mi++)
    {
      console.log("[" + self.messages[mi] + "]");
      var cs = CommandStream(self.messages[mi]);

      while(!cs.isEmpty())
      {
        self.processStream(cs);
      }
    }

    self.messages = [];
    self.socket.send("" + self.CONTINUE);
  };

  self.connect();

  return self;
}
