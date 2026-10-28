Sisyphus
========

A game about keeping software alive. The boulder is your software: push it
up the hill, and every terrace ships a release. Every release adds a
module, every module rots, and at the summit it all rolls back down.

Play
----

Windows:  double-click sisyphus.exe. Windows may say it protected your PC,
          because the game is not signed: choose "More info", then
          "Run anyway".
Linux:    ./sisyphus   (needs OpenGL 4.5, which any current Mesa or vendor
          driver has, and glibc 2.34 or newer: Ubuntu 22.04, Debian 12,
          Fedora 35 and later)

Click in the window to look around with the mouse. W A S D walk: walk into
the boulder from below to push it. Hold E on a module to patch it, hold R
to retire it. Q sits, V switches between his eyes and over his shoulder,
M turns the sound off or on, F1 shows the controls. Esc frees the mouse,
and Esc again quits.

The game saves on its own every minute, on F5 and when you quit, and
continues where you left off (F9 loads the last save):

  Windows:  %APPDATA%\Sisyphus\default
  Linux:    ~/.local/share/sisyphus/default  (or $XDG_DATA_HOME/sisyphus/default)

  sisyphus --new          start over (the old save is kept as the previous one)
  sisyphus --slot DIR     keep the save somewhere else
  sisyphus --autopilot --speed 4
                          watch the autopilot play (it never saves)

More: https://github.com/Preponderous-Software/sisyphus
Play in a browser: https://sisyphus.play.danielstephenson.dev

Sisyphus is MIT-licensed (LICENSE.txt). It is built on the Strata engine;
THIRD-PARTY.md lists what that links in, and their licences.
