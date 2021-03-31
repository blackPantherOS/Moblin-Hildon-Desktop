// Grab and Drag extension by Ian Weiner: ian.weiner (at) gmail.com
//
// See license.txt for more information (GPL)

// Node to Scroll across iframes?
// preempt and reinsert mousedown for speed/compat w/ajax apps?
// Mouse out/dragover?

// better handle on mout kludge?

const gadFF = 0; const gadTB = 1; 
const gadVERSION = "2.0.0";

const gadPrefListener = {
	domain: "grabAndDrag.",
	observe: function(subject, topic, prefName) {
		gadInit();
	}
}
const STRBUNDLE = Components.classes['@mozilla.org/intl/stringbundle;1'].getService(Components.interfaces.nsIStringBundleService);
const gadClipboardHelper = Components.classes["@mozilla.org/widget/clipboardhelper;1"].getService(Components.interfaces.nsIClipboardHelper); 
const prompts = Components.classes["@mozilla.org/embedcomp/prompt-service;1"].getService(Components.interfaces.nsIPromptService);
const gadPrefService = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefService);

var gadContentArea, gadRenderingArea, gadAppContent, gadMainWin, gadCIbrowser, gadCIbox;
var gadScrollObj;
var gadStartX, gadStartY, gadLastX, gadLastY, gadLastMoveX, gadLastMoveY, 
		gadLastTime, gadLastVel;
var gadOn, gadVer, gadFirstCall=true;
var gadStdCursor, gadDragCursor, gadDragInc, gadMult, gadSBMode, gadButton, gadQTonCnt, gadQToffCnt;
var gadTogKey, gadUseCtrl, gadUseAlt, gadUseShift, gadGeckoVers;
var gadMoveTime, gadFlickOn, gadFlickSpeed, gadFlickTimeLimit, gadFlickWholePage, gadHasMoved;
var gadInQT, gadWasScrolling, gadAppPlatform, gadDragDoc, gadTO, gadDownTarget;
var gadCopyOnQToff, gadDownStr, gadStrToCpy, gadPreLevel, gadOnText;

var gadScrollIntervalObj, gadScrollLoopInterval, gadScrollLastLoop, gadScrollLastMoveLoop;
var gadScrollToGo, gadSmoothStop, gadSmoothFactor, gadScrollHoriz;
var gadScrollVel, gadScrollVelMult, gadScrollDistance, gadScrollMaxInterval;
var gadDists = new gadStack(50), gadVels = new gadStack(50), gadTimes = new gadStack(50);
var gadMomOn, gadMomExtent, gadMomVariation, gadMomFriction;

var gadPref = gadPrefService.getBranch("grabAndDrag.");
var gadPrefRoot = gadPrefService.getBranch(null);
var gadPbi = gadPrefRoot.QueryInterface(Components.interfaces.nsIPrefBranchInternal);

function mod(m,n) {	return (m>=0?m%n:(n-((n-m)%n))%n); } // mod should always be positive!
function sgn(n) { if (n<0) return -1; if (n>0) return 1; return 0; }

