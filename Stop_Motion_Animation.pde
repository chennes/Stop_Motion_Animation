 /*

Stop Motion Animation
---------------------
Chris Hennes
CTC Assistant, Norman Public Library
Pioneer Library System
chennes@pioneerlibrarysystem.org
---------------------

RUNNING:
1) Connect a webcam to your computer. Wait for the computer to finish loading drivers, if necessary.
2) Click the "Play" button in the upper left corner of this screen
3) If desired, set the group name and scene number at the top of the screen. This controls the 
   filenames of the photos you take.
4) Line up your shot.
5) Press the spacebar, or click the "Take Photo" button
6) Repeat 4 and 5 until you've got some frames
7) Click ">>>PLAY>>>" to see the movie.
8) Click "JUMP TO LIVE VIEW" to return to seeing what the camera sees.

TIPS:
* Holding down the Control key will cause the last frame taken to be displayed. This can help when
  lining up the next shot.
* Holding down the Shift key will show a transparent overlay of the previous shot, again, for lining
  up your next shot.
* You can reload a previous set of frames, which will automatically set your group name, scene
  number, and current frame, so you can append to a previous movie.
* To create a single movie file (i.e. to "encode" a movie) use the "Tools" menu on this editor
  screen and choose "Movie Maker". Your frames are in the same folder this file is in. This will
  create a .mov file. To play it use Quicktime Player or VLC. 

---------------------

ABOUT THIS SOFTWARE:

This software is written in Processing/Java. Processing is available at http://processing.org and is
a free download. The graphical user interface components are provided by the ControlP5 library.

If you are not familiar with Processing, the first thing you have to know is that there are two
functions that are automatically handled: setup() and draw(). setup() is called once when the program
is first started, and then never again. draw() is called repeatedly after that. The rate at which
draw() is called is controlled by the frameRate() function, but it's typically something like 30
times per second. 

The GUI library, ControlP5, also provides the controlEvent() function, which gets called when one of
the controls (buttons, sliders, etc.) changes. You will see that for any given button click, the main
action is to set the current "mode", which the draw() function then uses to determine what to show
on the screen. The important modes for this program are "Live" and "Replay". "Live" means we are
showing the current camera output (technically, the most recent successfully interpreted data from the
camera). "Replay" means we are currently incrementing through the frames to play the movie. If neither
mode is active, we are just showing a single pre-recorded frame.
*/

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

void setup () {
  setupUserInterface ();
  setupDefaultGroupName ();
  setupCamera ();
}

void setupUserInterface () {
  size (1024, 768);
  cp5 = new ControlP5 (this);
  
  // GUI
  groupnameField = cp5.addTextfield("Group Name")
    .setPosition (20,20)
    .setSize (400,25)
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
   
  cp5.addButton ("Take Photo            (Spacebar)")
    .setPosition (20,200)
    .setSize(200,25)
    ;
    
  // This makes the spacebar activate the takePhoto() function...
  cp5.mapKeyFor(new ControlKey() {public void keyEvent() {takePhoto();}}, ' ');
  
  cp5.addTextlabel ("Tip2")
    .setText ("HOLD DOWN <CTRL> TO SEE PREVIOUS FRAME")
    .setPosition (20, 230)
    ;
    
  cp5.addTextlabel ("Tip3")
    .setText ("HOLD DOWN <SHIFT> TO SEE OVERLAY")
    .setPosition (20, 245)
    ;
    
  cp5.addButton ("Delete Last Photo")
    .setPosition (20,290)
    .setSize(200,25)
    ;
    
  // Not actually easy to do in Processing, it turns out...
  //cp5.addButton ("Save as Movie")
  //  .setPosition (20,655)
  //  .setSize(200,25)
  //  ;
    
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
    .setDecimalPrecision (0)
    ;
}
 
 
 void setupDefaultGroupName () {  
  // We want a group name that is unlikely to conflict with any previously-created group
  // names, so just use the date and time. 
  int d = day();
  int m = month();
  int y = year();
  int h = hour();
  int min = minute();
  String[] date = new String[5];
  date[0] = nf(y,4); // The nf() function creates a fixed-precision number with the specified
  date[1] = nf(m,2); // number of digits (here four for the year and two for everything else)
  date[2] = nf(d,2);
  date[3] = nf(h,2);
  date[4] = nf(min,2);
  groupnameField.setText(join(date, "-"));
}
  
void setupCamera () {
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
    if (eventName == "Take Photo            (Spacebar)") {
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
  int badCounter = 0;
  int MAX_BAD_FRAMES_BEFORE_GIVING_UP = 100;
  boolean success = false;
  
  while (badCounter < MAX_BAD_FRAMES_BEFORE_GIVING_UP && success == false) {
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
      success = true;
    } else {
      // If we didn't get data from the camera, wait a bit before trying again
      delay (10);
    }
    badCounter++; // Keep track of the number of times we failed to get the frame
  }
  if (badCounter == MAX_BAD_FRAMES_BEFORE_GIVING_UP) {
    println ("Unable to grab frame.");
  }
}

private String createFilename (String group, int scene, int frame)
{
  // Strip the group name of anything but letters, numbers, dashes, and underscores
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
    
    // We have to figure out the sequence:
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
  while(millis() - time <= delay); // Just loop until we've let enough time pass
}




void draw() {
  background (0,0,0);
  stroke (255,255,255);
  strokeWeight (3);
  line (20,80,1004,80);
  
  if (weAreLive) {
    boolean cameraHasGoodData = (cam != null && cam.available() == true);
    if (cameraHasGoodData) {
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
    } else if (liveFrame != null) {
      image (liveFrame, 260, 200);
      if (keyPressed && keyCode == SHIFT && imageSequence.size() > 0) {
        PImage sourceImage = imageSequence.get(imageSequence.size()-1);
        blend (imageSequence.get(imageSequence.size()-1), 0, 0, 640, 480, 260, 200, 640, 480, LIGHTEST);
      }
    }
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
  } else {
    if (currentFrame < imageSequence.size()) {
      image (imageSequence.get(currentFrame), 260, 200);
    } else {
      // Just do nothing, I guess...
    }
  }
}


