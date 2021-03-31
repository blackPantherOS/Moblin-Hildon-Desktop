// Javascript Include for icons layed out in grid

var appBtnSize = new size(134,134);


var bkgdImage = ""
var bkgdStyle = "";  //zoom, tiled, centered, fillscreen, scaled;
var bkgdColor = "#000000";
var btnSize = new size(134,134);
var btnIconSize = new size(48,48);

var START_TIMEOUT = 1500

//images set by the theme
var btnBkgdNormal;
var btnBkgdHover;
var btnBkgdSelected;

var btnTextColorNormal = "#F8AA62";
var btnTextColorHover = "#F9BB73";
var btnTextColorSelected = "#FFFFFF";

var btnTextSizeNormal = "14px";
var btnTextSizeHover = "14px";
var btnTextSizeSelected = "14px";

var themeName;
var iconTheme;
var fontName;

var appLaunching = false;

// size object
function size (width,height) 
{
    this.width = width;
    this.height = height;
}

// object for application attributes
function appObj(index, icon, label, path, category) 
{
    this.index = index;
    this.icon = icon;
    this.label = label;
    this.path = path;
    this.category = category;
}

function setButtonAttributes (_btnSize, _btnIconSize, 
		              _btnBkgdNormal,      _btnBkgdHover,      _btnBkgdSelected,
		              _btnTextColorNormal, _btnTextColorHover, _btnTextColorSelected,
		              _btnTextSizeNormal,  _btnTextSizeHover,  _btnTextSizeSelected)
{
    if (_btnSize) 
	btnSize = _btnSize;

    if (_btnIconSize) 
	btnIconSize = _btnIconSize;

    if (_btnBkgdNormal) 
	btnBkgdNormal = _btnBkgdNormal;

    if (_btnBkgdHover) 
	btnBkgdHover = _btnBkgdHover;

    if (_btnBkgdSelected) 
	btnBkgdSelected = _btnBkgdSelected;
    
    if (_btnTextColorNormal) 
	btnTextColorNormal = _btnTextColorNormal;

    if (_btnTextColorHover) 
	btnTextColorHover = _btnTextColorHover;

    if (_btnTextColorSelected) 
	btnTextColorSelected = _btnTextColorSelected;
    
    if (_btnTextSizeNormal) 
	btnTextSizeNormal = _btnTextSizeNormal;

    if (_btnTextSizeHover) 
	btnTextSizeHover = _btnTextSizeHover;

    if (_btnTextSizeSelected) 
	btnTextSizeSelected = _btnTextSizeSelected;
}

//change button attributes on user action
function buttonEvent(domObj, event, index) 
{
    if (appLaunching) {
	return;
    }

    if (event=='onmouseout') {
	domObj.style.backgroundImage = btnBkgdNormal;
	domObj.style.color = btnTextColorNormal;
	domObj.style.fontSize = btnTextSizeNormal;
    } else if (event=='onmouseover') {
	domObj.style.backgroundImage = btnBkgdHover;
	domObj.style.color = btnTextColorHover;
	domObj.style.fontSize = btnTextSizeHover;
    } else if (event=='onmousedown') {
	domObj.style.backgroundImage = btnBkgdSelected;
	domObj.style.color = btnTextColorSelected;
	domObj.style.fontSize =  btnTextSizeSelected;
    } else if (event=='onclick') {
	//tbd: get notification of application started
	domObj.style.backgroundImage = btnBkgdSelected;
	domObj.style.color = btnTextColorSelected;
	domObj.style.fontSize =  btnTextSizeSelected;
	appLaunching = true;
	setTimeout (appLaunchTimeout, START_TIMEOUT, domObj); //tbd: get notification
	document.body.style.cursor='wait';
	launchAppFromIndex(index);
    }

    return false;
}

function appLaunchTimeout (domObj) 
{
    try {
	appLaunching = false;
	document.body.style.cursor='default';
	domObj.style.backgroundImage = btnBkgdNormal;
	domObj.style.color = btnTextColorNormal;
	domObj.style.fontSize = btnTextSizeNormal;
    } catch (e) {
	alert ("Exception in appLaunchTimeout: " + e);
    }
}

