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
3) Line up your shot.
4) Press the spacebar, or click the "Take Photo" button
5) Repeat 4 and 5 until you've got some frames
6) Click ">>>PLAY>>>" to see the movie.

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
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.*;


String _groupName = "";
String _lastFileName = "";
String _filePath = "";
String _CWD = "";
int _numberOfFrames = 0;
PImage _loadedFrame;
PImage _liveFrame;
boolean _weAreLive = true;
boolean _weAreInReplay = false;
int _currentFrame = 0;

// GUI Constants
int BUTTON_WIDTH = 150;
int BUTTON_HEIGHT = 25;
int STANDARD_SPACING = 10;

int WEBCAM_LEFT = 2*STANDARD_SPACING+BUTTON_WIDTH;
int WEBCAM_UPPER = 3*STANDARD_SPACING+2*BUTTON_HEIGHT;

// Note that changing these doesn't actually change the webcam resolution selected, you will have
// to do that in the code below, in the setupCamera() function...
int WEBCAM_WIDTH = 640;
int WEBCAM_HEIGHT = 480;

int WINDOW_WIDTH = WEBCAM_WIDTH + 3*STANDARD_SPACING + BUTTON_WIDTH;
int WINDOW_HEIGHT = WEBCAM_HEIGHT + 5*STANDARD_SPACING + 3 * BUTTON_HEIGHT;

// GUI Access Variables
Capture cam;
ControlP5 cp5;
Textlabel folderNameLabel;
Textlabel numFramesLabel;
Slider frameSlider;


void setup () {
  setupUserInterface ();
  startNewMovie ();
}

void setupUserInterface () {
  size (WINDOW_WIDTH, WINDOW_HEIGHT);
  cp5 = new ControlP5 (this);
  
  PFont boldFont;
  boldFont = loadFont("MeiryoUI-Bold-12.vlw");
  
  // GUI
  folderNameLabel = cp5.addTextlabel("Image Folder Name")
    .setText ("Image Folder Name")
    .setPosition (STANDARD_SPACING, STANDARD_SPACING+5)
    .setFont (boldFont)
    ;
  
  cp5.addButton ("Start a new movie")
    .setPosition (2*STANDARD_SPACING+BUTTON_WIDTH, STANDARD_SPACING)
    .setSize(BUTTON_WIDTH,BUTTON_HEIGHT)
    ;
  
  cp5.addButton ("Add to previous movie")
    .setPosition (3*STANDARD_SPACING+2*BUTTON_WIDTH, STANDARD_SPACING)
    .setSize(BUTTON_WIDTH,BUTTON_HEIGHT)
    ;
  
  cp5.addButton ("Import and clean frames")
    .setPosition (WINDOW_WIDTH-BUTTON_WIDTH-2*STANDARD_SPACING-BUTTON_WIDTH/3, STANDARD_SPACING)
    .setSize(BUTTON_WIDTH,BUTTON_HEIGHT)
    ;
  
  cp5.addButton ("Help")
    .setPosition (WINDOW_WIDTH-BUTTON_WIDTH/3-STANDARD_SPACING, STANDARD_SPACING)
    .setSize(BUTTON_WIDTH/3,BUTTON_HEIGHT)
    ;
    
  cp5.addTextlabel ("Number of Frames")
    .setText ("NUMBER OF FRAMES: ")
    .setPosition (WINDOW_WIDTH-1.5*BUTTON_WIDTH, 3*STANDARD_SPACING+BUTTON_HEIGHT)
    ;
    
  numFramesLabel = cp5.addTextlabel ("frames")
    .setText ("0")
    .setPosition (WINDOW_WIDTH-.5*BUTTON_WIDTH, 3*STANDARD_SPACING+BUTTON_HEIGHT)
    ;
   
  cp5.addButton ("Take Photo            (Spacebar)")
    .setPosition (STANDARD_SPACING,3*STANDARD_SPACING+2*BUTTON_HEIGHT)
    .setSize(BUTTON_WIDTH,BUTTON_HEIGHT)
    .setColorBackground (color (40, 120, 40))
    ;
    
  // This makes the spacebar activate the takePhoto() function...
  cp5.mapKeyFor(new ControlKey() {
    public void keyEvent() {
        takePhoto();
    }
  }, ' ');
  
  cp5.addTextlabel ("Tip2")
    .setText ("<CTRL> TO SEE PREVIOUS FRAME")
    .setPosition (STANDARD_SPACING, 4*STANDARD_SPACING+3*BUTTON_HEIGHT)
    ;
    
  cp5.addTextlabel ("Tip3")
    .setText ("<SHIFT> TO SEE OVERLAY")
    .setPosition (STANDARD_SPACING, 4*STANDARD_SPACING+4*BUTTON_HEIGHT)
    ;
    
  cp5.addButton ("Delete Last Photo")
    .setPosition (STANDARD_SPACING,7*STANDARD_SPACING+4*BUTTON_HEIGHT)
    .setSize(BUTTON_WIDTH,BUTTON_HEIGHT)
    ;
       
  cp5.addButton ("  >>> Play >>>")
    .setPosition (STANDARD_SPACING, WEBCAM_UPPER+WEBCAM_HEIGHT+STANDARD_SPACING)
    .setSize(BUTTON_WIDTH,BUTTON_HEIGHT)
    ;
    
  frameSlider = cp5.addSlider ("Frame")
    .setPosition (WEBCAM_LEFT,WEBCAM_UPPER+WEBCAM_HEIGHT+STANDARD_SPACING)
    .setSize (WEBCAM_WIDTH-30,BUTTON_HEIGHT)
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
  _groupName = join(date, "-");
  folderNameLabel.setText(_groupName);
  _CWD = sketchPath("");
}
  
