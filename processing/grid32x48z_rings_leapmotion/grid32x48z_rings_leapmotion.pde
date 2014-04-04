// Experimental Leap Motion version of the "rings" example

import de.voidplus.leapmotion.*;

OPC opc;
LeapMotion leap;
float dx, dy, dz;

void setup()
{
  int zoom = 4;
  size(24*zoom, 8*zoom);
  size(160,80);
  
  leap = new LeapMotion(this);

   // Connect to an LEDscape opc-rx process. Only one client can be connected at a time.
  opc = new OPC(this, "beaglebone.local", 7890);
  // Map an 16x16 grid of LEDs to the center of the window, scaled to take up most of the space
  float spacing = height / 32.0;
  opc.ledGrid16x16(0,    width * 2/8, height*1/4, spacing, 0, true);
  opc.ledGrid16x16(256,  width * 4/8, height*1/4, spacing, 0, true);
  opc.ledGrid16x16(512,  width * 6/8, height*1/4, spacing, 0, true);
  opc.ledGrid16x16(768,  width * 2/8, height*3/4, spacing, 0, true);
  opc.ledGrid16x16(1024, width * 4/8, height*3/4, spacing, 0, true);
  opc.ledGrid16x16(1280, width * 6/8, height*3/4, spacing, 0, true);
  

  
  colorMode(HSB, 100);
}

float noiseScale=0.02;

float fractalNoise(float x, float y, float z) {
  float r = 0;
  float amp = 1.0;
  for (int octave = 0; octave < 4; octave++) {
    r += noise(x, y, z) * amp;
    amp /= 2;
    x *= 2;
    y *= 2;
    z *= 2;
  }
  return r;
}

void draw() {
  long now = millis();
  float speed = 0.002;
  float zspeed = 0.3;
  float angle = sin(now * 0.001);
  float z = now * 0.00008;
  float scale = 0.005;

  float saturation = 100 * constrain(pow(1.17 * noise(now * 0.000122), 2.5), 0, 1);
  float spacing = noise(now * 0.000124) * 0.1;

  dx += cos(angle) * speed;
  dy += sin(angle) * speed;
  dz += (noise(now * 0.000014) - 0.5) * zspeed;

  float centerx = noise(now *  0.000125) * 1.25 * width;
  float centery = noise(now * -0.000125) * 1.25 * height;

  float hx = 0, hy = 0, hz = 0;
  for (Hand hand : leap.getHands()) {
     PVector position = hand.getStabilizedPosition();
     hx += position.x;
     hy += position.y;
     hz += position.z;
  }

  // Simple hue/brightness theremin
  dx = hx * -0.01;
  float bright = pow(hy * 0.02, 5.0);

  loadPixels();
  for (int x=0; x < width; x++) {
    for (int y=0; y < height; y++) {
     
      float dist = sqrt(pow(x - centerx, 2) + pow(y - centery, 2));
      float pulse = (sin(dz + dist * spacing) - 0.3) * 0.3;
      
      float n = fractalNoise(dx + x*scale + pulse, dy + y*scale, z) - 0.45;
      float m = fractalNoise(dx + x*scale, dy + y*scale, z + 10.0) - 0.75;

      color c = color(
         (hx + 40.0 * m + hz * 0.1) % 100.0,
         saturation,
         100 * constrain(bright * pow(3.0 * n, 1.5), 0, 0.9)
         );
      
      pixels[x + width*y] = c;
    }
  }
  updatePixels();
}

