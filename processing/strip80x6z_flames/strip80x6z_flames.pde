OPC opc;
PImage im;

void setup()
{

  // Load a sample image
  im = loadImage("flames.jpeg");

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
  // Scale the image so that it matches the width of the window
  int imHeight = im.height * width / im.width;

  // Scroll down slowly, and wrap around
  float speed = 0.05;
  float y = (millis() * -speed) % imHeight;
  
  // Use two copies of the image, so it seems to repeat infinitely  
  image(im, 0, y, width, imHeight);
  image(im, 0, y + imHeight, width, imHeight);
}

