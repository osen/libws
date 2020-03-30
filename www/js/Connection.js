function Connection(core)
{
  var self = {};
  self.core = core;
  self.connected = false;
  self.messages = [];

  self.ADD_COMPONENT = 1;
  self.REMOVE_COMPONENT = 2;
  self.SET = 3;

  self.onMessage = function(event)
  {
    self.messages.push(splitString(event.data, "\t"));
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
    var url = "" + window.location;

    for(var ci = 0; ci < url.length; ci++)
    {
      if(url[ci] == ':')
      {
        url = url.substring(ci);
        break;
      }
    }

    url = "ws" + url;
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

  self.processMessage = function(message)
  {
    if(message[0] == self.SET)
    {
      self.core.getComponentById(parseInt(message[1])).set(message[2], message[3]);
    }
    else if(message[0] == self.ADD_COMPONENT)
    {
      self.core.addComponentByName(parseInt(message[1]), message[2]);
    }
    else if(message[0] == self.REMOVE_COMPONENT)
    {
      self.core.getComponentById(message[1]).kill();
    }
  };

  self.tick = function()
  {
    if(!self.connected) return;

    for(var mi = 0; mi < self.messages.length; mi++)
    {
      self.processMessage(self.messages[mi]);
    }

    self.messages = [];
  };

  self.connect();

  return self;
}
