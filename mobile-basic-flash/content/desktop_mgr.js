// Javascript Include for getting apps and launching apps

var applications = [];
var useFlash = false;  //this is set to true in flash_home.js

function setBackground(bg) {
	//alert ('setBackground: ' + bg[0] + ',' + bg[1] + ',' + bg[2]);
	if (useFlash) {
		try {
			//alert ('about to start movie');
			var movie = document['themovie'];
			movie.loadBackground(bg[0],bg[1],bg[2]);
		} catch(e) {
			//alert ("Whoa! We threw an exception setting the bkgd: " + e);
			setTimeout(setBackground, 500);
		}
	} else {
		//set HTML bkgd image/color 
		if (document.body){
			document.body.background = bg[0];
			//tbd: set style (centered, tiled, etc)
			//hack for Ken: we like black
			//bg[2] = "#000000"
			document.body.style.backgroundColor = bg[2];
			document.bgColor = bg[2];
			//alert ('setting: ' + document.body + " with " + bg[2]);
		}
	}
}

function setThemeValues(theme) {
	if (useFlash) {
		try {
			//tbd: implement theme awareness in flash
			//alert ('Setting theme');
			var movie = document['themovie'];
			//movie.loadTheme(theme[0],theme[1],theme[2]);
		} catch (e) {
			alert ("Exception setting theme: " + e);
			setTimeout (setThemeValues, 1000);
		}
	} else {
		try {
			updateTheme(theme[0], theme[1], theme[2]);
		} catch(e) {
			alert ("Exception when setting theme: " + e)
		}
	
	}
}

//Called from mobile-basic-home-plugin.cpp
// for each application discovered on system
function addApp(entry) {
	applications.push(entry);
}

function clearApps() {
	applications = [];
}

function launchDesktop() {
  if (useFlash) {
	try {
		//alert (applications);
		var movie = document['themovie'];
		movie.loadApplications(applications);
	} catch(e) {
		// if we attempt to call into the flash before it 
		// is ready, then just setup a timer to try again
		//alert ("Whoa! We threw an exception loading apps: " + e);
		setTimeout(launchDesktop, 500);
	}
  } else {
	try {
		if (applications.length > 0)
        		loadApps(applications);
	} catch(e) {
		alert ("Exception when loading apps: " + e)
	}
  }
}

function umeEvent (name, value) {
	var movie = getFlashMovie("themovie");
	movie.umeEvent(name, value);
}



/* ----------------------------------------------------------
   ----  Application launching API's ------------------------
   ---------------------------------------------------------*/
/* id is value of "MobileId" field in .desktop file */
function launchAppFromId(id) {
	window.status="run_id:" + id;
}

/* path = program and arguments */
function launchAppFromPath(path) {
	window.status="run_path:" + path;
}

/* index is index of order in which app added to movie */
function launchAppFromIndex(index) {
	window.status="run_index:" + index;
}

/* name is value of "Name" field in .desktop file */
function launchAppFromName(name) {
	window.status="run_name:" + name;
}

/* name is value of "X-control-panel-plugin" in control panel .desktop file*/
function launchPlugin(name) {
        window.status="launchPlugin:" + name;
}


function log(msg) {
	window.status = "log:" + msg;
}

// return query string value from current window href
function getQueryStringValue(name) {
	name = name.replace(/[\[]/,"\\\[").replace(/[\]]/,"\\\]");
	var regexS = "[\\?&]"+name+"=([^&#]*)";
	var regex = new RegExp( regexS );
	var results = regex.exec( window.location.href );
	if( results == null )
		return "";
	else
		return results[1];
}



