OPC opc;
PImage dot;

void setup()
{
  //size(800, 400);

  dot = loadImage("dot.png");

  size(320, 64);

  // Connect to an LEDscape opc-rx process. Only one client can be connected at a time.
  opc = new OPC(this, "beaglebone.local", 7890);

  // Map an 16x16 grid of LEDs to the center of the window, scaled to take up most of the space
  float spacing = height / 16.0;
  opc.ledGrid16x16(0,    width * 1/10, height/2, spacing, 0, true);
  opc.ledGrid16x16(256,  width * 3/10, height/2, spacing, 0, true);
  opc.ledGrid16x16(512,  width * 5/10, height/2, spacing, 0, true);
  opc.ledGrid16x16(768,  width * 7/10, height/2, spacing, 0, true);
  opc.ledGrid16x16(1024, width * 9/10, height/2, spacing, 0, true);
}

void draw()
{
  background(0);
  
  // Change the dot size as a function of time, to make it "throb"
  float dotSize = height * 0.6 * (1.0 + 0.2 * sin(millis() * 0.01));
  
  // Draw it centered at the mouse location
  image(dot, mouseX - dotSize/2, mouseY - dotSize/2, dotSize, dotSize);
}

