class Ring
{
  float x, y, size, intensity, hue;

  void respawn(float x1, float y1, float x2, float y2)
  {
    // Start at the newer mouse position
    x = x2;
    y = y2;
    
    // Intensity is just the distance between mouse points
    intensity = dist(x1, y1, x2, y2);
    
    // Hue is the angle of mouse movement, scaled from -PI..PI to 0..100
    hue = map(atan2(y2 - y1, x2 - x1), -PI, PI, 0, 100);

  // Default size is based on the screen size
    size = height * 0.1;
  }

  void draw()
  {
    // Particles fade each frame
    intensity *= 0.95;
    
    // They grow at a rate based on their intensity
    size += height * intensity * 0.01;

    // If the particle is still alive, draw it
    if (intensity >= 1) {
      blendMode(ADD);
      tint(hue, 50, intensity);
      image(texture, x - size/2, y - size/2, size, size);
    }
  }
};

OPC opc;
PImage texture;
Ring rings[];
float smoothX, smoothY;
boolean f = false;

void setup()
{
  size(640, 128, P3D);
  colorMode(HSB, 100);
  texture = loadImage("ring.png");

   // Connect to an LEDscape opc-rx process. Only one client can be connected at a time.
  opc = new OPC(this, "beaglebone.local", 7890);

  // Map an 16x16 grid of LEDs to the center of the window, scaled to take up most of the space
  float spacing = height / 16.0;
  opc.ledGrid16x16(0,    width * 1/10, height/2, spacing, 0, true);
  opc.ledGrid16x16(256,  width * 3/10, height/2, spacing, 0, true);
  opc.ledGrid16x16(512,  width * 5/10, height/2, spacing, 0, true);
  opc.ledGrid16x16(768,  width * 7/10, height/2, spacing, 0, true);
  opc.ledGrid16x16(1024, width * 9/10, height/2, spacing, 0, true);

  // We can have up to 100 rings. They all start out invisible.
  rings = new Ring[100];
  for (int i = 0; i < rings.length; i++) {
    rings[i] = new Ring();
  }
}

void draw()
{
  background(0);

  // Smooth out the mouse location. The smoothX and smoothY variables
  // move toward the mouse without changing abruptly.
  float prevX = smoothX;
  float prevY = smoothY;
  smoothX += (mouseX - smoothX) * 0.1;
  smoothY += (mouseY - smoothY) * 0.1;

  // At every frame, randomly respawn one ring
  rings[int(random(rings.length))].respawn(prevX, prevY, smoothX, smoothY);

  // Give each ring a chance to redraw and update
  for (int i = 0; i < rings.length; i++) {
    rings[i].draw();
  }
}

