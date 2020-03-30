function splitString(str, split)
{
  var rtn = [];
  var curr = "";

  for(var i = 0; i < str.length; i++)
  {
    if(str[i] == split)
    {
      rtn.push(curr);
      curr = "";
    }
    else
    {
      curr = curr + str[i];
    }
  }

  if(curr.length > 0)
  {
    rtn.push(curr);
  }

  return rtn;
}
