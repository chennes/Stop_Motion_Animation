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
* Holding down the z key will cause the last frame taken to be displayed. This can help when
  lining up the next shot.
* Holding down the x key will show a transparent overlay of the previous shot, again, for lining
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
import java.util.concurrent.*;

// Sets the playback and encoding frame rate
static final int FRAMERATE = 15;


String _groupName = "";
String _lastFileName = "";
String _filePath = "";
String _CWD = "";
int _numberOfFrames = 0;
PImage _loadedFrame;
PImage _liveFrame;
boolean _weAreLive = true;
boolean _weAreInReplay = false;
boolean _weAreWaiting = false;
int _currentFrame = 0;
int _waitTime = 0;

// For actually encoding the movie...
boolean _weHaveAudio = false;
File _audioFile;
Process _ffmpegProcess;

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

String _pathToFFMPEG;

// GUI Access Variables
Capture cam;
ControlP5 cp5;
Textlabel folderNameLabel;
Textlabel numFramesLabel;
Textlabel infoLabel;
Slider frameSlider;
ControlGroup messageBox;

void settings () {
  size (WINDOW_WIDTH, WINDOW_HEIGHT); // MUST be the first line in setup (according to the Processing documentation)
  
  // Figure out the path to FFMPEG
  String os = System.getProperty("os.name");
  boolean isWindows = (match(os,"Windows") != null);
  boolean isLinux = (match(os,"Linux") != null);
  boolean isMac = (match(os,"Mac") != null);
  if (isWindows) {
    _pathToFFMPEG = dataPath("") + "\\ffmpeg-3.2.2-win64-static\\bin\\ffmpeg.exe";
  } else if (isLinux) {
    _pathToFFMPEG = dataPath("") + "/ffmpeg-3.2.2-linux64-static/bin/ffmpeg";
  } else if (isMac) {
    println ("Mac users need to manually download and install ffmpeg to Data/ffmpeg-3.2.2-mac64-static/bin/ffmpeg");
    _pathToFFMPEG = dataPath("") + "/ffmpeg-3.2.2-mac64-static/bin/ffmpeg";
  } else {
    println ("Unrecognized OS: " + os);
  }
}

void setup () {
  setupUserInterface ();
  startNewMovie ();
}




void delay(int d)
{
  int time = millis();
  while(millis() - time <= d); // Just loop until we've let enough time pass
}

void internalPauseCall ()
{
  _weAreLive = false;
  _weAreInReplay = false;
  _weAreWaiting = true;
  delay (_waitTime);
  _weAreLive = true;
  _weAreInReplay = false;
  _weAreWaiting = false;
}

void pauseUpdates (int d)
{
  _waitTime = d;
  thread ("internalPauseCall"); // This doesn't freeze the UI, it just doesn't show the camera output or change the info label text
}

