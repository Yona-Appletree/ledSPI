OPC opc;
PShader blur;
PGraphics src;
PGraphics pass1, pass2;

void setup()
{
  size(640, 320, P2D);

  blur = loadShader("blur.glsl");
  blur.set("blurSize", height / 8);
  blur.set("sigma", 10.0f);  
  
  src = createGraphics(width, height, P3D); 
  
  pass1 = createGraphics(width, height, P2D);
  pass1.noSmooth();  
  
  pass2 = createGraphics(width, height, P2D);
  pass2.noSmooth();

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

}

void draw()
{
  float t = millis() * 0.001;
  randomSeed(0);
  
  src.beginDraw();
  src.noStroke();
  src.background(255, 0, 0);
  src.fill(255, 150);
  src.blendMode(NORMAL);

  src.directionalLight(255, 255, 255, -1, 0, 0.4);
  src.directionalLight(50, 50, 50, -1, 0, 0.2);
  src.directionalLight(50, 50, 50, -1, 0, 0);
  src.directionalLight(255, 0, 0, 1, 0, 0);

  // Lots of rotating cubes
  for (int i = 0; i < 80; i++) {
    src.pushMatrix();

    // This part is the chaos demon.
    src.translate(map(noise(random(1000), t * 0.07), 0, 1, -width, width*2),
      map(noise(random(1000), t * 0.07), 0, 1, -height, height*2), 0);

    // Progression of time
    src.rotateY(t * 0.4 + randomGaussian());
    src.rotateX(t * 0.122222 + randomGaussian());

    // But of course.
    src.box(height * abs(0.2 + 0.2 * randomGaussian()));
    src.popMatrix();
  }

  // Brighten if we care
  /*
  src.noLights();
  src.blendMode(ADD);
  src.fill(40, 0, 0);
  src.rect(0, 0, width, height);
  */

  // Separable blur filter
  src.endDraw();

  blur.set("horizontalPass", 0);
  pass1.beginDraw();            
  pass1.shader(blur);  
  pass1.image(src, 0, 0);
  pass1.endDraw();
  
  blur.set("horizontalPass", 1);
  pass2.beginDraw();            
  pass2.shader(blur);  
  pass2.image(pass1, 0, 0);
  pass2.endDraw();    
        
  image(pass2, 0, 0);
}