// a gadStack is an object that holds a stack of data and data analysis routines-- used to 
// hold/analyze user's dragging behavior for momentum implementation. emphasis is on fast 
// writing to data struct during the dragging in order to maximize performance
function gadStack(k) {
	this.entries = new Array(k);
	this.length=0; this.next=0; this.n=k;
	this.add=add; this.clear=clear; this.val=val; 
	this.sum=sum; this.avg=avg; this.biasedAvg = biasedAvg;
	this.sumWithin=sumWithin; this.maxabs=maxabs; this.minabs=minabs;
	function add(x) {
		this.entries[this.next]=x; 
		if (this.length < this.n) this.length++;
		this.next=mod(this.next+1,this.n);
	}
	function clear() { this.length=0; }
	function val(m) { return this.entries[mod(this.next-1-m,this.n)]; }
	function sum() { var i; var tot=0; for (i=0; i<this.length; i++) tot+=this.val(i); return tot; }
	function avg(m,N) { 
		var i; var tot=0; N=Math.min(N,this.length-1);
		for (i=m; i<=N; i++) tot+=this.val(i); 
		return tot/(N-m+1); 
	}
	function biasedAvg(m,N) {  // biased to lower values to reduce large outliers
		var s=0, c=0, i, tot=1.0; N=Math.min(N,this.length-1);
		for (i=m; i<=N; i++) tot*=this.val(i); 
		tot = Math.pow(tot,1/(N-m+1)); 
		for (i=m; i<=N; i++) {
			if (this.val(i) < tot) { s+=this.val(i); c++; }
		}
		return (s>0?s/c:tot);		
	}
	function sumWithin(D) { 
		var dist=0, i; 
		for (i=0; (dist<D)&&(i<this.length); i++) dist+=this.val(i);
		return i-1;
	}
	function maxabs(m,N) { 
		var i; var v=Math.abs(this.val(m)); N=Math.min(N,this.length-1);
		for (i=m; i<=N; i++) v=Math.max(v,Math.abs(this.val(i))); 
		return v; 
	}
	function minabs(m,N) { 
		var i; var v=Math.abs(this.val(m)); N=Math.min(N,this.length-1);
		for (i=m; i<=N; i++) v=Math.min(v,Math.abs(this.val(i))); 
		return v; 
	}
}
function gadShowOptions(intro) {
	if (intro)
		//modified to remove the startup wizard code 
		//window.openDialog("chrome://grabanddrag/content/gadIntro.xul", "gadIntro", "chrome,centerscreen,dialog=no");
		go();
	else 
		window.openDialog("chrome://grabanddrag/content/gadPrefs.xul", "gadPrefs", "chrome,centerscreen,dialog=no");
}
function gadInit() 
{  
	//gadGeckoVers = parseFloat(window.navigator.userAgent.substring(window.navigator.userAgent.lastIndexOf("rv:") + 3));
	gadInQT=false; gadWasScrolling=false; gadHasMoved = false; 
	if (gadFirstCall) {
		gadFirstCall=false;
		var boxes = document.getElementsByTagName("toolbox");
		for (var i=0; i<boxes.length; i++) 
			boxes[i].addEventListener("DOMNodeInserted", gadButtonAdd, false);
		gadVer = gadPrefRoot.getCharPref("grabAndDrag-version");
		gadPrefRoot.setCharPref("grabAndDrag-version", gadVERSION); 
		if (gadVer != gadVERSION) {
			gadShowOptions(true);
		}		
		gadPbi.addObserver(gadPrefListener.domain, gadPrefListener, false);
	}

	gadOn = gadPref.getBoolPref("on");
	gadTogKey = gadPref.getIntPref("togKey");
	gadUseCtrl = gadPref.getBoolPref("useCtrl");
	gadUseAlt = gadPref.getBoolPref("useAlt");
	gadUseShift = gadPref.getBoolPref("useShift");
	gadDragInc = parseFloat(gadPref.getCharPref("dragInc"));
	gadMult = (gadPref.getBoolPref("reverse")?(-1):(1))*parseFloat(gadPref.getCharPref("mult"));
	gadSBMode = gadPref.getBoolPref("sbmode");
	gadStdCursor = gadPref.getCharPref("grabCursor");
	if (gadPref.getBoolPref("samecur")) gadDragCursor = gadStdCursor;
	else gadDragCursor = gadPref.getCharPref("grabbingCursor");	 
	gadFlickOn = gadPref.getBoolPref("flickon");
	gadFlickSpeed = parseFloat(gadPref.getCharPref("flickspeed_ppms"));
	gadFlickTimeLimit = parseFloat(gadPref.getCharPref("flicktimelimit"));
	gadFlickWholePage = (gadPref.getIntPref("flickwhole")==-1);
	gadMomOn = gadPref.getBoolPref("momOn");
	gadMomVariation = parseFloat(gadPref.getCharPref("momVariation"));
	gadMomExtent = parseFloat(gadPref.getCharPref("momExtent")); 
	gadMomFriction = parseFloat(gadPref.getCharPref("momFriction"));
	gadSmoothStop = gadPref.getBoolPref("smoothStop");
	gadButton = gadPref.getIntPref("button");
	gadQTonCnt = gadPref.getIntPref("qtOnCnt");
	gadQToffCnt = gadPref.getIntPref("qtOffCnt");
	gadCopyOnQToff = gadPref.getBoolPref("qtOffCopy");
	gadPreLevel = gadPref.getIntPref("preLevel");
	gadStrict = gadPref.getBoolPref("strict");
	
	// determine which app we're running under, firefox/flock or thunderbird
	if (document.getElementById("main-window")) {
		gadAppPlatform=gadFF; 
		//gadMainWin = document.getElementById("main-window");
		gadContentArea = document.getElementById("content");
		gadRenderingArea = document.getElementById("content").mPanelContainer;
		gadAppContent = document.getElementById("appcontent");
		// for Cool Iris, if installed
		gadCIbrowser = document.getElementById("cooliris-preview-frame");
		gadCIbox = document.getElementById("cooliris-preview-overlay");
		
	} else { 
		gadAppPlatform=gadTB; 
		//gadMainWin = document.getElementById("messengerWindow");
		gadContentArea = document.getElementById("messagepane");
		gadRenderingArea = document.getElementById("messagepane");
		gadAppContent = document.getElementById("messengerWindow");
	}

	// clean up any old listeners
	gadRenderingArea.removeEventListener("mousedown", gadDownHandler, true);
	gadAppContent.removeEventListener("mousedown",gadDownPreempt, true);
	gadRenderingArea.removeEventListener("click",gadClickHandler, true);
	gadAppContent.removeEventListener("DOMContentLoaded", gadLoad, true);
	//gadContentArea.removeEventListener("dragover", gadUpHandler, false);
	gadAppContent.removeEventListener("click",gadClickPreempt, true);
	gadContentArea.removeEventListener("select", gadTabSelect, true);
	if (gadCIbrowser) {
		gadCIbrowser.removeEventListener("mousedown", gadDownHandler, true);
		gadCIbox.removeEventListener("mousedown",gadDownPreempt, true);
		gadCIbrowser.removeEventListener("click",gadClickHandler, true);
		gadCIbox.removeEventListener("click",gadClickPreempt, true);
		gadCIbox.removeEventListener("DOMContentLoaded", gadLoad, true);
	}
	window.removeEventListener("mouseup", gadUpHandler, true);
	window.removeEventListener("mousedown", gadUpHandler, true);
	window.removeEventListener("mousemove", gadMoveScrollBar, true);

	if (gadOn) { 
  	//try { window.goDoCommand("cmd_selectNone"); } catch(err) { }
		gadRenderingArea.addEventListener("mousedown", gadDownHandler, true);
		gadAppContent.addEventListener("mousedown",gadDownPreempt, true);
		gadRenderingArea.addEventListener("click",gadClickHandler, true);
		gadAppContent.addEventListener("click",gadClickPreempt, true);
		gadAppContent.addEventListener("DOMContentLoaded", gadLoad, true);
		gadContentArea.addEventListener("select", gadTabSelect, true);
		if (gadCIbrowser) {
			gadCIbrowser.addEventListener("mousedown", gadDownHandler, true);
			gadCIbox.addEventListener("mousedown",gadDownPreempt, true);
			gadCIbrowser.addEventListener("click",gadClickHandler, true);
			gadCIbox.addEventListener("click",gadClickPreempt, true);
			gadCIbox.addEventListener("DOMContentLoaded", gadLoad, true);
		}
	}
	
	gadUpdateGraphics(true);
}

