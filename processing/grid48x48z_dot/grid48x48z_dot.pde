OPC opc;
PImage dot;

void setup()
{
  //size(800, 400);

  dot = loadImage("dot.png");

  size(640, 640);

   // Connect to an LEDscape opc-rx process. Only one client can be connected at a time.
  opc = new OPC(this, "beaglebone.local", 7890);
  // Map an 16x16 grid of LEDs to the center of the window, scaled to take up most of the space
  float spacing = height / 64.0;
  opc.ledGrid16x16(0,    width * 2/8, height*2/8, spacing, 0, true);
  opc.ledGrid16x16(256,  width * 4/8, height*2/8, spacing, 0, true);
  opc.ledGrid16x16(512,  width * 6/8, height*2/8, spacing, 0, true);
  opc.ledGrid16x16(768,  width * 2/8, height*4/8, spacing, 0, true);
  opc.ledGrid16x16(1024, width * 4/8, height*4/8, spacing, 0, true);
  opc.ledGrid16x16(1280, width * 6/8, height*4/8, spacing, 0, true);
  opc.ledGrid16x16(1536, width * 2/8, height*6/8, spacing, 0, true);
  opc.ledGrid16x16(1792, width * 4/8, height*6/8, spacing, 0, true);
  opc.ledGrid16x16(2048, width * 6/8, height*6/8, spacing, 0, true);  
  
  
}

void draw()
{
  background(0);
  
  // Change the dot size as a function of time, to make it "throb"
  float dotSize = height/2 * 0.3 * (1.0 + 0.2 * sin(millis() * 0.01));
  
  // Draw it centered at the mouse location
  image(dot, mouseX - dotSize/2, mouseY - dotSize/2, dotSize, dotSize);
}

