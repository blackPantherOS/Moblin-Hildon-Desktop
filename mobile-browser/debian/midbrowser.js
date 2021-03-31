// This is the Debian specific preferences file for Midbrowser
// You can make any change in here, it is the purpose of this file.
// You can, with this file and all files present in the
// /etc/midbrowser/pref directory, override any preference that is
// present in /usr/lib/midbrowser/defaults/pref directory.
// While your changes will be kept on upgrade if you modify files in
// /etc/midbrowser/pref, please note that they won't be kept if you
// do them in /usr/lib/midbrowser/defaults/pref.

pref("extensions.update.enabled", true);

// Use LANG environment variable to choose locale
pref("intl.locale.matchOS", true);

// Disable default browser checking.
pref("browser.shell.checkDefaultBrowser", false);