function gadTabSelect() 
{
	if ((content)&&(content.document))
		gadSetCursor(content.document, ((gadOn&&!gadInQT)?gadStdCursor:"auto"),true);
}

function gadClose() 
{
	gadPbi.removeObserver(gadPrefListener.domain, gadPrefListener);
	gadRenderingArea.removeEventListener("mousedown", gadDownHandler, true);
	gadAppContent.removeEventListener("mousedown",gadDownPreempt, true);
	gadRenderingArea.removeEventListener("click",gadClickHandler, true);
	gadAppContent.removeEventListener("DOMContentLoaded", gadLoad, true);
	//gadContentArea.removeEventListener("dragover", gadUpHandler, false);
	gadAppContent.removeEventListener("click",gadClickPreempt, true);
	window.removeEventListener("mouseup", gadUpHandler, true);
	window.removeEventListener("mousedown", gadUpHandler, true);
	window.removeEventListener("mousemove", gadMoveScrollBar, true);
	var boxes = document.getElementsByTagName("toolbox");
	for (var i=0; i<boxes.length; i++) 
			boxes[i].removeEventListener("DOMNodeInserted", gadButtonAdd, false);
	window.removeEventListener("load", gadInit, false);

	gadContentArea=null; gadRenderingArea=null; gadAppContent=null; gadMainWin=null;
	gadScrollObj=null; gadPrefService=null; gadPref=null;
	gadPrefRoot=null; 
	gadPbi=null;	
}

// The following two preempts are attached at Appcontent to catch clicks on scrollbars etc
// set some variables for later use by down and click handlers
function gadDownPreempt(e) 
{ 
	gadWasScrolling = (gadScrollIntervalObj)?true:false;
	gadHasMoved = false;
	gadOnText = (e.explicitOriginalTarget.nodeType==document.TEXT_NODE || 
							 e.explicitOriginalTarget.nodeName.toLowerCase()=="input" || 
							 e.explicitOriginalTarget.nodeName.toLowerCase()=="textarea");
	//save the selected text for later copying to clipboard if necessary
	if (gadInQT && gadCopyOnQToff) 
		gadDownStr = e.originalTarget.ownerDocument.defaultView.getSelection().toString();
}
// catches clicks for quick toggle
// preempt the normal image size toggling to allow dragging of images
// preempt clicks triggered by drags, clicks meant to stop momentum
function gadClickPreempt(e) 
{ 
	// end QT if clicked outside text node
	if (gadInQT) {
		if (e.button==gadButton && !gadOnText) {	
			if (e.detail==1 && gadCopyOnQToff) gadStrToCpy = gadDownStr; // save for 2nd click if necessary
			if (e.detail==gadQToffCnt) {
				gadTextToggle();
				if (gadCopyOnQToff && gadStrToCpy) {
					try { gadClipboardHelper.copyString(gadStrToCpy); } catch(err) { }
				}
			}
		}
		return true;
	} 
	// everything below called only when G&D is on and not in QT:
	// start quick toggle from [double] clicking text node
	if (e.button==gadButton && e.detail==gadQTonCnt && 
								!gadHasMoved && !gadOnItem(e.originalTarget) && gadOnText) {	
		gadTextToggle();
		return true;
	}
	// preempt image toggling
	if (e.originalTarget.ownerDocument.toggleImageSize && 
				e.originalTarget.nodeName.toLowerCase()=="img" && e.button==gadButton) {					 
		e.preventDefault(); e.stopPropagation(); 
		gadWasScrolling=false; gadHasMoved=false;
		return false;
	}
	// preempt if click was to stop momentum/flick scrolling, and "fake" clicks generated 
	// while dragging. this must be handled at the appcontent level in order to work, but we 
	// only want to preempt WasScrolling clicks if the scrolling really was ended (so user 
	// can still click other appcontent UI elements during scrolling)
	if ((gadWasScrolling && !gadScrollIntervalObj) || gadHasMoved) { 
		e.preventDefault(); e.stopPropagation(); 
		gadWasScrolling=false; gadHasMoved=false;
		return false;
	}
}

function gadOnItem(initialNode) 
{
	var nextNode, currNode, currNodeUnsafe;
	var doc = initialNode.ownerDocument;
	var st = doc.defaultView.getComputedStyle(initialNode, "");
	var cur = st.getPropertyValue("cursor");

	// ordering of these initial cases is important
	if (initialNode.namespaceURI == "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul") return true;
	if (gadPreLevel==2) return false;
	if (gadPreLevel==0 && gadOnText) return true;
	if (initialNode.nodeName.toLowerCase() == "textarea") 
		return !(initialNode.getAttribute("disabled") || initialNode.getAttribute("readonly"));
	if (doc.imageResizingEnabled) return false; 
	if (doc.designMode) if (doc.designMode=="on") return true;  // Gmail html mode editing, e.g.
	if (st.getPropertyValue("-moz-user-select")=="none") return true; //hack for Google maps...
	if ((cur != gadStdCursor) && (cur != "auto")) return true;

	nextNode = initialNode; 
	do {
		currNode = nextNode; 
		if (currNode.wrappedJSObject) currNodeUnsafe=currNode.wrappedJSObject;
		else currNodeUnsafe=currNode;
		try { 
			if (gadStrict) {
				if ((currNodeUnsafe.ondrag) || (currNodeUnsafe.onmousedown) || 
					(currNodeUnsafe.onmouseup) || (currNodeUnsafe.ondragover)) 
					return true; 
			}
			if (currNode.hasAttribute("href")) return true; 
			if ((currNode.nodeName.toLowerCase() == "scrollbar") ||
					(currNode.nodeName.toLowerCase() == "input") ||
					(currNode.nodeName.toLowerCase() == "select") ||
					(currNode.nodeName.toLowerCase() == "option") ||
					(currNode.nodeName.toLowerCase() == "embed") ||
					(currNode.nodeName.toLowerCase() == "object") ||
					(currNode.nodeName.toLowerCase() == "tree") ||
					(currNode.nodeName.toLowerCase() == "applet") ||
					(currNode.nodeName.toLowerCase() == "statusbar")) return true; 
		} catch(err) { }
				nextNode = currNode.parentNode;
	} while (nextNode && currNode != doc.documentElement);
  
	return false;
}