void setupCamera () {
  if (cam != null) {
    cam.stop();
    cam = null;
  }
  String[] cameras = Capture.list();
  
  if (cameras.length == 0) {
    println ("There are no cameras connected.");
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
      startNewMovie();
    } else if (eventName == "Delete Last Photo") {
      deleteFrame();
    } else if (eventName == "Add to previous movie") {
      loadPrevious();
    } else if (eventName == "Import and clean frames") {
      cleanFolder();
    } else if (eventName == "Help") {
      open (_CWD+"data/help/Help.html");
    }
  }
}


public void takePhoto ()
{
  _weAreInReplay = false;
  _weAreLive = true;
  int badCounter = 0;
  int MAX_BAD_FRAMES_BEFORE_GIVING_UP = 100;
  boolean success = false;

  if (_liveFrame != null) {
    _loadedFrame = _liveFrame;
    _numberOfFrames++;
    numFramesLabel.setText (nfc(_numberOfFrames));
    frameSlider.setRange (1, _numberOfFrames);
    frameSlider.setValue (_numberOfFrames);
    
    // Create the filename:
    String[] parts = new String[3];
    parts[0] = _CWD + File.separator + "Image Files";
    parts[1] = _groupName.replaceAll("[^a-zA-Z0-9\\-_]", "");
    parts[2] = createFilename (_groupName, _numberOfFrames);
    String filename = join (parts,File.separator);
    _loadedFrame.save (filename);
    _lastFileName = filename;
    print ("Wrote frame to ");
    println (filename);
    success = true;
  }
}

private String createFilename (String group, int frame)
{
  // Strip the group name of anything but letters, numbers, dashes, and underscores
  String sanitized_groupName = group.replaceAll("[^a-zA-Z0-9\\-_]", "");
  String filename = sanitized_groupName + "_Frame" + nf(frame,4) + ".tif";
  return filename;
}

private void startNewMovie ()
{
  setupDefaultGroupName();
  setupCamera();
  _numberOfFrames = 0;
  _weAreLive = true;
  _weAreInReplay = false;
  _currentFrame = 0;
  numFramesLabel.setText (nfc(_numberOfFrames));
}

private void playFrames ()
{
  println ("Playing frames");
  
  int fps = 15;
  _weAreLive = false;
  _weAreInReplay = true;
  frameRate (fps);
  _currentFrame = 1;
}

private void deleteFrame ()
{
  if (_numberOfFrames > 0) {
    File f = new File (_lastFileName);
    if (f.exists()) {
      f.delete();
    }
    _numberOfFrames--;
    _currentFrame = _numberOfFrames;
    numFramesLabel.setText (nfc(_numberOfFrames));
    frameSlider.setRange (1, _numberOfFrames);
    frameSlider.setValue (_numberOfFrames);
    if (_currentFrame > 0) {
      LoadFrame (_currentFrame);
    }
  }
}

private void loadPrevious ()
{
  selectInput("Choose a file from the sequence you want to load", "loadPreviousFromFile");
}


