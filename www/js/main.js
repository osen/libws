function main()
{
  try
  {
    var core = Core();

    //core.addComponent(TriangleRenderer);

    core.start();
  }
  catch(err)
  {
    alert("Unhandled Exception: " + err);
  }
}

window.addEventListener("load", main);