void setupUserInterface () {
  cp5 = new ControlP5 (this);
  
  PFont boldFont;
  boldFont = loadFont("MeiryoUI-Bold-12.vlw");
  
  PFont largeFont;
  largeFont = loadFont("SegoeUI-Bold-16.vlw");
  
  // GUI
  folderNameLabel = cp5.addTextlabel("Image Folder Name")
    .setText ("Image Folder Name")
    .setPosition (STANDARD_SPACING, STANDARD_SPACING+5)
    .setFont (boldFont)
    ;
  
  cp5.addButton ("Save and start a new movie")
    .setPosition (2*STANDARD_SPACING+BUTTON_WIDTH, STANDARD_SPACING)
    .setSize(BUTTON_WIDTH,BUTTON_HEIGHT)
    ;
  
  cp5.addButton ("Add to previous movie")
    .setPosition (3*STANDARD_SPACING+2*BUTTON_WIDTH, STANDARD_SPACING)
    .setSize(BUTTON_WIDTH,BUTTON_HEIGHT)
    ;
  
  cp5.addButton ("Create final movie")
    .setPosition (4*STANDARD_SPACING+3*BUTTON_WIDTH, STANDARD_SPACING)
    .setSize(BUTTON_WIDTH,BUTTON_HEIGHT)
    ;
  
  cp5.addButton ("Import")
    .setPosition (WINDOW_WIDTH-BUTTON_WIDTH/2-2*STANDARD_SPACING-BUTTON_WIDTH/3, STANDARD_SPACING)
    .setSize(BUTTON_WIDTH/2,BUTTON_HEIGHT)
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
    .setText ("<z> TO SEE PREVIOUS FRAME")
    .setPosition (STANDARD_SPACING, 4*STANDARD_SPACING+3*BUTTON_HEIGHT)
    ;
    
  cp5.addTextlabel ("Tip3")
    .setText ("<x> TO SEE OVERLAY")
    .setPosition (STANDARD_SPACING, 4*STANDARD_SPACING+4*BUTTON_HEIGHT)
    ;
    
  cp5.addButton ("Delete last photo")
    .setPosition (STANDARD_SPACING,7*STANDARD_SPACING+4*BUTTON_HEIGHT)
    .setSize(BUTTON_WIDTH,BUTTON_HEIGHT)
    ;
    
  infoLabel = cp5.addTextlabel ("Info")
    .setText ("Starting up...\nplease wait")
    .setPosition (STANDARD_SPACING, 20*STANDARD_SPACING+4*BUTTON_HEIGHT)
    .setFont (largeFont)
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
  _weAreLive = false;
  _weAreInReplay = false;
  _weAreWaiting = true;
  if (cam != null) {
    cam.stop();
    cam = null;
  }
  String[] cameras = Capture.list();
  
  if (cameras.length == 0) {
    infoLabel.setValue ("There are no cameras connected.");
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
      infoLabel.setValue ("No camera says it supports 640x480 resolution!");
    } else {
	
	    boolean workingCamera = false;      
      while (!workingCamera && cameraNumber > 0) {
        _weAreLive = false;
        _weAreInReplay = false;
        _weAreWaiting = true;
        print ("Selected camera ");
        print (cameraNumber);
        print (": ");
        println (cameras[cameraNumber]);
        cam = new Capture (this, cameras[cameraNumber]);
        cam.start();
        
        // Test it...
        int counter = 1;
        while (!cam.available() && counter <= 10) {
          print ("Waiting for camera... ");
          print (counter);
          println (" of 10");
          delay (1000);
          counter++;
        }
        
        if (cam.available()) {
          cam.read();
          workingCamera = true;
          infoLabel.setValue ("");
        } else {
          println ("Camera is not available, moving onto the next one...");
          cameraNumber--;
          cam.stop();
          cam = null;
        }
      }
    }
  }
  
  _weAreLive = true;
  _weAreInReplay = false;
  _weAreWaiting = false;
}

public void controlEvent(ControlEvent theEvent) {
  if (theEvent.isController()) {
    String eventName = theEvent.getController().getName();
    if (eventName == "Take Photo            (Spacebar)") {
      takePhoto();
    } else if (eventName == "  >>> Play >>>") {
      playFrames();
    } else if (eventName == "Save and start a new movie") {
      startNewMovie();
    } else if (eventName == "Create final movie") {
      saveMovie();
    } else if (eventName == "Delete last photo") {
      deleteFrame();
    } else if (eventName == "Add to previous movie") {
      loadPrevious();
    } else if (eventName == "Import") {
      cleanFolder();
    } else if (eventName == "Help") {
      launch (_CWD+"data/help/Help.html");
    }
  }
}