function gadDownHandler(e) 
{
	gadDownTarget = e.originalTarget;
	gadStartY = e.screenY; gadStartX = e.screenX;	
	gadMoveTime=0;
	// special case: grab anywhere mode still allows easy text editing
	if (gadPreLevel==2 && e.button==gadButton && gadButton==0) {
		var f = document.commandDispatcher.focusedElement; 
		var ot = e.explicitOriginalTarget;
		if (f && (f.nodeName.toLowerCase()=="input" || f.nodeName.toLowerCase()=="textarea"
							|| f.nodeName.toLowerCase()=="select" || f.nodeName.toLowerCase()=="option")) { 
			if (ot.nodeName.toLowerCase()=="input" || ot.nodeName.toLowerCase()=="select" || 
					ot.nodeName.toLowerCase()=="option") return true;
			if ((ot.nodeName.toLowerCase()=="textarea") && 
					(!ot.getAttribute("disabled") && !ot.getAttribute("readonly"))) return true;
		}
	}
	// special case: on an item, unless click was to stop scrolling
	if (!gadWasScrolling && gadOnItem(gadDownTarget)) return true;
	// special case: quick toggle or wrong button, unless click was to stop scrolling
	if (!gadWasScrolling && (e.button!=gadButton || gadInQT)) return true; 
	// stop the scrolling
	if (gadScrollIntervalObj) {
		window.clearInterval(gadScrollIntervalObj); 
		gadScrollIntervalObj = null;
		// special case: clicked on the scrollbar or other UI elements not part of page
		if (gadDownTarget.namespaceURI == "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul") return true;
		e.stopPropagation();
	}	
	e.preventDefault();

	//gadContentArea.addEventListener("dragover", gadUpHandler, false);
	gadScrollObj = gadFindNodeToScroll(gadDownTarget);
	window.addEventListener("mouseup", gadUpHandler, true);
	window.addEventListener("mousemove", gadMoveScrollBar, true);
	gadDragDoc = gadDownTarget.ownerDocument;
	if (!gadScrollObj.nodeToScrollX && !gadScrollObj.nodeToScrollY) return false;
	gadRenderingArea.removeEventListener("mousedown", gadDownHandler, true);
	if (gadCIbrowser) gadCIbrowser.removeEventListener("mousedown", gadDownHandler, true);
	window.addEventListener("mousedown", gadUpHandler, true);
	gadLastY = e.screenY; gadLastX = e.screenX;
	gadLastMoveY = e.screenY; gadLastMoveX = e.screenX;
	gadLastVel = 0;
	gadLastTime=(new Date()).getTime(); 
	gadDists.clear(); gadVels.clear(); gadTimes.clear();
	if (gadStdCursor!=gadDragCursor) {
		if (gadDownTarget.ownerDocument)
			gadSetCursor(gadDownTarget.ownerDocument, gadDragCursor, false);
	}
	//return false;
}

function gadMoveScrollBar(e) 
{
	var horiz, time, dX, dY, dXM, dYM, dT, dD, vel;

	e.preventDefault(); e.stopPropagation();   
	window.clearTimeout(gadTO); 
	time = (new Date()).getTime();
	dT = time-gadLastTime; 
	dX = gadLastX - e.screenX;  dY = gadLastY - e.screenY;
	dXM = gadLastMoveX - e.screenX;  dYM = gadLastMoveY - e.screenY;
	horiz = (Math.abs(dX) > Math.abs(dY));
	// ignore slight quick movements 
	if (!gadHasMoved && dT<=50 && 
				Math.abs(gadStartX-e.screenX)<=2 && Math.abs(gadStartY-e.screenY)<=2) {
		return;
	}
  if (!gadHasMoved && (Math.abs(gadStartX-e.screenX) > 2 || Math.abs(gadStartY-e.screenY) > 2)) {
		gadHasMoved = true;
		gadMoveTime = time;
	}
	dX*=gadScrollObj.ratioX; dY*=gadScrollObj.ratioY;
	dXM*=gadScrollObj.ratioX; dYM*=gadScrollObj.ratioY;

	if (gadFlickOn||gadMomOn) {
		if (horiz) dD = dX; else dD = dY;
		if (dT==0) return;
		vel = dD/dT;   
		//accel = sgn(dD)*(vel - gadLastVel)/dT;
		if ((gadScrollHoriz!=horiz) || (dD*gadLastVel<0)) {  
			gadDists.clear(); gadVels.clear(); gadTimes.clear();
		}
		gadDists.add(dD); gadVels.add(vel); gadTimes.add(dT);
		gadLastVel = vel;
	}
	gadScrollHoriz = horiz; 
	gadLastTime = time; gadLastY = e.screenY; gadLastX = e.screenX; 

	if ((Math.abs(dXM)>=gadDragInc) || (Math.abs(dYM)>=gadDragInc)) {
		gadLastMoveY = e.screenY; gadLastMoveX = e.screenX;
		if (horiz && gadScrollObj.nodeToScrollX) gadScrollObj.scrollXBy(Math.ceil(dXM));
		if (!horiz && gadScrollObj.nodeToScrollY) gadScrollObj.scrollYBy(Math.ceil(dYM));
	}
	gadTO=window.setTimeout('gadDists.clear(); gadVels.clear(); gadTimes.clear();', 100); 
}

