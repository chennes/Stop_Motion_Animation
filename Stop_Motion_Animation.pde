import processing.video.*;
import controlP5.*;
import java.io.File;

Capture cam;
ControlP5 cp5;
Textfield groupnameField;
Textfield sceneNumberField;
RadioButton fpsRadio;
Textlabel numFramesLabel;
Slider frameSlider;

String groupName = "";
String lastFileName = "";
int sceneNumber = 1;
int numberOfFrames = 0;
ArrayList <PImage> imageSequence = new ArrayList<PImage> ();
PImage liveFrame;
boolean weAreLive = true;
boolean weAreInReplay = false;
int currentFrame = 0;
boolean justShotWithSpacebar = false;

void setup () {
  size (1024, 768);
  cp5 = new ControlP5 (this);
  
  // GUI
  groupnameField = cp5.addTextfield("Group Name")
    .setPosition (20,20)
    .setSize (400,25)
    .setFocus (true)
    ; 
  sceneNumberField = cp5.addTextfield("Scene Number")
   .setPosition(450,20)
   .setSize(80,25)
   .setText("1")
   ;
  cp5.addButton ("Click here to load previous photos")
    .setPosition (550, 20)
    .setSize(200,25)
    ;
  
  cp5.addTextlabel ("Frames per Second")
    .setText ("FRAMES PER SECOND")
    .setPosition (260, 140)
    ;
    
  fpsRadio = cp5.addRadioButton("Playback Speed")
    .setPosition(260,160)
    .setSize(20,20)
    .setItemsPerRow(3)
    .setSpacingColumn(50)
    .setNoneSelectedAllowed (false)
    .addItem("10",1)
    .addItem("15",2)
    .addItem("24",3)
    .activate (1)
    ;
    
  cp5.addTextlabel ("Number of Frames")
    .setText ("NUMBER OF FRAMES: ")
    .setPosition (750, 170)
    ;
    
  numFramesLabel = cp5.addTextlabel ("frames")
    .setText ("0")
    .setPosition (880, 170)
    ;
   
  cp5.addButton ("Take Photo")
    .setPosition (20,200)
    .setSize(200,25)
    ;
  
  cp5.addTextlabel ("Tip2")
    .setText ("HOLD DOWN CTRL TO SEE PREVIOUS FRAME")
    .setPosition (20, 230)
    ;
    
  cp5.addTextlabel ("Tip3")
    .setText ("HOLD DOWN SHIFT TO SEE OVERLAY")
    .setPosition (20, 245)
    ;
    
  cp5.addButton ("Delete Last Photo")
    .setPosition (20,290)
    .setSize(200,25)
    ;
    
  cp5.addButton ("Save as Movie")
    .setPosition (20,655)
    .setSize(200,25)
    ;
    
  cp5.addButton ("  >>> Play >>>")
    .setPosition (20,700)
    .setSize(90,25)
    ;
    
  cp5.addButton ("Jump to Live View")
    .setPosition (130,700)
    .setSize(90,25)
    ;
    
  frameSlider = cp5.addSlider ("Frame")
    .setPosition (260,700)
    .setSize (640,25)
    .setValue (0)
    .setRange (0,0)
    .setSliderMode (Slider.FIX)
    ;
    
    
    // Invent a group name:
  int d = day();
  int m = month();
  int y = year();
  int h = hour();
  int min = minute();
  String[] date = new String[5];
  date[0] = nf(y,4);
  date[1] = nf(m,2);
  date[2] = nf(d,2);
  date[3] = nf(h,2);
  date[4] = nf(min,2);
  groupnameField.setText(join(date, "-"));
  
  justShotWithSpacebar = false;
  
  String[] cameras = Capture.list();
  
  if (cameras.length == 0) {
    println ("There are no cameras connected.");
    //exit();
  } else {
    println ("Available cameras:");
    for (int i = 0; i < cameras.length; i++) {
      println (cameras[i]);
    }
    cam = new Capture (this, cameras[0]);
    cam.start();
  }
}

public void controlEvent(ControlEvent theEvent) {
  if (theEvent.isController()) {
    String eventName = theEvent.getController().getName();
    if (eventName == "Take Photo") {
      takePhoto();
    } else if (eventName == "  >>> Play >>>") {
      playFrames();
    } else if (eventName == "Jump to Live View") {
      weAreLive = true;
      weAreInReplay = false;
      currentFrame = imageSequence.size();
      frameSlider.setValue (currentFrame);
    } else if (eventName == "Delete Last Photo") {
      deleteFrame();
    } else if (eventName == "Click here to load previous photos") {
      loadPrevious();
    }
  }
}


