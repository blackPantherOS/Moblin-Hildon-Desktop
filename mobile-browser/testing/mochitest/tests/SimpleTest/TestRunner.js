/**
 * TestRunner: A test runner for SimpleTest
 * TODO:
 * 
 *  * Avoid moving iframes: That causes reloads on mozilla and opera.
 *
 *
**/
var TestRunner = {};
TestRunner.logEnabled = false;
TestRunner._currentTest = 0;
TestRunner.currentTestURL = "";
TestRunner._urls = [];

/**
 * Make sure the tests don't hang. Runs every 300 seconds, but it will
 * take up to 360 seconds to detect a hang.
**/
TestRunner._testCheckPoint = -1;
TestRunner._checkForHangs = function() {
  if (TestRunner._currentTest < TestRunner._urls.length) {
    if (TestRunner._testCheckPoint == TestRunner._currentTest) {
      var frameWindow = $('testframe').contentWindow.wrappedJSObject ||
                       	$('testframe').contentWindow;
      frameWindow.SimpleTest.ok(false, "Test timed out.");
      frameWindow.SimpleTest.finish();
    }
    TestRunner._testCheckPoint = TestRunner._currentTest;
    TestRunner.deferred = callLater(300, TestRunner._checkForHangs); 
  }
}

/**
 * This function is called after generating the summary.
**/
TestRunner.onComplete = null;

/**
 * If logEnabled is true, this is the logger that will be used.
**/
TestRunner.logger = MochiKit.Logging.logger;

/**
 * Toggle element visibility
**/
TestRunner._toggle = function(el) {
    if (el.className == "noshow") {
        el.className = "";
        el.style.cssText = "";
    } else {
        el.className = "noshow";
        el.style.cssText = "width:0px; height:0px; border:0px;";
    }
};


/**
 * Creates the iframe that contains a test
**/
TestRunner._makeIframe = function (url) {
    var iframe = $('testframe');
    iframe.src = url;
    iframe.name = url; 
    iframe.width = "500";
    return iframe;
};

/**
 * TestRunner entry point.
 *
 * The arguments are the URLs of the test to be ran.
 *
**/
TestRunner.runTests = function (/*url...*/) {
    if (TestRunner.logEnabled)
        TestRunner.logger.log("SimpleTest START");
  
    TestRunner._urls = flattenArguments(arguments);
    $('testframe').src="";
    TestRunner._checkForHangs();
    TestRunner.runNextTest();
};

/**
 * Run the next test. If no test remains, calls makeSummary
**/
TestRunner.runNextTest = function() {
    if (TestRunner._currentTest < TestRunner._urls.length) {
        var url = TestRunner._urls[TestRunner._currentTest];
        TestRunner.currentTestURL = url;

        $("current-test-path").innerHTML = url;
        
        if (TestRunner.logEnabled)
            TestRunner.logger.log("Running " + url + "...");
        
        TestRunner._makeIframe(url);
    }  else {
        $("current-test").innerHTML = "<b>Finished</b>";
        TestRunner._makeIframe("about:blank");
        if (TestRunner.logEnabled) {
            TestRunner.logger.log("SimpleTest FINISHED");
	    TestRunner.logger.log("Passed: " + $("pass-count").innerHTML);
 	    TestRunner.logger.log("Failed: " + $("fail-count").innerHTML);
	    TestRunner.logger.log("Todo:   " + $("todo-count").innerHTML);
	}
        if (TestRunner.onComplete)
            TestRunner.onComplete();
    }
};

/**
 * This stub is called by SimpleTest when a test is finished.
**/
TestRunner.testFinished = function(doc) {
    var finishedURL = TestRunner._urls[TestRunner._currentTest];
    
    if (TestRunner.logEnabled)
        TestRunner.logger.debug("SimpleTest finished " + finishedURL);
    
    TestRunner.updateUI();
    TestRunner._currentTest++;
    TestRunner.runNextTest();
};

/**
 * Get the results.
 */
TestRunner.countResults = function(doc) {
  var nOK = withDocument(doc,
     partial(getElementsByTagAndClassName, 'div', 'test_ok')
  ).length;
  var nNotOK = withDocument(doc,
     partial(getElementsByTagAndClassName, 'div', 'test_not_ok')
  ).length;
  var nTodo = withDocument(doc,
     partial(getElementsByTagAndClassName, 'div', 'test_todo')
  ).length;
  return {"OK": nOK, "notOK": nNotOK, "todo": nTodo};
}

TestRunner.updateUI = function() {
  var results = TestRunner.countResults($('testframe').contentDocument);
  var passCount = parseInt($("pass-count").innerHTML) + results.OK;
  var failCount = parseInt($("fail-count").innerHTML) + results.notOK;
  var todoCount = parseInt($("todo-count").innerHTML) + results.todo;
  $("pass-count").innerHTML = passCount;
  $("fail-count").innerHTML = failCount;
  $("todo-count").innerHTML = todoCount;
  
  // Set the top Green/Red bar
  var indicator = $("indicator");
  if (failCount > 0) {
    indicator.innerHTML = "Status: Fail";
    indicator.style.backgroundColor = "red";
  } else if (passCount > 0) {
    indicator.innerHTML = "Status: Pass";
    indicator.style.backgroundColor = "green";
  }
  
  // Set the table values
  var trID = "tr-" + $('current-test-path').innerHTML;
  var row = $(trID);
  replaceChildNodes(row,
    TD({'style':
        {'backgroundColor': results.notOK > 0 ? "#f00":"#0d0"}}, results.OK),
    TD({'style':
        {'backgroundColor': results.notOK > 0 ? "#f00":"#0d0"}}, results.notOK),
    TD({'style': {'backgroundColor':
                   results.todo > 0 ? "orange":"#0d0"}}, results.todo)
  );
}