function gadBigDecel() 
{	
	var N = gadTimes.sumWithin(gadMomExtent); 
	var M = gadVels.maxabs(0,N);
	return (gadVels.maxabs(0,1)/M < gadMomVariation);
}

function gadUpHandler(e) 
{  
	window.clearTimeout(gadTO); 
	var time = (new Date()).getTime(); 
	window.removeEventListener("mousemove", gadMoveScrollBar, true);
	window.removeEventListener("mouseup", gadUpHandler, true);
	window.removeEventListener("mousedown", gadUpHandler, true);
	//gadContentArea.removeEventListener("dragover", gadUpHandler, false); 
	var dT = time-gadLastTime;
	var aT = gadTimes.biasedAvg(1,50);
	// handle flick gestures and momentum
	var scrollStarted=false;
	if ( (dT<2*aT) && (gadFlickOn||gadMomOn) ) {
		var dist, vel, interval, DXt, DYt, totCurDist;
		DXt = e.screenX - gadStartX; DYt = e.screenY - gadStartY;
		DXt*=gadScrollObj.ratioX; DYt*=gadScrollObj.ratioY; 
		
		totCurDist = Math.abs(gadDists.sum()/(gadScrollHoriz?gadScrollObj.ratioX:gadScrollObj.ratioY));
		interval=7 // 200fps
		gadScrollMaxInterval = aT*3;
		if (gadFlickOn && (time-gadMoveTime < gadFlickTimeLimit)) {  //&& (!gadWasScrolling) 
			// FLICK
			vel = sgn(gadVels.val(0))*gadFlickSpeed;
			if (gadFlickWholePage) {
				if (gadScrollHoriz) dist = sgn(vel)*(gadScrollObj.scrollW);
				else dist = sgn(vel)*(gadScrollObj.scrollH);
			} else {
				if (gadScrollHoriz) dist = sgn(vel)*(gadScrollObj.realWidth - 15 - Math.abs(DXt));
				else dist = sgn(vel)*(gadScrollObj.realHeight - 15 - Math.abs(DYt));
			}
			gadInitScroll(dist,interval,vel,gadScrollHoriz,false);
			scrollStarted=true;
		} else if (gadMomOn) {
			// MOMENTUM
			if (!gadBigDecel() && (totCurDist > 10) && 
						(gadTimes.sum() > gadMomExtent/4) && (gadDists.length>1)) {
				vel = sgn(gadVels.val(0))*gadVels.maxabs(0,1);
				if (gadScrollHoriz) dist = sgn(vel)*(gadScrollObj.scrollW);
				else dist = sgn(vel)*(gadScrollObj.scrollH);
				gadInitScroll(dist,interval,vel,gadScrollHoriz,true);
				scrollStarted=true;
			}
		}
	}
	var t = e.originalTarget;
	if (gadStdCursor!=gadDragCursor) {
		if (gadDragDoc) gadSetCursor(gadDragDoc, gadStdCursor,false);
	}
	gadDragDoc=null; gadDownTarget=null;
	if (e.button!=0) e.preventDefault(); // scrollbar bug fixed if we don't preventdefault here. But need it to prevent right click menu. Any negative consequences?
	gadRenderingArea.addEventListener("mousedown", gadDownHandler, true);
	if (gadCIbrowser) gadCIbrowser.addEventListener("mousedown", gadDownHandler, true);
	// handle the image size toggling we preempted earlier
	if (!gadHasMoved && (t.ownerDocument.toggleImageSize)) {
		t.ownerDocument.toggleImageSize(); 
	}  
	//  if we're done scrolling, give focus to the scrolled node
	if (gadHasMoved && !scrollStarted) {
		var focused = document.commandDispatcher.focusedElement;
		if (focused) focused.blur();
		gadScrollObj.clientFrame.focus();
		// focus the div for kb input, unless it will kill a selection.
		if (gadScrollObj.clientFrame.getSelection().isCollapsed) {
			if (gadScrollObj.nodeToScrollX && gadScrollHoriz) gadScrollObj.nodeToScrollX.focus(); 
			else if (gadScrollObj.nodeToScrollY) gadScrollObj.nodeToScrollY.focus();
		}  	
	}
}

function gadFixFocus(n) 
{
	var fr;
	var focused = document.commandDispatcher.focusedElement;
	if (focused) focused.blur();
	if (content) content.focus();
	if (n.ownerDocument.defaultView.frameElement) {
		fr = n.ownerDocument.defaultView.frameElement.contentWindow;
		if (fr) fr.focus();
	}
}

