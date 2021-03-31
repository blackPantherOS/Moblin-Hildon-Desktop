import flash.external.ExternalInterface;
import flash.events.Event;

// ----------------------------------------------------
// Linux UMD/MID Flash Home Screen
// GUI Stuffs
// Intel Corporation
// ----------------------------------------------------

// GUI globals
var b_sliding = false;		// tracks icon sliding in progress to prevent weird race conditions
var snd_click = new Sound();	// click sound (attached in init())
var apps = [];			// array of app launchers (icons + captions)
var this_build = "0.5";		// specific flash build 
var num_icons_visible = 4;	// icons showing at once. 4 works, 5 is also good.  could be dynamic
var icon_spacing = -1;		// distance between icons and to left/right of icons to edge (gets set dynamically)

var conf_xml = new XML();	// configuration XML file (conf.xml)

ExternalInterface.addCallback("loadBackground", this, loadBackground);	
ExternalInterface.addCallback("loadApplications", this, loadApplications);	

Stage.scaleMode = StageScaleMode.NO_BORDER; //StageScaleMode.EXACT_FIT
var stageListener:Object = new Object();
Stage.addListener(stageListener);
stageListener.onResize = setStage;
setStage();
loadSettings();

// toggle debug output window
consoleListener = new Object();
consoleListener.onKeyUp = function() {	
		if (Key.getCode() == 16) // shift key
		{
			if (mc_console._visible == true)
				mc_console._visible = false;
			else
				mc_console._visible = true;
		}
	}
// listen for key press
Key.addListener(consoleListener);

//-----------------------------------------------------------
// Function Definitions
//-----------------------------------------------------------

function setStage() {
	mc_background.width = Stage.width;
	mc_background.height = Stage.height	
	console ("setStage called");
    console ("mc_background width: " + mc_background.width + "; mc_background height: " + mc_background.height);
	console ("Stage.width: " + Stage.width + "; Stage height: " + Stage.height);	
}


// debug output function
function console(s,b_plaintext)
{
	mc_console.txt_console.text += s + "\n";
	mc_console.txt_console.scroll = mc_console.txt_console.maxscroll;
        ExternalInterface.call("log", s);
}

function loadSettings() {
	console ("loadSettings called");
	// config file
	conf_xml.onLoad = conf_xmlloaded;
	conf_xml.load("conf.xml");
}

/* This doesn't yet work, as the following function
    is not called until after loadApplications has finished :(
*/
function conf_xmlloaded(success) 
{
	console ("conf_xmlloaded called");
	if (success) {
		tmp = conf_xml.firstChild.attributes.iconsvisible;
		if (tmp > 0 && tmp < 10) {
			console ("TBD (not happening now): Setting visible icons to " + tmp);
			//num_icons_visible = tmp;
		}
	}
}
function loadBackground(bg_file:String, bg_style:String, bg_color:String):Void
{
	//tbd: file may be null.  We could just load a color
	mc_background.loadMovie(bg_file);	
}

function loadApplications(app_array:Array):Void
{
	var app_id;
	var app_title;
	var app_icon;
	var xpos = 0;
	var total_apps = app_array.length;

	mc_console._visible = false;
	console("MID Flash Home - Build " + this_build);
	snd_click.attachSound("click1");
	icon_spacing = (Stage.width / (num_icons_visible + 1));

	//load applications
	for ( i=0; i<total_apps; i++ ) {
		app_id = app_array[i][0];
		app_title = app_array[i][1];
		app_icon = app_array[i][2];

		apps[i] = _root.mc_slider.attachMovie("app_launcher","app_launcher"+i,1000+i);
		apps[i].app_title.text = app_title;
		apps[i].app_title_shadow.text = app_title;
		apps[i].mc_icon.loadMovie(app_icon);
		apps[i].mc_progress._visible = false;
		apps[i].mc_progress.stop(); //important! otherwise flowers rotate in the background
		
		apps[i].launching = false;
		
		apps[i].id = parseInt(app_id);
		apps[i].title = app_title;
		
		xpos = xpos + icon_spacing;
		apps[i]._x = Math.round(xpos);
	}

	mc_slider._width = Math.round(icon_spacing * (total_apps));

	// take all the icons > num_icons_visible, split them in half, 
	// and slide over that many icons to start in the middle
	var icons_not_shown = total_apps - num_icons_visible;
	console("icons not shown: " + icons_not_shown);
	if (total_apps < num_icons_visible) {
		mc_slider._x =  Math.round(((Stage.width - ((total_apps -1) * icon_spacing)) / 2) - icon_spacing);
	} else {
		mc_slider._x =  - (Math.round(icons_not_shown / 2)) * icon_spacing; //split icons, 1/2 to left, 1/2 to right
	}
	updateSliderButtons();  
	mc_console._visible = false;
}

// update the transparency of the slider button to the left/right
function updateSliderButtons()
{
	//update slider buttons
	btn_slider_left._alpha = (mc_slider._x >=0 ? 5 : 50);
	btn_slider_right._alpha = (mc_slider._x + mc_slider._width >= Stage.width ? 50 : 5);
	console("mc_slider._x: " + mc_slider._x  + ", mc_slider._width: " + mc_slider._width + ", Stage.width: " + Stage.width);
}

// move the slider
function slider(dir)
{
	snd_click.start();
	b_sliding = true;
	
	var current_x = mc_slider._x;
	var easeType = mx.transitions.easing.Regular.easeOut;
	var time = .7;
	var mc = mc_slider;
	var begin = current_x;
	var end = current_x;

	var slide_x = icon_spacing * num_icons_visible;
	end = current_x + (dir == "right" ? (-slide_x) : slide_x);

	ballTween = new mx.transitions.Tween(mc, "_x", easeType, begin, end, time, true);
	
	//subscribe to be notified when animation completes
	ballTween.onMotionFinished = function() {
		b_sliding = false;
		updateSliderButtons();		
	};
}

// main slider left button release
btn_slider_right.onPress = function()
{
	if (b_sliding == false)	{
		if (btn_slider_right._alpha > 5) {
			slider("right");
		}
	}
}

// main slider left button release
btn_slider_left.onPress = function()
{
	if (b_sliding == false)	{
		if (btn_slider_left._alpha > 5) {
			slider("left");
		}
	}
}

// launch app when icon is clicked on
function launchApp(index)
{
	if (apps[index].launching == false)
	{
		snd_click.start();
		//show text in flash console (press shift see)
		console("launch '" + apps[index].title + "' (index=" + apps[index].id + ")");
		
		apps[index].mc_progress.play();
		apps[index].mc_progress._visible = true;		
		apps[index].mc_icon._alpha = 40;
		apps[index].launching = true;
		var appName:String = apps[index].path;
		var className:String = "test";
		var result:Object = ExternalInterface.call("launchAppFromIndex", apps[index].id);
		//create a 3 second delay then hide the progress icon.
		setTimeout(hideIconProgress,3000,index);
	}
}


// hide app launch progress animation and run application
function hideIconProgress(id)
{
	apps[id].mc_progress._visible = false;
	apps[id].mc_progress.stop();	
	apps[id].mc_icon._alpha = 100;
	apps[id].launching = false;
}
