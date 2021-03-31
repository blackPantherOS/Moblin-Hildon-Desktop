// Javascript for flash pages


// getFlashFile detects if flash libraries are present
function getFlashFile() {
	useFlash = true;
	var haveFlash = false;
	var flashVer = 0;
	var hideMarquee = getQueryStringValue("hideMarquee");
	var customMovieName = getQueryStringValue("movieName");

	if (navigator.plugins && navigator.plugins.length) {
		var x = navigator.plugins["Shockwave Flash"];
		if (x) {
			haveFlash = true;
		}
	} else if (navigator.mimeTypes && navigator.mimeTypes.length) {
		x = navigator.mimeTypes['application/x-shockwave-flash'];
		if (x && x.enabledPlugin) {
			haveFlash = true;
		}
	}
	// if custom movie name passed in as query string
	// then use it
	var flashFile = "flash_home_800x480.swf";
	if (customMovieName != "") {
		flashFile = customMovieName;
	} else {
		if (screen.width==1024 && screen.height==600) {
			flashFile = "flash_home_1024x600.swf";	
		} else if (screen.width==800 && screen.height==600) {
			flashFile = "flash_home_800x600.swf";
		} else if (screen.width==800 && screen.height==480) {
			flashFile = "flash_home_800x480.swf";
		}
	var flashDiv = document.getElementById ('flashDiv');
	if (flashDiv) {
		// don't substract the marquee size if we are hiding it
		if (hideMarquee == "true") {
			flashDiv.innerHTML = "<embed name='themovie'  src='" + flashFile + "' width='" + screen.width + "px' height='" + screen.height + "px' />";
		} else {
			flashDiv.innerHTML = "<embed name='themovie'  src='" + flashFile + "' width='" + screen.width + "px' height='" + (screen.height - 52) + "px' />";
		}
		flashDiv.style.visibility="visible";
	}
}