//handle some useful default browser click actions that we prevented with preventDefault 
//in the mousedown handler.
function gadClickHandler(e) 
{
	var t = e.originalTarget;
	// even in grab anywhere mode, we allow clicks to focus text boxes, etc.
	if (gadPreLevel==2 && e.button==gadButton && gadButton==0) {
		var ot = e.explicitOriginalTarget;
		if (ot.nodeName.toLowerCase()=="input" || ot.nodeName.toLowerCase()=="textarea" 
				|| ot.nodeName.toLowerCase()=="select") {
			ot.focus();
		} else if (!gadInQT) {
			gadFixFocus(e.originalTarget);
		 	try { window.goDoCommand("cmd_selectNone"); } catch(err) { }
		}
		return;
	}
	// below here is never called for prelevel 2
	if (gadOnItem(t) || e.button!=gadButton || gadInQT) return; // || gadHasMoved 
	// handle left-click focus/deselection actions if they were prevented before
	if (e.button==0 && gadButton==0) {
	 	gadFixFocus(e.originalTarget);
 		try { window.goDoCommand("cmd_selectNone"); } catch(err) { }
	}
}

function gadButtonAdd(e)
{
	var bu = e.target.firstChild;
	if (bu) if (bu.id=="grabanddrag-button") {
		document.getElementById("grabanddrag-button").setAttribute("gadOn",gadInQT?"QT":gadOn);
	}
}

function gadToggleKeyHandler(e) 
{
	if ((e.keyCode == gadTogKey) && (e.ctrlKey == gadUseCtrl) && 
			(e.altKey == gadUseAlt) && (e.shiftKey == gadUseShift)) {
		gadToggle();
	}
}

function gadSetCursor(doc, cur, deep) 
{
	if (!doc) return;
	if (doc.body) {
		if (doc.designMode) if (doc.designMode=="on") cur="auto"; // don't change cursor in design mode docs
			if (doc.body.style.cursor != cur) {
				doc.body.style.setProperty("cursor", cur, "important");
			}
	}
	if (deep && (doc.evaluate)) {
		var i;		
		var x = doc.evaluate("//frame | //iframe", doc, null, XPathResult.UNORDERED_NODE_SNAPSHOT_TYPE,null);
		if (x) {
			for (i=0; i<x.snapshotLength; i++) {
				gadSetCursor(x.snapshotItem(i).contentDocument,cur,deep);
			}
		}
	}
}

function gadLoad(e) {
	if (!gadInQT) gadSetCursor(e.target, gadStdCursor,false);
}
 
function gadUpdateGraphics(allTabs) {
	var b, bu;

	bu = document.getElementById("grabanddrag-button");
	if (bu) bu.setAttribute("gadOn",gadInQT?"QT":gadOn);
 	
	if (gadAppPlatform==gadFF) {
		// handle the active doc first for faster user feedback
		if ((content)&&(content.document)) gadSetCursor(content.document, ((gadOn&&!gadInQT)?gadStdCursor:"auto"),true);
		if (document.getElementById("cooliris-preview-frame") && document.getElementById("cooliris-preview-frame").contentDocument)
			gadSetCursor(document.getElementById("cooliris-preview-frame").contentDocument,((gadOn&&!gadInQT)?gadStdCursor:"auto"),true);
		if (allTabs) {
			for (var i=0; b = gadContentArea.browsers[i]; i++) 
				if (b.contentDocument) 
					gadSetCursor(b.contentDocument,((gadOn&&!gadInQT)?gadStdCursor:"auto"),true);
		}
	} else { 
		if (gadRenderingArea.contentDocument) 
			gadSetCursor(gadRenderingArea.contentDocument,((gadOn&&!gadInQT)?gadStdCursor:"auto"),true);
	}
}

function gadTextToggle() {
	gadInQT=!gadInQT;
	gadUpdateGraphics(false);
}
function gadToggle() {
	if (gadInQT) gadTextToggle();
	else gadPref.setBoolPref("on", !gadOn);
}

function gadInitScroll(dist,interval,vel,horiz,friction)
{
	if (horiz && !gadScrollObj.nodeToScrollX) return;
	if (!horiz && !gadScrollObj.nodeToScrollY) return;
	gadScrollLastLoop = (new Date()).getTime();
	
	if (gadScrollIntervalObj) window.clearInterval(gadScrollIntervalObj); 
	var destination;
	gadScrollSmooth=gadSmoothStop;
	gadSmoothFactor=2;
	if (horiz) {
		destination = gadScrollObj.scrollLeft + dist;
		if (destination <= 0) { 
			dist = -gadScrollObj.scrollLeft; 
			gadSmoothFactor=3; 
		} else if (destination >= gadScrollObj.scrollW-gadScrollObj.realWidth)  {
			dist = gadScrollObj.scrollW-gadScrollObj.realWidth-gadScrollObj.scrollLeft;
			gadSmoothFactor=3; 
		}
	} else {
		destination = gadScrollObj.scrollTop + dist;
		if (destination <= 0) {
			dist = -gadScrollObj.scrollTop;
			gadSmoothFactor=3; 
		} else if (destination >= gadScrollObj.scrollH-gadScrollObj.realHeight)  {
			dist = gadScrollObj.scrollH-gadScrollObj.realHeight-gadScrollObj.scrollTop;
			gadSmoothFactor=3; 
		}
	}
	gadScrollLoopInterval = interval;
	gadScrollVel = vel;
	gadScrollVelMult = 1;
	gadScrollHoriz = horiz;
	gadScrollToGo = dist;
	gadScrollDistance = dist;
	gadScrollFrictionMult = (friction?1-gadMomFriction:1);
	gadScrollLastMoveLoop = gadScrollLastLoop;
	gadScrollIntervalObj = window.setInterval(gadScrollLoop,gadScrollLoopInterval);
	gadScrollLoop();
}

