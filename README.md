# Fallblattcode

ESP32-Code für die Fallblattanzeigen der Epoche 2 und 3:

* https://github.com/julianschick/fallblatt-schematics-ep2
* https://github.com/julianschick/fallblatt-schematics-ep3

Die älteren Anzeigen der Epoche 2 sind mit Hall-Sensoren versehen, während die neueren der Epoche 3 mit Infrarotlichtsensorik arbeiten. Das Prinzip ist in beiden Fällen das Gleiche: Es gibt einen Sensor, der bei jedem herabfallenden Blatt einmal triggert, und einen, der eine absolute Nullstellung der gesamten Palette markiert. Mit den Implementationen `PollingSplitflap` für Epoche 3 und `HallSplitflap` für Epoche 2 stehen austauschbare C++-Klassen für beide Arten von Anzeigen zur Verfügung.

Das Projekt ist mittlerweile PlatformIO-basiert. Von außen werden die Fallblattmodule per HTTP gesteuert. Der Mikrocontroller verbindet sich mit einem WLAN und startet dann einen einfachen HTTP-Server.
