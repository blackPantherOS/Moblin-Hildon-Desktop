window.addEventListener("load", testOnLoad, false);

function testOnLoad() {
  const Cc = Components.classes;
  const Ci = Components.interfaces;

  // Make sure to launch the test harness for the first opened window only
  var prefs = Cc["@mozilla.org/preferences-service;1"].
              getService(Ci.nsIPrefBranch);
  if (prefs.prefHasUserValue("testing.browserTestHarness.running"))
    return;

  prefs.setBoolPref("testing.browserTestHarness.running", true);

  var ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
           getService(Ci.nsIWindowWatcher);
  var sstring = Cc["@mozilla.org/supports-string;1"].
                createInstance(Ci.nsISupportsString);
  sstring.data = location.search;
  ww.openWindow(window, "chrome://mochikit/content/browser-harness.xul", "browserTest",
                "chrome,centerscreen,dialog,resizable,titlebar,toolbar=no,width=800,height=600", sstring);
}

function Tester(aTests, aCallback) {
  this.tests = aTests;
  this.callback = aCallback;
}
Tester.prototype = {
  checker: null,
  currentTestIndex: -1,
  get currentTest() {
    return this.tests[this.currentTestIndex];
  },
  get done() {
    return this.currentTestIndex == this.tests.length - 1;
  },
  step: function Tester_step() {
    this.currentTestIndex++;
  },
  
  start: function Tester_start() {
    this.execTest();
  },

  finish: function Tester_finish() {
    // Tests complete, notify the callback and return
    this.callback(this.tests);
    this.callback = null;
    this.tests = null;
  },

  execTest: function Tester_execTest() {
    if (this.done) {
      this.finish();
      return;
    }

    // Move to the next test (or first test).
    this.step();

    // Load the tests into a testscope
    this.currentTest.scope = new testScope(this.currentTest.tests);

    var scriptLoader = Cc["@mozilla.org/moz/jssubscript-loader;1"].
                       getService(Ci.mozIJSSubScriptLoader);
    try {
      scriptLoader.loadSubScript(this.currentTest.path, this.currentTest.scope);

      // Run the test
      this.currentTest.scope.test();
    } catch (ex) {
      this.currentTest.tests.push(new testResult(false, "Exception thrown", ex, false));
      this.currentTest.scope.done = true;
    }

    // If the test ran synchronously, move to the next test,
    // otherwise start a poller to monitor it's progress.
    if (this.currentTest.scope.done) {
      this.execTest();
    } else {
      var self = this;
      this.checker = new resultPoller(this.currentTest, function () { self.execTest(); });
      this.checker.start();
    }
  }
};

function testResult(aCondition, aName, aDiag, aIsTodo) {
  aName = aName || "";

  this.pass = !!aCondition;
  this.todo = aIsTodo;
  if (this.pass) {
    if (aIsTodo)
      this.msg = "\tTODO PASS - " + aName;
    else
      this.msg = "\tPASS - " + aName;
  } else {
    this.msg = "\tFAIL - ";
    if (aIsTodo)
      this.msg += "TODO Worked? - ";
    this.msg += aName;
    if (aDiag)
      this.msg += " - " + aDiag;
  }
}

function testScope(aTests) {
  var scriptLoader = Cc["@mozilla.org/moz/jssubscript-loader;1"].
                     getService(Ci.mozIJSSubScriptLoader);
  scriptLoader.loadSubScript("chrome://mochikit/content/tests/SimpleTest/EventUtils.js", this.EventUtils);

  this.tests = aTests;
}
testScope.prototype = {
  ok: function test_ok(condition, name, diag) {
    this.tests.push(new testResult(condition, name, diag, false));
  },
  
  is: function test_is(a, b, name) {
    this.ok(a == b, name, "Got " + a + ", expected " + b);
  },
  
  isnot: function test_isnot(a, b, name) {
    this.ok(a != b, name, "Didn't expect " + a + ", but got it");
  },

  todo: function test_todo(condition, name, diag) {
    this.tests.push(new testResult(!condition, name, diag, true));
  },

  done: true,
  waitForExplicitFinish: function test_WFEF() {
    this.done = false;
  },
  finish: function test_finish() {
    this.done = true;
  },

  EventUtils: {}
};

// Check whether the test has completed every 3 seconds
const CHECK_INTERVAL = 3000;
// Test timeout (seconds)
const TIMEOUT_SECONDS = 15;

const MAX_LOOP_COUNT = (TIMEOUT_SECONDS * 1000) / CHECK_INTERVAL;

function resultPoller(aTest, aCallback) {
  this.test = aTest;
  this.callback = aCallback;
}
resultPoller.prototype = {
  loopCount: 0,
  interval: 0,

  start: function resultPoller_start() {
    var self = this;
    function checkDone() {
      self.loopCount++;
  
      if (self.loopCount > MAX_LOOP_COUNT) {
        self.test.tests.push(new testResult(false, "Timed out", "", false));
        self.test.scope.done = true;
      }

      if (self.test.scope.done) {
        clearInterval(self.interval);

        // Notify the callback
        self.callback();
        self.callback = null; 
        self.test = null;
      }
    }
    this.interval = setInterval(checkDone, CHECK_INTERVAL);
  }
};