function gadScrollLoop() 
{
	var toScroll, currentTime, dT, tCk;
	currentTime=(new Date()).getTime();
	// make sure previous loop is done
	if (gadScrollLastLoop==currentTime) {
		window.clearInterval(gadScrollIntervalObj);
		gadScrollIntervalObj = window.setInterval(gadScrollLoop,gadScrollLoopInterval);
		return;
	}
	gadScrollLastLoop = currentTime;
	dT = currentTime-gadScrollLastMoveLoop;

	// the following line sacrifices constant speed for smoother motion when the loop is delayed a lot
	if (currentTime-gadScrollLastLoop>gadScrollMaxInterval) dT=gadScrollMaxInterval;
	
	gadScrollVel *= gadScrollFrictionMult;
	toScroll = Math.round(gadScrollVel*gadScrollVelMult*dT);	
	// with friction, we end the loop as soon as we aren't moving in a time step. Must check this before the next 
	// statement to avoid infinite loop
	if (toScroll==0 && gadScrollFrictionMult<1) { 
		window.clearInterval(gadScrollIntervalObj);
		gadScrollIntervalObj=null;
		return;
	}
	// if there's still distance to go and we're moving slowly enough to not scroll a pixel this time, loop
	if ( (Math.abs(toScroll)<gadDragInc*gadScrollVelMult) &&
				(Math.abs(gadScrollToGo)>=gadDragInc*gadScrollVelMult) ) return;
	 // the following must be before smooth velocity scaling below or loop may not end
	if ((toScroll==0)&&(gadScrollToGo!=0)) return;
	if ((gadScrollToGo-toScroll)*sgn(gadScrollToGo) <= 0) {
		toScroll=gadScrollToGo;
	}	
	// fast (tho somewhat inaccurate) hack to smoothly stop
	tCk = Math.max(30,dT)*gadSmoothFactor*gadScrollVel;
	while (gadScrollSmooth && ((gadScrollToGo-tCk*gadScrollVelMult)*sgn(gadScrollToGo) < 0)) {
		// it's important that we take ceil below so the last pixel always finishes
		toScroll=sgn(toScroll)*Math.ceil(Math.abs(toScroll)/2);  
		gadScrollVelMult*=0.3;
	}
	gadScrollLastMoveLoop = gadScrollLastLoop;

	if (gadScrollHoriz) gadScrollObj.scrollXBy(toScroll);
	else gadScrollObj.scrollYBy(toScroll);
	gadScrollToGo -= toScroll;

	if (gadScrollToGo==0){
		window.clearInterval(gadScrollIntervalObj);
		gadScrollIntervalObj=null;
		/*// focus on scrolled element
		var focused = document.commandDispatcher.focusedElement;
		if (focused) focused.blur();
		gadScrollObj.clientFrame.focus();
		// focus the div for kb input, unless it will kill a selection.
		if (gadScrollObj.clientFrame.getSelection().isCollapsed) {
			if (!gadScrollHoriz) gadScrollObj.nodeToScrollY.focus(); 
			else gadScrollObj.nodeToScrollX.focus();
		}  	*/
		return;
	}
}

