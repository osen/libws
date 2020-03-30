function main()
{
  try
  {
    var core = Core();

    //core.addComponent(7, TriangleRenderer);

    core.start();
  }
  catch(err)
  {
    alert("Unhandled Exception: " + err);
  }
}

window.addEventListener("load", main);
