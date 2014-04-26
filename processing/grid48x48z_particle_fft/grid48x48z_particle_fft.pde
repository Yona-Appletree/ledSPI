// Some real-time FFT! This visualizes music in the frequency domain using a
// polar-coordinate particle system. Particle size and radial distance are modulated
// using a filtered FFT. Color is sampled from an image.

import ddf.minim.analysis.*;
import ddf.minim.*;

OPC opc;
PImage dot;
PImage colors;
Minim minim;
AudioPlayer sound;
FFT fft;
float[] fftFilter;

//String filename = "/Users/micah/Dropbox/music/Mark Farina - Mushroom Jazz Vol 5.mp3";
//String filename = "//DISKSTATION/music/Catch 22/Keasbey Nights/Catch 22_Keasbey Nights_05_Walking Away.mp3";
String filename = "//DISKSTATION/music/Arcade Fire/The Suburbs - Digital/01 The Suburbs.mp3";

float spin = 0.001;
float radiansPerBucket = radians(2);
float decay = 0.97;
float opacity = 50;
float minSize = 0.1;
float sizeScale = 0.3;

void setup()
{
  //size(600, 300, P3D);

  minim = new Minim(this); 

  // Small buffer size!
  sound = minim.loadFile(filename, 512);
  sound.loop();
  fft = new FFT(sound.bufferSize(), sound.sampleRate());
  fftFilter = new float[fft.specSize()];

  dot = loadImage("dot.png");
  colors = loadImage("colors.png");

  // Map an 16x16 grid of LEDs to the center of the window, scaled to take up most of the space
  size(120, 120);
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

  fft.forward(sound.mix);
  for (int i = 0; i < fftFilter.length; i++) {
    fftFilter[i] = max(fftFilter[i] * decay, log(1 + fft.getBand(i)));
  }
  
  for (int i = 0; i < fftFilter.length; i += 3) {   
    color rgb = colors.get(int(map(i, 0, fftFilter.length-1, 0, colors.width-1)), colors.height/2);
    tint(rgb, fftFilter[i] * opacity);
    blendMode(ADD);
 
    float size = height/2 * (minSize + sizeScale * fftFilter[i]);
    PVector center = new PVector(width * (fftFilter[i] * 0.2), 0);
    center.rotate(millis() * spin + i * radiansPerBucket);
    center.add(new PVector(width * 0.5, height * 0.5));
 
    image(dot, center.x - size/2, center.y - size/2, size, size);
  }
}