// The following is modified from Marc Boullet's All-in-one Gestures extension
function gadFindNodeToScroll(initialNode) 
{
	
	function getStyle(elem, aProp) {
		var p = elem.ownerDocument.defaultView.getComputedStyle(elem, "").getPropertyValue(aProp);
		var val = parseFloat(p);
		if (!isNaN(val)) return Math.ceil(val);
		if (p == "thin") return 1;
		if (p == "medium") return 3;
		if (p == "thick") return 5;
		return 0;
	}

	function scrollCursorType(neededW, availW, neededH, availH, scrollBarSize) {
		if (neededW <= availW && neededH <= availH) return 3;
		if (neededW > availW && neededH > availH) return 0;
		if (neededW > availW) return ((neededH <= (availH - scrollBarSize)) - 0) << 1;  // 0 or 2
		return (neededW <= (availW - scrollBarSize)) - 0;
	}
  
	const defaultScrollBarSize = 16;
	const twiceScrollBarSize = defaultScrollBarSize * 2;
	var retObj = {isXML: false, nodeToScrollX: null, nodeToScrollY: null,
								ratioX: 1, ratioY: 1, clientFrame: null, isFrame: false,
								targetDoc: null, insertionNode: null, realHeight: 1, realWidth: 1, 
								scrollXBy: null, scrollYBy: null, scrollW: 1, scrollH: 1 };
	var realWidth, realHeight, nextNode, currNode, scrollType;
	var targetDoc = initialNode.ownerDocument;
	var docEl = targetDoc.documentElement;
	retObj.insertionNode = (docEl) ? docEl : targetDoc;
	var docBox = targetDoc.getBoxObjectFor(retObj.insertionNode);
	var clientFrame = targetDoc.defaultView;
	retObj.targetDoc = targetDoc; retObj.clientFrame = clientFrame;
	if (docEl && docEl.nodeName.toLowerCase() == "html") { // walk the tree up looking for something to scroll
		if (clientFrame.frameElement) retObj.isFrame = true; else retObj.isFrame = false;
		var bodies = docEl.getElementsByTagName("body");
		if (!bodies || !bodies.length) 
			return retObj;
		var bodyEl = bodies[0];
		if (initialNode == docEl) nextNode = bodyEl;
		else if (initialNode.nodeName.toLowerCase() == "select") nextNode = initialNode.parentNode;
		else nextNode = initialNode;
		do {
			try {
				currNode = nextNode;
				// note: we ignore DIVs etc with "visible" overflow but allow it for body and html elements, 
				// which get scrollbars automatically in frames or from the browser.
				if (currNode.clientWidth && currNode.clientHeight &&  
						(currNode.ownerDocument.defaultView.getComputedStyle(currNode, "").getPropertyValue("overflow") != "hidden") &&
						(currNode.nodeName.toLowerCase()=="html" || currNode.nodeName.toLowerCase()=="body" || currNode.ownerDocument.defaultView.getComputedStyle(currNode, "").getPropertyValue("overflow") != "visible")) {
					realWidth = currNode.clientWidth + getStyle(currNode, "border-left-width") + getStyle(currNode, "border-right-width");
					realHeight = currNode.clientHeight + getStyle(currNode, "border-top-width") + getStyle(currNode, "border-bottom-width");
					scrollType = scrollCursorType(currNode.scrollWidth, realWidth, currNode.scrollHeight, realHeight, 0);
					if (scrollType != 3) {
						if ((scrollType==0 || scrollType==2)&&(!retObj.nodeToScrollX)) {
							retObj.nodeToScrollX = currNode;
							if (realWidth > twiceScrollBarSize) realWidth -= twiceScrollBarSize;
							if (gadSBMode) retObj.ratioX = -currNode.scrollWidth/realWidth; 
							else retObj.ratioX = gadMult;
							retObj.scrollW = currNode.scrollWidth-twiceScrollBarSize; 
							retObj.realWidth = realWidth;
							retObj.scrollXBy = function(dx) { this.nodeToScrollX.scrollLeft += dx; }
							retObj.__defineGetter__("scrollLeft",function(){return this.nodeToScrollX.scrollLeft;});
						}
						if ((scrollType==0 || scrollType==1)&&(!retObj.nodeToScrollY)) {
							retObj.nodeToScrollY = currNode;
							if (realHeight > twiceScrollBarSize) realHeight -= twiceScrollBarSize;
							if (gadSBMode) retObj.ratioY = -currNode.scrollHeight/realHeight;
							else retObj.ratioY = gadMult;
							retObj.scrollH = currNode.scrollHeight-twiceScrollBarSize;
							retObj.realHeight = realHeight;
							retObj.scrollYBy = function(dy) { this.nodeToScrollY.scrollTop += dy; }
							retObj.__defineGetter__("scrollTop",function(){return this.nodeToScrollY.scrollTop;});
						}
						if (retObj.nodeToScrollX && retObj.nodeToScrollY)	
							return retObj;
					}
				}
				nextNode = currNode.parentNode;
			}
			catch(err) {
				return retObj;
			}
		} while (nextNode && currNode != docEl);
		if (retObj.isFrame) {
			if (retObj.nodeToScrollX || retObj.nodeToScrollY) 
				return retObj;
			else 
				return gadFindNodeToScroll(clientFrame.frameElement.ownerDocument.documentElement);
		}
	}
	else { // XML document; do our best
		retObj.nodeToScrollX = initialNode;
		retObj.nodeToScrollY = initialNode;
		if (docBox) {
			if (gadSBMode) {
				retObj.ratioX = -docBox.width/gadRenderingArea.boxObject.width;
				retObj.ratioY = -docBox.height/gadRenderingArea.boxObject.height;
			} else {
				retObj.ratioX = gadMult;
				retObj.ratioY = gadMult;
			}	            	
			retObj.scrollW = docBox.width;
			retObj.scrollH = docBox.height;
			retObj.realWidth = gadRenderingArea.boxObject.width;
			retObj.realHeight = gadRenderingArea.boxObject.height;
			try { scrollType = scrollCursorType(docBox.width, gadRenderingArea.boxObject.width,
								docBox.height, gadRenderingArea.boxObject.height, defaultScrollBarSize); } catch(err) { }
			if (scrollType==0 || scrollType==2)	retObj.nodeToScrollX = initialNode;
			if (scrollType==0 || scrollType==1)	retObj.nodeToScrollY = initialNode;
		}
		retObj.isXML = true;
		retObj.scrollXBy = function(dx) { this.clientFrame.scrollBy(dx,0); }
		retObj.scrollYBy = function(dy) { this.clientFrame.scrollBy(0,dy); }
		retObj.__defineGetter__("scrollTop",function(){return this.clientFrame.scrollY;});
		retObj.__defineGetter__("scrollLeft",function(){return this.clientFrame.scrollX;});
	}
	return retObj;
}
// End AiOG
 

function go() 
{
	var gadPrefRoot = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefService).getBranch(null);
	var gadPref = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefService).getBranch("grabAndDrag.");
		
	// add tb toggle
	var win = mostRecentWindow("navigator:browser");
	if (!win) {
		gadPref.setBoolPref("reverse",true);
		win = mostRecentWindow("mail:3pane");
		if (!win) return;
		// its ok to use different cursors in TB always
		gadPref.setBoolPref("samecur",false);
	}
	var bar = win.document.getElementById("nav-bar");
	if (!bar) bar = win.document.getElementById("mail-bar");
	if (!bar) return;
        if (bar.currentSet.indexOf("grabanddrag-button")==-1) {
		bar.insertItem("grabanddrag-button", null, null, false);
		win.document.getElementById("grabanddrag-button").setAttribute("gadOn",gadPref.getBoolPref("on"));
		bar.setAttribute("currentset",bar.currentSet); // needed to make persist work for some reason
		win.document.persist(bar.id, 'currentset');
	}	
}

function mostRecentWindow(winType) {
	var WindowMediator = Components.classes['@mozilla.org/appshell/window-mediator;1'].
				getService(Components.interfaces.nsIWindowMediator);
	return WindowMediator.getMostRecentWindow(winType);
}
	