void loadPreviousFromFile (File selection)
{
  if (selection != null) {
    _numberOfFrames = 0;
    String pathToFile = selection.getPath();
    _filePath = pathToFile.substring(0,pathToFile.lastIndexOf(File.separator));
    String filename = selection.toPath().getFileName().toString();
    
    // We have to figure out the sequence:
    String[] parts = filename.split ("[_.]");
    _groupName = parts[0];
    int selectedFrameNumber = Integer.parseInt(parts[1].substring(5));
    String extension = parts[2];
    
    folderNameLabel.setText (_groupName);
    
    // Now actually load up the old photos:
    int loadingPhotoNumber = 1;
    boolean lastLoadWasSuccessful = true;
    int failCount = 0;
    int MAX_FAILS_BEFORE_STOP = 100;
    while (lastLoadWasSuccessful) {
      lastLoadWasSuccessful = LoadFrame (loadingPhotoNumber);
      if (lastLoadWasSuccessful == true) {
        _numberOfFrames++;
        numFramesLabel.setText (nfc(_numberOfFrames));
        frameSlider.setRange (1, _numberOfFrames);
        frameSlider.setValue (_numberOfFrames);
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
  parts[1] = _groupName.replaceAll("[^a-zA-Z0-9\\-_]", "");
  parts[2] = createFilename (_groupName, frameNumber);
      
  String filename = join (parts,File.separator);
  println (filename);
  PImage newFrame = loadImage (filename);
  
  if (newFrame == null) {
    lastLoadWasSuccessful = false;
  } else {
    _loadedFrame = newFrame;
    lastLoadWasSuccessful = true;
  }
  
  return lastLoadWasSuccessful;
}


void cleanFolder () {
  // Find all of the files in the folder that end in .tif, renumber them, and reload.
  selectFolder("Choose the folder containing the images to rename", "cleanSelectedFolder");
}


void cleanSelectedFolder (File selectedFolder) {
  if (selectedFolder != null) {
    File[] fList = selectedFolder.listFiles();
    ArrayList<File> tifFiles = new ArrayList<File>();
    for (File file : fList) {
      if (file.isFile() && match(file.getName(), "^(.*)\\.tif$") != null) {
        tifFiles.add (file);
      }
    }
    Collections.sort(tifFiles); // Make sure it's alphabetical
    
    for (File file: tifFiles) {
      try{
        _liveFrame = loadImage (file.getCanonicalPath());
        takePhoto();
      } catch (IOException e) {
        println ("getCanonicalPath failed! I have no idea what that means. Giving up.");
        return;
      }
    }
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
  line (STANDARD_SPACING,2*STANDARD_SPACING+BUTTON_HEIGHT,
        WINDOW_WIDTH-STANDARD_SPACING,2*STANDARD_SPACING+BUTTON_HEIGHT);
  
  if (_weAreLive) {
    boolean cameraHasGoodData = (cam != null && cam.available() == true);
    if (cameraHasGoodData) {
      if (keyPressed && keyCode == CONTROL && _loadedFrame != null) {
        image (_loadedFrame, WEBCAM_LEFT, WEBCAM_UPPER);
      } else {
        cam.read();
        _liveFrame = cam.get();
        image (_liveFrame, WEBCAM_LEFT, WEBCAM_UPPER);
        if (keyPressed && keyCode == SHIFT && _loadedFrame != null) {
          blend (_loadedFrame, 0, 0, WEBCAM_WIDTH, WEBCAM_HEIGHT, WEBCAM_LEFT, WEBCAM_UPPER, WEBCAM_WIDTH, WEBCAM_HEIGHT, LIGHTEST);
        }
      }
    } else if (_liveFrame != null) {
      image (_liveFrame, WEBCAM_LEFT, WEBCAM_UPPER);
      if (keyPressed && keyCode == SHIFT && _loadedFrame != null) {
        blend (_loadedFrame, 0, 0, WEBCAM_WIDTH, WEBCAM_HEIGHT, WEBCAM_LEFT, WEBCAM_UPPER, WEBCAM_WIDTH, WEBCAM_HEIGHT, LIGHTEST);
      }
    }
  } else if (_weAreInReplay) {
    // Show the current frame instead:
    boolean success = LoadFrame (_currentFrame);
    if (success == true) {
      image (_loadedFrame, WEBCAM_LEFT, WEBCAM_UPPER);
      _currentFrame++;
      frameSlider.setValue (_currentFrame);
    }
    if (_currentFrame > _numberOfFrames || success == false) {
      _weAreLive = true;
      _weAreInReplay = false;
      _currentFrame = _numberOfFrames+1;
      frameSlider.setValue (_currentFrame);
      frameRate (30);
    }
  } else {
    if (_currentFrame <= _numberOfFrames && _loadedFrame != null) {
      image (_loadedFrame, WEBCAM_LEFT, WEBCAM_UPPER);
    } else {
      // Just do nothing, I guess...
    }
  }
}


