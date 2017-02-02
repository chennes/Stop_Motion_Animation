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
import ddf.minim.*; // Sound processing (as of January 2017 the Sound library from Processing doesn't work properly)
import ddf.minim.ugens.*;
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
boolean _weAreGettingAudio = false;
boolean _weAreWaiting = false;
int _currentFrame = 0;
int _waitTime = 0;

// For actually encoding the movie...
boolean _weHaveAudio = false;
Minim _minim;
AudioPlayer _audioPlayer;
MultiChannelBuffer _audioBuffer;
float _audioSampleRate;
File _soundFile;
float _soundStart;
Process _ffmpegProcess;
AudioWaveformVisualizer _awv;
AudioOutput _audioOutput;
Sampler _audioSampler;
boolean _playingSound;
boolean _playingSelectionOnly;

// For the audio selection user interface:
Button buttonPlayAll;
Button buttonPlaySelection;

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
ControlGroup _setSoundPositionUI;

void settings () {
  size (WINDOW_WIDTH, WINDOW_HEIGHT); // MUST be the first line in setup (according to the Processing documentation)
  _minim = new Minim(this);
  _audioBuffer = new MultiChannelBuffer( 1, 1024 ); // Parameters don't really matter here
  _playingSound = false;
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



class AudioWaveformVisualizer extends controlP5.Controller< AudioWaveformVisualizer > {

  protected int _w, _h;
  protected int _selectionStart, _selectionWidth;
  protected int _playhead;
  protected MultiChannelBuffer _buffer;
  protected float _audioSampleRate;
  protected float[] _audioNavigationBuffer;
  protected boolean _draggingWindow;
  protected int _dragStart;
  
  public AudioWaveformVisualizer (ControlP5 theControlP5, java.lang.String theName, MultiChannelBuffer audioBuffer, float sampleRate)
  {
    super (theControlP5, theName);
    _w = width;
    _h = height;
    _selectionStart = 0;
    _selectionWidth = 0;
    _playhead = 0;
    super.setSize(_w, _h);
    _buffer = audioBuffer;
    _audioSampleRate = sampleRate;
    createAudioNavigationBuffer ();
    _draggingWindow = false;
    _myControllerView = new AudioWaveformVisualizerView( );
  }
  
  protected void createAudioNavigationBuffer () {
    int navBufferWidth = _w;
    int navBufferWindow = _audioBuffer.getBufferSize()/navBufferWidth;
    _audioNavigationBuffer = new float[navBufferWidth];
    for (int i = 0; i < navBufferWidth; i++) {
      // For each window calculate the maximum wave value:
      _audioNavigationBuffer[i] = max(subset(_audioBuffer.getChannel(0),i*navBufferWindow, navBufferWindow));
    }
  }
  
  @Override protected void onEnter( ) {
    isActive = true;
  }

  @Override protected void onLeave( ) {
    isActive = false;
    setIsInside( false );
  }

  /**
   * @exclude
   */
  @Override public void mousePressed( ) {
    // See if the mouse is inside the selection window:
    float xPos = mouseX - ( _myParent.getAbsolutePosition( )[0] + position[0] );
    if (_selectionStart >= _selectionStart && xPos < _selectionStart+_selectionWidth) {
      _draggingWindow = true;
      _dragStart = int(xPos);
    } else {
      _draggingWindow = false;
    }
  }

  /**
   * @exclude
   */
  @Override public void mouseReleased( ) {
    float xPos = mouseX - ( _myParent.getAbsolutePosition( )[0] + position[0] );
    if (!_draggingWindow) {
      _playhead = int(xPos);
    } else {
      int distanceDragged = int(xPos) - _dragStart;
      _selectionStart += distanceDragged;
      // Clamp it so it never runs off the end.
      if (_selectionStart + _selectionWidth > _w) {
        _selectionStart = _w - _selectionWidth - 1;
      } else if (_selectionStart < 0) {
        _selectionStart = 0;
      }
    }
    _draggingWindow = false;
  }
  
  @Override public AudioWaveformVisualizer setSize(int theWidth, int theHeight) 
  {
    if (theWidth != _w) {
      _w = theWidth;
      createAudioNavigationBuffer();
    }
    _h = theHeight;
    return super.setSize (theWidth, theHeight);
  }
  
  public void setSelectionWindow (float startTime, float audioLength) {
    float totalLength = _audioBuffer.getBufferSize() / _audioSampleRate;
    _selectionStart = int(map(startTime, 0, totalLength, 0, _w));
    _selectionWidth = int(map(audioLength, 0, totalLength, 0, _w));
  }
  
  public float getSelectionStart () {
    float percentage = float(_selectionStart) / float(_w);
    float sampleLength = _buffer.getBufferSize() / _audioSampleRate;
    return percentage*sampleLength;
  }
  
  public void setPlayheadPosition (float t)
  {
    float totalLength = _buffer.getBufferSize() / _audioSampleRate;
    _playhead = int(map(t, 0, totalLength, 0, _w));
  }
  
  public float getPlayheadPosition ()
  {
    float percentage = float(_playhead) / float(_w);
    float sampleLength = _buffer.getBufferSize() / _audioSampleRate;
    return percentage*sampleLength;
  }
  
  public void setBuffer (MultiChannelBuffer audioBuffer)
  {
    _buffer = audioBuffer;
  }
  
  
  private class AudioWaveformVisualizerView implements ControllerView< AudioWaveformVisualizer > {
    public void display( PGraphics p , AudioWaveformVisualizer theController ) {
      p.fill(100);
      p.rect(0, 0, _w, _h);
      
      p.stroke(255,0,0);
      p.strokeWeight(1);
      
      // Get the mouse position in case we need it
      float xPos = mouseX - ( _myParent.getAbsolutePosition( )[0] + position[0] );
      
      for(int i = 0; i < _w; i++)
      {
        int x1 = i;
        int x2 = i;
        int y1 = _h;
        int y2 = _h-int(_audioNavigationBuffer[i]*_h);
        p.line(x1, y1, x2, y2);
      }
      
      if (!_draggingWindow) {
        // Draw the selection window
        if (isActive && xPos >= _selectionStart && xPos <= _selectionStart+_selectionWidth ) {
          p.fill(255,255,255,100);
        } else {
          p.fill(255,255,255,60);
          
          if (isActive) {
            // Draw the playhead repositioning cursor
            p.stroke(0,0,255);
            p.strokeWeight(1);
            p.line (xPos, 0, xPos, _h);
          }
          
        }          
        p.stroke(255,255,255);
        p.strokeWeight(1);
        p.rect (_selectionStart, 0, _selectionWidth,_h);
      
        // Draw the playhead
        p.stroke(0,255,0);
        p.strokeWeight(1);
        p.line (_playhead, 0, _playhead, _h);
      
      
      } else {
        int distanceDragged = int(xPos) - _dragStart;
        int tempSelectionStart = _selectionStart + distanceDragged;
        // Clamp it so it never runs off the end.
        if (tempSelectionStart + _selectionWidth >= _w) {
          tempSelectionStart = _w - _selectionWidth - 1;
        } else if (tempSelectionStart < 0) {
          tempSelectionStart = 0;
        }
        p.stroke(255,255,255);
        p.strokeWeight(1);
        p.fill(255,255,255,140);
        p.rect (tempSelectionStart, 0, _selectionWidth,_h);
      }
      
    }
  }
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
        if (_weAreGettingAudio) {
          buttonPlayAll(0);
        } else {
          takePhoto();
        }
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
  noLoop();
  _weAreLive = false;
  _weAreInReplay = false;
  _weAreWaiting = false;
  _weAreGettingAudio = true;
  infoLabel.setValue ("Loading audio,\nplease wait...");
  redraw();
  
  _soundFile = selection;
  _audioPlayer = _minim.loadFile(_soundFile.getPath());
  _audioSampleRate = _minim.loadFileIntoBuffer(_soundFile.getPath(),_audioBuffer);
  _soundStart = 0.0;
  _weHaveAudio = true;
  setSoundPosition(_audioSampleRate);
  loop();
}

void setSoundPosition (float sampleRate)
{
  _setSoundPositionUI = cp5.addGroup("soundPosition",0,0,width);
  _setSoundPositionUI.setBackgroundHeight(height);
  _setSoundPositionUI.setBackgroundColor(color(0,0.9));
  _setSoundPositionUI.hideBar();
  Textlabel l = cp5.addTextlabel("soundPositionLabel","Set audio range",STANDARD_SPACING,STANDARD_SPACING);
  l.moveTo(_setSoundPositionUI);
  
  buttonPlayAll = cp5.addButton("buttonPlayAll")
    .setPosition(STANDARD_SPACING,height-BUTTON_HEIGHT-STANDARD_SPACING)
    .setSize(BUTTON_WIDTH,BUTTON_HEIGHT);
  buttonPlayAll.moveTo(_setSoundPositionUI);
  buttonPlayAll.setColorBackground(color(40));
  buttonPlayAll.setColorActive(color(20));
  buttonPlayAll.setBroadcast(false); 
  buttonPlayAll.setValue(1);
  buttonPlayAll.setBroadcast(true);
  buttonPlayAll.setCaptionLabel("Play all");
  
  buttonPlaySelection = cp5.addButton("buttonPlaySelection")
    .setPosition(2*STANDARD_SPACING + BUTTON_WIDTH,height-BUTTON_HEIGHT-STANDARD_SPACING)
    .setSize(BUTTON_WIDTH,BUTTON_HEIGHT);
  buttonPlaySelection.moveTo(_setSoundPositionUI);
  buttonPlaySelection.setColorBackground(color(40));
  buttonPlaySelection.setColorActive(color(20));
  buttonPlaySelection.setBroadcast(false); 
  buttonPlaySelection.setValue(1);
  buttonPlaySelection.setBroadcast(true);
  buttonPlaySelection.setCaptionLabel("Play selection");
  
  Button b2 = cp5.addButton("buttonSave")
    .setPosition(width-2*(BUTTON_WIDTH/2+STANDARD_SPACING),height-BUTTON_HEIGHT-STANDARD_SPACING)
    .setSize(BUTTON_WIDTH/2,BUTTON_HEIGHT);
  b2.moveTo(_setSoundPositionUI);
  b2.setColorBackground(color(40));
  b2.setColorActive(color(20));
  b2.setBroadcast(false);
  b2.setValue(2);
  b2.setBroadcast(true);
  b2.setCaptionLabel("Save");
  
  Button b3 = cp5.addButton("buttonCancel")
    .setPosition(width-1*(BUTTON_WIDTH/2+STANDARD_SPACING),height-BUTTON_HEIGHT-STANDARD_SPACING)
    .setSize(BUTTON_WIDTH/2,BUTTON_HEIGHT);
  b3.moveTo(_setSoundPositionUI);
  b3.setColorBackground(color(40));
  b3.setColorActive(color(20));
  b3.setBroadcast(false);
  b3.setValue(0);
  b3.setBroadcast(true);
  b3.setCaptionLabel("Cancel");
  
  // Now we just draw our audio waveform and set up the interaction with it
  _awv = new AudioWaveformVisualizer(cp5, "awv", _audioBuffer, sampleRate);
  _awv.setSize(width-2*STANDARD_SPACING, height - 2*(2*STANDARD_SPACING+BUTTON_HEIGHT));
  _awv.setPosition(STANDARD_SPACING, BUTTON_HEIGHT+2*STANDARD_SPACING);
  float totalTime = _numberOfFrames / float(FRAMERATE);
  _awv.setSelectionWindow (0,totalTime);
  _awv.setPlayheadPosition (_soundStart);
  _awv.moveTo(_setSoundPositionUI);
}

void buttonPlayAll (int theValue) {
  if (_playingSound) {
    _playingSound = false;
    //b1.setCaptionLabel("Play all");
    _audioPlayer.pause();
    float playhead = _audioPlayer.position() / 1000.0;
    _awv.setPlayheadPosition(playhead);
    buttonPlaySelection.show();
    buttonPlayAll.setCaptionLabel("Play selection");
  } else {
    _playingSound = true;
    buttonPlaySelection.hide();
    buttonPlayAll.setCaptionLabel("Pause");
    _playingSelectionOnly = false;
    //b1.setCaptionLabel("Stop playing");
    float startTime = _awv.getPlayheadPosition();
    _audioPlayer.cue (int(startTime * 1000));
    _audioPlayer.loop();
  }
}

void buttonPlaySelection (int theValue) {
  if (_playingSound) {
    _playingSound = false;
    _audioPlayer.pause();
    buttonPlayAll.show();
    buttonPlaySelection.setCaptionLabel("Play all");
  } else {
    buttonPlayAll.hide();
    buttonPlaySelection.setCaptionLabel("Pause");
    _playingSound = true;
    _playingSelectionOnly = true;
    float playhead = _awv.getPlayheadPosition();
    float selectionStart = _awv.getSelectionStart();
    if (playhead < selectionStart || playhead > selectionStart + float(_numberOfFrames)/15.0) {
      playhead = selectionStart;
      _audioPlayer.cue (int(playhead * 1000));
    }
    _audioPlayer.loop();
  }
}

void buttonSave (int theValue) {
  _setSoundPositionUI.hide();
  encodeVideo();
}

void buttonCancel (int theValue) {
  _setSoundPositionUI.hide();
}

void encodeVideo() {
  // Construct the arguments for FFMPEG:
  String[] ffmpegCall = new String[9];
  ffmpegCall[0] = "\"" + _pathToFFMPEG + "\"";
  
  String[] parts = new String[3];
  parts[0] = _CWD + "Image Files";
  parts[1] = _groupName.replaceAll("[^a-zA-Z0-9\\-_]", "");
  parts[2] = createFilename (_groupName, 9999);
  parts[2] = parts[2].replaceAll("9999","%4d");
  String filename = "\"" + join (parts,File.separator) + "\"";
  
  ffmpegCall[1] = " -r " + nf(FRAMERATE); // Set the input frame rate
  ffmpegCall[2] = " -i " + filename;
  
  if (_weHaveAudio) {
    float selectionStart = _awv.getSelectionStart();
    ffmpegCall[3] = " -ss " + nfs(selectionStart,0,3) + " -i " + "\"" + _soundFile.getPath() + "\"";
  } else {
    ffmpegCall[3] = "";
  }
  
  float totalTime = _numberOfFrames / float(FRAMERATE);
  
  ffmpegCall[4] = "-r " + nf(FRAMERATE); // Set the output frame rate
  ffmpegCall[5] = "-t " + nfs(totalTime,0,3); // Set the duration so that the music gets cut off if it's too long
  ffmpegCall[6] = "-nostdin"; // Don't ever ask for input, just do whatever the default is
  ffmpegCall[7] = "-pix_fmt yuv420p"; // Use the older format to be compatible with Windows Media Player (still needed as of 2017)
  
  ffmpegCall[8] = "\"" + parts[0] + File.separator + parts[1] + File.separator + "FinishedMovie.mp4\"";
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
    launch (ffmpegCall[8]);
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
  } else if (_playingSound) {
    float selectionStart = _awv.getSelectionStart();
    float playhead;
    playhead = _audioPlayer.position() / 1000.0;
    if (_playingSelectionOnly) {
      if (playhead < selectionStart || playhead > selectionStart + float(_numberOfFrames)/15.0) {
        playhead = selectionStart;
        _audioPlayer.cue (int(playhead * 1000));
      }
    }
    _awv.setPlayheadPosition(playhead);
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