public void takePhoto ()
{
  weAreInReplay = false;
  weAreLive = true;
  if (cam != null && cam.available() == true) {
    PImage frame;
    cam.read();
    frame = cam.get();
    imageSequence.add (frame);
    numberOfFrames++;
    numFramesLabel.setText (nfc(numberOfFrames));
    frameSlider.setRange (1, numberOfFrames);
    frameSlider.setValue (numberOfFrames);
    
    // Create the filename:
    groupName = groupnameField.getText();
    String filename = createFilename (groupName, sceneNumber, numberOfFrames);
    frame.save (filename);
    lastFileName = filename;
    print ("Wrote frame to ");
    println (filename);
  }
}

private String createFilename (String group, int scene, int frame)
{
  String sanitizedGroupName = group.replaceAll("[^a-zA-Z0-9\\-_]", "");
  String filename = sanitizedGroupName + "_Scene" + nf(scene,2) + "_Frame" + nf(frame,4) + ".tif";
  return filename;
}

private void playFrames ()
{
  println ("Playing frames");
  // Get the FPS:
  boolean isTen = fpsRadio.getState ("10");
  boolean isFifteen =  fpsRadio.getState ("15");
  boolean isTwentyFour = fpsRadio.getState ("24");
  
  int fps = 15;
  if (isTen) {
    fps = 10;
  } else if (isFifteen) {
    fps = 15;
  } else if (isTwentyFour) {
    fps = 24;
  }
  weAreLive = false;
  weAreInReplay = true;
  frameRate (fps);
  currentFrame = 0;
}

private void deleteFrame ()
{
  if (imageSequence.size() > 0) {
    File f = new File (lastFileName);
    if (f.exists()) {
      f.delete();
    }
    imageSequence.remove (imageSequence.size() - 1);
    currentFrame = imageSequence.size();
    numberOfFrames--;
    numFramesLabel.setText (nfc(numberOfFrames));
    frameSlider.setRange (1, numberOfFrames);
    frameSlider.setValue (numberOfFrames);
  }
}

private void loadPrevious ()
{
  selectInput("Choose a file from the sequence you want to load", "loadPreviousFromFile");
}


void loadPreviousFromFile (File selection)
{
  if (selection != null) {
    String pathToFile = selection.getPath();
    String filename = selection.toPath().getFileName().toString();
    
    String[] parts = filename.split ("[_.]");
    groupName = parts[0];
    sceneNumber = Integer.parseInt(parts[1].substring(5));
    int selectedFrameNumber = Integer.parseInt(parts[2].substring(5));
    String extension = parts[3];
    
    groupnameField.setText (groupName);
    sceneNumberField.setText (nfc(sceneNumber));
    
    // Now actually load up the old photos:
    int loadingPhotoNumber = 1;
    boolean lastLoadWasSuccessful = true;
    while (lastLoadWasSuccessful) {
      String loadFilename = createFilename (groupName, sceneNumber, loadingPhotoNumber);
      PImage frame = loadImage (loadFilename);
      if (frame == null) {
        lastLoadWasSuccessful = false;
      } else {
        imageSequence.add (frame);
        numberOfFrames++;
        loadingPhotoNumber++;
        numFramesLabel.setText (nfc(numberOfFrames));
        frameSlider.setRange (1, numberOfFrames);
        frameSlider.setValue (numberOfFrames);
      }
    }
    loadingPhotoNumber--;
    print ("Successfully loaded ");
    print (loadingPhotoNumber);
    print (" photos from files like ");
    println (filename);
  }
}


void delay(int delay)
{
  int time = millis();
  while(millis() - time <= delay);
}

void draw() {
  background (0,0,0);
  stroke (255,255,255);
  strokeWeight (3);
  line (20,80,1004,80);
    
  if (weAreLive && cam != null && cam.available() == true) {
    if (keyPressed && keyCode == CONTROL && imageSequence.size() > 0) {
      image (imageSequence.get(imageSequence.size()-1), 260, 200);
    } else {
      cam.read();
      liveFrame = cam.get();
      image (liveFrame, 260, 200);
      if (keyPressed && keyCode == SHIFT && imageSequence.size() > 0) {
        PImage sourceImage = imageSequence.get(imageSequence.size()-1);
        blend (imageSequence.get(imageSequence.size()-1), 0, 0, 640, 480, 260, 200, 640, 480, LIGHTEST);
      }
    }
    //delay(50);
  } else if (liveFrame != null && weAreLive) {
      image (liveFrame, 260, 200);
  } else if (weAreInReplay) {
    // Show the current frame instead:
    image (imageSequence.get(currentFrame), 260, 200);
    currentFrame++;
    frameSlider.setValue (currentFrame);
    if (currentFrame >= imageSequence.size()) {
      currentFrame = imageSequence.size() - 1;
      weAreInReplay = false;
      frameRate (30);
    }
  } else if (currentFrame < imageSequence.size()) {
    image (imageSequence.get(currentFrame), 260, 200);
  }
}


