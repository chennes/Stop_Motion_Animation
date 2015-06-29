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
Textlabel folderNameLabel;
RadioButton fpsRadio;
Textlabel numFramesLabel;
Slider frameSlider;

String groupName = "";
String lastFileName = "";
String filePath = "";
int sceneNumber = 1;
int numberOfFrames = 0;
PImage loadedFrame;
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
  folderNameLabel = cp5.addTextlabel("Image Folder Name")
    .setText ("Image Folder Name")
    .setPosition (20,20)
    ;
  
  cp5.addButton ("Start a new movie")
    .setPosition (300, 20)
    .setSize(200,25)
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
    .addItem("30",3)
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
  cp5.mapKeyFor(new ControlKey() {
    public void keyEvent() {
        takePhoto();
    }
  }, ' ');
  
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
  int sec = second();
  String[] date = new String[6];
  date[0] = nf(y,4); // The nf() function creates a fixed-precision number with the specified
  date[1] = nf(m,2); // number of digits (here four for the year and two for everything else)
  date[2] = nf(d,2);
  date[3] = nf(h,2);
  date[4] = nf(min,2);
  date[5] = nf(sec,2);
  groupName = join(date, "-");
  folderNameLabel.setText(groupName);
}
  
void setupCamera () {
  if (cam != null) {
    cam.stop();
    cam = null;
  }
  String[] cameras = Capture.list();
  
  if (cameras.length == 0) {
    println ("There are no cameras connected.");
    //exit();
  } else {
    println ("Available cameras:");
    for (int i = 0; i < cameras.length; i++) {
      println (cameras[i]);
    }
    
    // Find the LAST camera that says it is 640x480:
    int cameraNumber = -1;
    for (int i = cameras.length - 1; i >= 0; i--) {
      if (match(cameras[i], "640x480") != null) {
        cameraNumber = i;
        break;
      }
    }
    if (cameraNumber < 0) {
      println ("No camera says it supports 640x480 resolution!");
    } else {
      print ("Selected camera ");
      print (cameraNumber);
      print (": ");
      println (cameras[cameraNumber]);
      cam = new Capture (this, cameras[cameraNumber]);
      cam.start();
    }
  }
}

public void controlEvent(ControlEvent theEvent) {
  if (theEvent.isController()) {
    String eventName = theEvent.getController().getName();
    if (eventName == "Take Photo            (Spacebar)") {
      takePhoto();
    } else if (eventName == "  >>> Play >>>") {
      playFrames();
    } else if (eventName == "Start a new movie") {
      setupDefaultGroupName();
      setupCamera();
      numberOfFrames = 0;
      weAreLive = true;
      weAreInReplay = false;
      currentFrame = 0;
      numFramesLabel.setText (nfc(numberOfFrames));
    } else if (eventName == "Jump to Live View") {
      weAreLive = true;
      weAreInReplay = false;
      currentFrame = numberOfFrames+1;
      numFramesLabel.setText (nfc(numberOfFrames));
      frameSlider.setRange (1, numberOfFrames);
      frameSlider.setValue (numberOfFrames);
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
      cam.read();
      loadedFrame = cam.get();
      numberOfFrames++;
      numFramesLabel.setText (nfc(numberOfFrames));
      frameSlider.setRange (1, numberOfFrames);
      frameSlider.setValue (numberOfFrames);
      
      // Create the filename:
      String[] parts = new String[3];
      parts[0] = "Image Files";
      parts[1] = groupName.replaceAll("[^a-zA-Z0-9\\-_]", "");
      parts[2] = createFilename (groupName, numberOfFrames);
      String filename = join (parts,File.separator);
      loadedFrame.save (filename);
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

private String createFilename (String group, int frame)
{
  // Strip the group name of anything but letters, numbers, dashes, and underscores
  String sanitizedGroupName = group.replaceAll("[^a-zA-Z0-9\\-_]", "");
  String filename = sanitizedGroupName + "_Frame" + nf(frame,4) + ".tif";
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
  currentFrame = 1;
}

private void deleteFrame ()
{
  if (numberOfFrames > 0) {
    File f = new File (lastFileName);
    if (f.exists()) {
      f.delete();
    }
    numberOfFrames--;
    currentFrame = numberOfFrames;
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
    numberOfFrames = 0;
    String pathToFile = selection.getPath();
    filePath = pathToFile.substring(0,pathToFile.lastIndexOf(File.separator));
    String filename = selection.toPath().getFileName().toString();
    
    // We have to figure out the sequence:
    String[] parts = filename.split ("[_.]");
    groupName = parts[0];
    int selectedFrameNumber = Integer.parseInt(parts[1].substring(5));
    String extension = parts[2];
    
    folderNameLabel.setText (groupName);
    
    // Now actually load up the old photos:
    int loadingPhotoNumber = 1;
    boolean lastLoadWasSuccessful = true;
    int failCount = 0;
    int MAX_FAILS_BEFORE_STOP = 100;
    while (lastLoadWasSuccessful) {
      lastLoadWasSuccessful = LoadFrame (loadingPhotoNumber);
      if (lastLoadWasSuccessful == true) {
        numberOfFrames++;
        numFramesLabel.setText (nfc(numberOfFrames));
        frameSlider.setRange (1, numberOfFrames);
        frameSlider.setValue (numberOfFrames);
      } else {
        failCount++;
      }
      loadingPhotoNumber++;
    }
    loadingPhotoNumber--;
    print ("Successfully loaded ");
    print (loadingPhotoNumber);
    print (" photos from files like ");
    println (filename);
  }
}


boolean LoadFrame (int frameNumber)
{
  boolean lastLoadWasSuccessful = false;
  String[] parts = new String[3];
  parts[0] = "Image Files";
  parts[1] = groupName.replaceAll("[^a-zA-Z0-9\\-_]", "");
  parts[2] = createFilename (groupName, frameNumber);
      
  String filename = join (parts,File.separator);
  println (filename);
  PImage newFrame = loadImage (filename);
  
  if (newFrame == null) {
    lastLoadWasSuccessful = false;
  } else {
    loadedFrame = newFrame;
    lastLoadWasSuccessful = true;
  }
  
  return lastLoadWasSuccessful;
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
      if (keyPressed && keyCode == CONTROL && loadedFrame != null) {
        image (loadedFrame, 260, 200);
      } else {
        cam.read();
        liveFrame = cam.get();
        image (liveFrame, 260, 200);
        if (keyPressed && keyCode == SHIFT && loadedFrame != null) {
          blend (loadedFrame, 0, 0, 640, 480, 260, 200, 640, 480, LIGHTEST);
        }
      }
    } else if (liveFrame != null) {
      image (liveFrame, 260, 200);
      if (keyPressed && keyCode == SHIFT && loadedFrame != null) {
        blend (loadedFrame, 0, 0, 640, 480, 260, 200, 640, 480, LIGHTEST);
      }
    }
  } else if (weAreInReplay) {
    // Show the current frame instead:
    boolean success = LoadFrame (currentFrame);
    if (success == true) {
      image (loadedFrame, 260, 200);
      currentFrame++;
      frameSlider.setValue (currentFrame);
    }
    if (currentFrame > numberOfFrames || success == false) {
      weAreLive = true;
      weAreInReplay = false;
      currentFrame = numberOfFrames+1;
      frameSlider.setValue (currentFrame);
      frameRate (30);
    }
  } else {
    if (currentFrame <= numberOfFrames && loadedFrame != null) {
      image (loadedFrame, 260, 200);
    } else {
      // Just do nothing, I guess...
    }
  }
}


