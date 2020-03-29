function Connection(core)
{
  var self = {};
  self.core = core;
  self.connected = false;
  self.messages = [];

  self.onMessage = function(event)
  {
    alert("Message: " + event.data);
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

  self.tick = function()
  {
    if(!self.connected) return;

  };

  self.connect();

  return self;
}