public void takePhoto ()
{
  _weAreInReplay = false;
  _weAreLive = true;
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
    
    // Make sure it worked:
    PImage img;
    img = loadImage(filename);
    if (img == null) {
      infoLabel.setValue ("Failed to take photo!\nAre you out of space?");
      print ("Failed to create the image file ");
      print (filename);
      println (" -- Is your drive out of space?");
      success = false;
    } else {
      _lastFileName = filename;
      print ("Wrote frame to ");
      println (filename);
      success = true;
    }
    if (!success) {
      infoLabel.setText("Saving failed!\nCheck available space");
      delay (10000);
    }
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
  infoLabel.setValue ("Waiting for camera...");
  thread("setupCamera");
  _numberOfFrames = 0;
  _weAreLive = true;
  _weAreInReplay = false;
  _currentFrame = 0;
  numFramesLabel.setText (nfc(_numberOfFrames));
}

private void playFrames ()
{  
  _weAreLive = false;
  _weAreInReplay = true;
  frameRate (FRAMERATE);
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
  _weAreLive = false;
  _weAreInReplay = false;
  if (selection != null) {
    _numberOfFrames = 0;
    String pathToFile = selection.getPath();
    _filePath = pathToFile.substring(0,pathToFile.lastIndexOf(File.separator));
    String filename = selection.toPath().getFileName().toString();
    
    // We have to figure out the sequence:
    String[] parts = filename.split ("[_.]");
    _groupName = parts[0];
    
    folderNameLabel.setText (_groupName);
    
    // Now actually load up the old photos:
    int loadingPhotoNumber = 1;
    boolean lastLoadWasSuccessful = true;
    while (lastLoadWasSuccessful) {
      lastLoadWasSuccessful = LoadFrame (loadingPhotoNumber);
      if (lastLoadWasSuccessful == true) {
        _numberOfFrames++;
        numFramesLabel.setText (nfc(_numberOfFrames));
        frameSlider.setRange (1, _numberOfFrames);
        frameSlider.setValue (_numberOfFrames);
        
        String[] msgArray = new String[2];
        msgArray[0] = "Loaded\nFrame ";
        msgArray[1] = nf (_numberOfFrames);
        String msg = join (msgArray, "");
        infoLabel.setValue (msg);
        
      }
      loadingPhotoNumber++;
    }
    loadingPhotoNumber--;
    String[] s = new String[3];
    s[0] = "Loaded ";
    s[1] =  nf(loadingPhotoNumber);
    s[2] = " photos";
    String joined = join (s,"");
    infoLabel.setValue (joined);
    pauseUpdates (2000);
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


void saveMovie () {
  askForAudio();
}

void askForAudio() {
  messageBox = cp5.addGroup("messageBox",width/2 - 150,100,300);
  messageBox.setBackgroundHeight(120);
  messageBox.setBackgroundColor(color(0,0.9));
  messageBox.hideBar();
  Textlabel l = cp5.addTextlabel("messageBoxLabel","Do you have a soundtrack for your movie?",20,20);
  l.moveTo(messageBox);
  
  Button b1 = cp5.addButton("buttonYesAudio")
    .setPosition(65,80)
    .setSize(80,24);
  b1.moveTo(messageBox);
  b1.setColorBackground(color(40));
  b1.setColorActive(color(20));
  b1.setBroadcast(false); 
  b1.setValue(1);
  b1.setBroadcast(true);
  b1.setCaptionLabel("Yes");
  
  Button b2 = cp5.addButton("buttonNoAudio")
    .setPosition(155,80)
    .setSize(80,24);
  b2.moveTo(messageBox);
  b2.setColorBackground(color(40));
  b2.setColorActive(color(20));
  b2.setBroadcast(false);
  b2.setValue(0);
  b2.setBroadcast(true);
  b2.setCaptionLabel("No");
}

// function buttonOK will be triggered when pressing
// the OK button of the messageBox.
void buttonYesAudio(int theValue) {
  println (theValue); // Shut up the compiler
  messageBox.hide();
  selectInput("Choose a file to use as your soundtrack", "setAudioFile");
}


void buttonNoAudio(int theValue) {
  println(theValue); // Shut up the compiler
  messageBox.hide();
  _weHaveAudio = false;
  encodeVideo();
}


void setAudioFile(File selection) {
  _audioFile = selection;
  _weHaveAudio = true;
  encodeVideo();
}

void encodeVideo() {
  // Construct the arguments for FFMPEG:
  String[] ffmpegCall = new String[8];
  ffmpegCall[0] = "\"" + _pathToFFMPEG + "\"";
  
  String[] parts = new String[3];
  parts[0] = _CWD + "Image Files";
  parts[1] = _groupName.replaceAll("[^a-zA-Z0-9\\-_]", "");
  parts[2] = createFilename (_groupName, 9999);
  parts[2] = parts[2].replaceAll("9999","%4d");
  String filename = "\"" + join (parts,File.separator) + "\"";
  
  ffmpegCall[1] = " -i " + filename;
  
  if (_weHaveAudio) {
    ffmpegCall[2] = " -i " + "\"" + _audioFile.getPath() + "\"";
  } else {
    ffmpegCall[2] = "";
  }
  
  float totalTime = _numberOfFrames / float(FRAMERATE);
  
  ffmpegCall[3] = "-r " + nf(FRAMERATE); // Set the frame rate
  ffmpegCall[4] = "-t " + nfs(totalTime,0,3); // Set the duration so that the music gets cut off if it's too long
  ffmpegCall[5] = "-nostdin"; // Don't ever ask for input, just do whatever the default is
  ffmpegCall[6] = "-pix_fmt yuv420p"; // Use the older format to be compatible with Windows Media Player (still needed as of 2017)
  
  ffmpegCall[7] = "\"" + parts[0] + File.separator + parts[1] + File.separator + "FinishedMovie.mp4\"";
  println ("Call to ffmpeg: " + join (ffmpegCall," "));
  
  ProcessBuilder pb = new ProcessBuilder (join(ffmpegCall," "));
  pb.inheritIO(); // This makes FFmpeg print to the main stdout for this program
  try {
    infoLabel.setValue ("Encoding video,\nplease wait...");
    _weAreLive = false;
    _weAreInReplay = false;
    _weAreWaiting = true;
    _ffmpegProcess = pb.start();
    boolean done = false;
    while (!done) {
      try {
        done = _ffmpegProcess.waitFor(100, TimeUnit.MILLISECONDS);
      } catch (InterruptedException e) {
        done = true;
      }
    }
    infoLabel.setValue ("Done encoding.");
    launch (ffmpegCall[7]);
    pauseUpdates(2000);
  } catch (Exception e) {
    infoLabel.setValue ("ERROR!\nMovie failed.");
    pauseUpdates(2000);
  }
  
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
      if (keyPressed && key == 'z' && _loadedFrame != null) {
        infoLabel.setValue ("(Previous frame)");
        image (_loadedFrame, WEBCAM_LEFT, WEBCAM_UPPER);
      } else {
        cam.read();
        _liveFrame = cam.get();
        image (_liveFrame, WEBCAM_LEFT, WEBCAM_UPPER);
        if (keyPressed && key == 'x' && _loadedFrame != null) {
          infoLabel.setValue ("(Overlay view)");
          blend (_loadedFrame, 0, 0, WEBCAM_WIDTH, WEBCAM_HEIGHT, WEBCAM_LEFT, WEBCAM_UPPER, WEBCAM_WIDTH, WEBCAM_HEIGHT, LIGHTEST);
        } else {
          infoLabel.setValue ("(Live view)");
        }
      }
    } else if (_liveFrame != null) {
      image (_liveFrame, WEBCAM_LEFT, WEBCAM_UPPER);
      if (keyPressed && key == 'x' && _loadedFrame != null) {
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
  } else if (_weAreWaiting) {
    // Do not update the info string here!
  } else {
    if (_currentFrame <= _numberOfFrames && _loadedFrame != null) {
      image (_loadedFrame, WEBCAM_LEFT, WEBCAM_UPPER);
    } else {
      // Just do nothing, I guess...
    }
  }
}