function loadApps(apps, bkgd) 
{
    var numColumns;
    var numRows;
    var rowsNeeded;
    var distributeEvenly = true; 
    var verticalScroll = false;
    var hspace=10;
    var vspace=25;
    var vertScrollbarWidth = 25;

    try {
	var appArray = new Array();
	for (var i=0; i<apps.length; i++) {
	    //appObj fields:          <index>      <icon>      <label>   <path>  <category>
	    appArray[i] = new appObj (apps[i][0], apps[i][2], apps[i][1], "",    "");
	}

	numColumns = Math.floor ( (window.innerWidth-hspace) / (btnSize.width+hspace));
	numColumns = Math.min (apps.length, numColumns);

	rowsNeeded = Math.ceil(apps.length / numColumns);
	numRows = Math.floor( (window.innerHeight-vspace) / (btnSize.height+vspace));
	numRows = Math.min (rowsNeeded, numRows);

	if (distributeEvenly) {
	    //now we know how many will fit. Find initial x,y to center vert and horiz
	    var width = window.innerWidth;
	    if (numRows != rowsNeeded) {
		// Need to account for scroll bar
		width = window.innerWidth - vertScrollbarWidth;
	    }
	    vspace = (window.innerHeight - (numRows*btnSize.height)) / (numRows+1);
	    hspace = (width - (numColumns*btnSize.width)) / (numColumns+1);
	}

	//-- UPDATE DIV --------------------------------
	//create table contetns
	var appIndex = 0;
	var guts = "";
	var x = Math.round(hspace);
	var y = Math.round(vspace);
	    
	for (row=0; row < rowsNeeded && appIndex<appArray.length; row++) {
	    for (col=0; col < numColumns && appIndex<appArray.length; col++, appIndex++ ) {
		ao = appArray[appIndex];
		guts += "<div class='appButton' id='appButton' style=\"top:"+y+"px; left:"+x+"px; " + 
		    "background-image:" + btnBkgdNormal + "; \"\n" + 
		    "onclick=    \"buttonEvent (this, 'onclick',    " + ao.index + ")\"    \n" + 
		    "onmouseover=\"buttonEvent (this, 'onmouseover',null)\"\n" + 
		    "onmouseout= \"buttonEvent (this, 'onmouseout', null)\" \n" + 
		    "onmousedown=\"buttonEvent (this, 'onmousedown',null); return false;\"\n" + 
		    "  <object data='" + ao.icon + "' width='"+btnIconSize.width+"' height='"+btnIconSize.height+"' ></object>\n" + 
		    "  <div class='textOuter'>\n" +
		    "     <div class='textInner'>\n" +
		    "        " + ao.label + "\n"  +
		    "     </div>\n" +
		    "  </div>\n" +
		    "</div>\n";

		x += Math.round(btnSize.width + hspace);
	    }
	    x = hspace;
	    y += Math.round(btnSize.height + vspace);
	}

	var appDiv = document.getElementById('appDiv');
	appDiv.innerHTML = guts;
	appDiv.style.visibility = "visible";

	//set height to add some padding under the last row for scrolling area
	var divHeight = (((rowsNeeded+1)*vspace) + (rowsNeeded*btnSize.height));
	appDiv.style.height = Math.floor(divHeight) + 'px';
    } catch (e) {
	alert ("Exception while loading applications: " + e);
    }
}

function updateTheme(_theme, _icon_theme, _font_theme) 
{
    themeName = _theme;
    iconTheme = _icon_theme;
    fontName = _font_theme;

    //manually extract since HTML doesn't comprehend themes
    btnBkgdNormal =   "url(/usr/share/themes/" + themeName + "/images/mb_gridhome_btn.png)";
    btnBkgdHover  =   "url(/usr/share/themes/" + themeName + "/images/mb_gridhome_btn_prelight.png)";
    btnBkgdSelected = "url(/usr/share/themes/" + themeName + "/images/mb_gridhome_btn_active.png)";
}

//disable selecting text
function disabletext(e)
{
    return false;
}

function reEnable()
{
    return true;
}   

if (window.sidebar) {
    document.onmousedown=disabletext;
    document.onclick=reEnable;
}
