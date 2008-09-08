/*
	Filename:	GermanStrings.h
	Contents:	Definitions of UI strings in German.
	Author:		Rainer Riedl (riedl@navios.de)
	Remark:		v0.1, not really ready now, just a first try.
				v0.2, next step. Not ready, but big improvement
				v0.3, next step. We get closer :-)
				v0.4, added just the new ones and did some minor changes
				v0.5, SMALL changes
					- Right after starting there's "Un/Redo" in the menus instead of "Un/Redo impossible"
					- The Window-Zoom button shoul work like the Tracker windows: switch between actual size and "fit window" instead of making it whole screen
					- After using "Save image as" in a project it should still have the projects name instead of the name of the image
					- After opening an image that was saved with "Save image as" it's window has/had a strange size (something like 30x400)
					- The zoom display in the paint window is wrong right after loading a project
					- Being able to add some comments to the project would be very nice (eg. I'd like to write down somewhere the fontname/size I had used for my weg graphics)
					- How about if layers could be named?
					- Duplicating a Layer should put it just in front of that duplicated layer instead of in front of the whole canvas
					- there should be added some more standard sizes (How about if there would be a prefs panel where one could enter/edit his prefered sizes?)
					- it wouldn't really necessary to put the "[PIX] [x] Visible" into every entry entry of the layer window
					  How about just using it that way "[x] [PIX]" without a text?
					  Or the space there could be used a bit better. Is it true that every layer has it's own background color?! How about displaying it there?
					- there is no warning when quitting a changed but unsaved project
					- There's still a nontranslated "Save" in the file panel of "Save Image As"
					- no translation for "Merge Layers" in menu Undo ...
					- no translation for "Clear Layer" in menu Undo ...
					- Scrollbars: clicking in the area between knob and arrows should move the window content one page (the visible area)
					- entering numbers in textcontrols should immediatly make the necessary actions (eg in the move dialog you have to put the focus to another
					  field before the input is recognized. That's even more bad as input is ignored when pressing the green checkmark)
					- pressing the background color icon should set it as current color
					- The frame that marks the cropped area should have another color (at least when it is outside)
					  You don't see it on the grey background that is outside a canvas
				v0.6 Some small changes
					- an idea for the future: Using colors for painting is real nice. Using Textures would be the hit! :-)
					- Would be nice if there is a string in the language prefs saying that changes take place after restarting the app
					- there's still a nontranslated Cancel/Ok in the text input window
					- XY-display shows last value when mouse exits paint-view. Perhaps it should display canvas width/height instead (or nothing)
					- window title displays "[Untitled - 1(*)][ - (*)]" -- seems like there is anything wrong :-)
					- the undo depth isn't remembered - it's adjustable everytime after restart
				v0.7 Added the registering strings
*/


#ifndef GERMAN_STRINGS_H
#define	GERMAN_STRINGS_H

#define	GERMAN_ACTIVE_LAYER_STRING						"Aktive Ebene"
#define	GERMAN_ADD_AREA_STRING							"Bereich hinzufügen"
#define	GERMAN_ADD_LAYER_HELP_STRING					"Fügt dem Bild eine Ebene hinzu."
#define	GERMAN_ADD_LAYER_STRING							"Ebene hinzufügen"
#define	GERMAN_ADD_LAYER_BEHIND_STRING					"Ebene dahinter hinzufügen"
#define GERMAN_ADD_LAYER_IN_FRONT_STRING				"Ebene davor hinzufügen"
#define	GERMAN_ADD_ONS_STRING							"Module"
#define	GERMAN_ADJUSTABLE_STRING						"Einstellbar"
#define	GERMAN_ADJUSTABLE_WIDTH_STRING					"Breite einstellbar"
#define	GERMAN_AIRBRUSH_STRING							"Airbrush"
#define	GERMAN_ALL_LAYERS_COPY_HELP_STRING				"Kopiert das Bild in die Zwischenablage."
#define	GERMAN_ALL_LAYERS_CUT_HELP_STRING				"Kopiert das Bild in die Zwischenablage und löscht alle Ebenen."
#define	GERMAN_ALL_LAYERS_STRING						"Alle Ebenen"
#define	GERMAN_BACKGROUND_STRING						"Hintergrund"
#define	GERMAN_BOTTOM_STRING							"unten"
#define	GERMAN_BRUSHES_STRING							"Stiftauswahl"
#define GERMAN_CANNOT_CREATE_IMAGE_STRING				"Der Speicher reicht nicht aus um dieses Bild zu erstellen. Bitte versuchen sie es mit einer kleineren Bildgröße!"
#define	GERMAN_CANVAS_STRING							"Bild"
#define	GERMAN_CENTER_TO_CORNER_STRING					"Mittelpunkt-Ecke"
#define	GERMAN_CHANGE_TRANSPARENCY_HELP_STRING			"Ändert die Transparenz der aktiven Ebene."
#define	GERMAN_CHANGE_TRANSPARENCY_STRING				"Transparenz ändern…"
#define	GERMAN_CLEAR_CANVAS_HELP_STRING					"Löscht alle Ebenen."
#define	GERMAN_CLEAR_CANVAS_STRING						"Bild löschen"
#define	GERMAN_CLEAR_LAYER_HELP_STRING					"Löscht die aktive Ebene."
#define	GERMAN_CLEAR_LAYER_STRING						"Ebene löschen"
#define	GERMAN_CLEAR_SELECTION_HELP_STRING				"Hebt die Markierung auf. Anschliessend gilt wieder alles als markiert."
#define	GERMAN_CLEAR_SELECTION_STRING					"Markierung aufheben"
#define	GERMAN_CLOSE_HELP_STRING						"Schließt dieses Fenster."
#define	GERMAN_CLOSE_STRING								"Fenster schließen"
#define	GERMAN_COLOR_AMOUNT_STRING						"Anzahl der Farben"
#define	GERMAN_COLOR_STRING								"Farbe"
#define	GERMAN_COLOR_VARIANCE_STRING					"Farbwechsel"
#define	GERMAN_COLORS_STRING							"Farben"
#define	GERMAN_CONTINUOUS_STRING						"Kontinuierlich"
#define	GERMAN_COPY_STRING								"Kopieren"
#define	GERMAN_CORNER_TO_CORNER_STRING					"Ecke-Ecke"
#define	GERMAN_CREATE_CANVAS_STRING						"Bild erstellen"
#define	GERMAN_CROP_HELP_STRING							"Hinzufügen und Abschneiden der Ränder."
#define	GERMAN_CROP_STRING								"Ränder ändern…"
#define	GERMAN_CUT_STRING								"Ausschneiden"
#define	GERMAN_DELETE_CURRENT_SET_STRING				"Angezeigte Palette löschen"
#define	GERMAN_DELETE_LAYER_STRING						"Ebene entfernen"
#define	GERMAN_DELETE_SELECTED_BRUSH_STRING				"Ausgewählten Stift löschen"
#define	GERMAN_APPLY_CHANGES_STRING						"Übernehmen"
#define	GERMAN_DISCARD_CHANGES_STRING					"Verwerfen"
#define	GERMAN_DUPLICATE_LAYER_STRING					"Ebene duplizieren"
#define GERMAN_EDIT_STRING								"Bearbeiten"
#define	GERMAN_ELLIPSE_STRING							"Ellipse"
#define	GERMAN_ENABLE_ANTI_ALIASING_STRING				"Anti-Aliasing"
#define	GERMAN_ENABLE_GRADIENT_STRING					"Farbverlauf"
#define	GERMAN_ENABLE_PREVIEW_STRING					"Vorschau"
#define	GERMAN_ENABLE_ROTATION_STRING					"Drehbar"
#define	GERMAN_FADE_STRING								"Intensität"
#define	GERMAN_FILE_STRING								"Datei"
#define	GERMAN_FILL_ELLIPSE_STRING						"Ellipse füllen"
#define	GERMAN_FILL_RECTANGLE_STRING					"Rechteck füllen"
#define	GERMAN_FINISHING_STRING							"In Arbeit"
#define	GERMAN_FLIP_HORIZONTAL_LAYER_HELP_STRING		"Spiegelt die aktive Ebene horizontal."
#define	GERMAN_FLIP_HORIZONTAL_ALL_LAYERS_HELP_STRING	"Spiegelt alle Ebenen horizontal."
#define	GERMAN_FLIP_HORIZONTAL_STRING					"Horizontal spiegeln"
#define	GERMAN_FLIP_VERTICAL_LAYER_HELP_STRING			"Spiegelt die aktive Ebene vertikal."
#define	GERMAN_FLIP_VERTICAL_ALL_LAYERS_HELP_STRING		"Spiegelt alle Ebenen vertikal."
#define	GERMAN_FLIP_VERTICAL_STRING						"Vertikal spiegeln"
#define	GERMAN_FLOOD_FILL_STRING						"Begrenzt füllen"
#define	GERMAN_FLOW_STRING								"Intensität"
#define	GERMAN_FONT_STRING								"Schriftart"
#define	GERMAN_FREE_LINE_STRING							"Freihandlinie"
#define	GERMAN_GLOBAL_SETTINGS_STRING					"Globale Einstellungen"
#define	GERMAN_GRID_OFF_HELP_STRING						"Schaltet das Raster aus."
#define GERMAN_GRID_2_BY_2_HELP_STRING					"Setzt das Raster auf 2 x 2 Pixel."
#define GERMAN_GRID_4_BY_4_HELP_STRING					"Setzt das Raster auf 4 x 4 Pixel."
#define GERMAN_GRID_8_BY_8_HELP_STRING					"Setzt das Raster auf 8 x 8 Pixel."
#define	GERMAN_GROW_SELECTION_HELP_STRING				"Vergrößert die Markierung in jede Richtung ein bißchen."
#define	GERMAN_GROW_SELECTION_STRING					"Markierung vergrößern"
#define	GERMAN_HAIRS_STRING								"Haare"
#define	GERMAN_HEIGHT_STRING							"Höhe"
#define	GERMAN_HELP_STRING								"Hilfe"
#define	GERMAN_INSERT_TEXT_HELP_STRING					"Fügt Text in die aktive Ebene ein. Hat dieselbe Funktion wie das Text-Werkzeug."
#define	GERMAN_INSERT_TEXT_STRING						"Text einfügen…"
#define	GERMAN_INTELLIGENT_SCISSORS_STRING				"Intelligente Schere"
#define	GERMAN_INVERT_SELECTION_HELP_STRING				"Invertiert die Markierung so daß markiertes nicht-markiert wird und umgekehrt."
#define	GERMAN_INVERT_SELECTION_STRING					"Markierung invertieren"
#define GERMAN_STANDARD_SIZES_STRING					"Standardgrößen"
#define GERMAN_KEEP_IN_FRONT_STRING						"Fenster immer im Vordergrund"
#define GERMAN_BRUSH_WINDOW_STRING						"Stiftauswahl"
#define GERMAN_COLOR_WINDOW_STRING						"Farbauswahl"
#define GERMAN_EFFECTS_WINDOW_STRING					"Effektfenster"
#define GERMAN_LAYER_WINDOW_STRING						"Ebenenauswahl"
#define GERMAN_TOOL_SELECTION_WINDOW_STRING				"Werkzeugauswahl"
#define GERMAN_TOOL_SETUP_WINDOW_STRING					"Werkzeugeinstellungen"
#define	GERMAN_LANGUAGE_STRING							"Sprache"
#define	GERMAN_LAYER_COPY_HELP_STRING					"Aktive Ebene in die Zwischenablage kopieren."
#define	GERMAN_LAYER_CUT_HELP_STRING					"Aktive Ebene auschneiden und in die Zwischenablage legen."
#define	GERMAN_LAYER_STRING								"Ebene"
#define	GERMAN_LAYERS_STRING							"Ebenen"
#define	GERMAN_LEFT_STRING								"links"
#define	GERMAN_LITTLE_STRING							"wenig"
#define	GERMAN_OPEN_COLOR_SET_STRING					"Laden…"
#define	GERMAN_LOCK_PROPORTIONS_STRING					"Proportionen beibehalten"
#define	GERMAN_MAG_STRING								"Zoom"
#define	GERMAN_MAGIC_WAND_STRING						"Zauberstab"
#define	GERMAN_MERGE_WITH_BACK_LAYER_STRING				"Mit hinterer Ebene vereinigen"
#define	GERMAN_MERGE_WITH_FRONT_LAYER_STRING			"Mit vorderer Ebene vereinigen"
#define	GERMAN_MODE_STRING								"Modus"
#define	GERMAN_MUCH_STRING								"viel"
#define	GERMAN_NEW_COLOR_SET_STRING						"Neu anlegen"
#define	GERMAN_NEW_PROJECT_HELP_STRING					"Erstellt zusätzlich ein neues, leeres Projekt-Fenster."
#define	GERMAN_NEW_PROJECT_STRING						"Neues Projekt"
#define	GERMAN_NO_OPTIONS_STRING						"Keine Optionen"
#define	GERMAN_NONE_STRING								"keine"
#define	GERMAN_OFF_STRING								"aus"
#define	GERMAN_OK_STRING								"OK"
#define GERMAN_CANCEL_STRING							"Abbruch"
#define	GERMAN_OPEN_STRING								"Öffnen"
#define	GERMAN_OPAQUE_STRING							"Deckend"
#define	GERMAN_OPEN_IMAGE_HELP_STRING					"Lädt ein Bild von der Festplatte."
#define	GERMAN_OPEN_IMAGE_STRING						"Bild öffnen…"
#define	GERMAN_OPEN_PROJECT_HELP_STRING					"Lädt ein Projekt von der Festplatte."
#define	GERMAN_OPEN_PROJECT_STRING						"Projekt öffnen…"
#define	GERMAN_PALETTE_WINDOW_NAME_STRING				"Farbpalette"
#define	GERMAN_PASTE_AS_NEW_LAYER_HELP_STRING			"Fügt eine kürzlich in die Zwischenablage kopierte Ebene als neue Ebene ein."
#define	GERMAN_PASTE_AS_NEW_LAYER_STRING				"Als neue Ebene einfügen"
#define	GERMAN_PASTE_AS_NEW_PROJECT_HELP_STRING			"Fügt die Zwischenablage in ein neues Projekt ein."
#define	GERMAN_PASTE_AS_NEW_PROJECT_STRING				"In neues Projekt einfügen"
#define	GERMAN_QUIT_HELP_STRING							"Beendet das Programm."
#define	GERMAN_QUIT_STRING								"Programmende"
#define	GERMAN_RANDOM_STRING							"Zufall"
#define	GERMAN_RECTANGLE_STRING							"Rechteck"
#define	GERMAN_REDO_HELP_STRING							"Wiederholt die Aktion, die als letztes rückgängig gemacht wurde."
#define	GERMAN_REDO_NOT_AVAILABLE_STRING				"Wiederholen nicht möglich"
#define	GERMAN_REDO_STRING								"Wiederholen: "
#define	GERMAN_RESIZE_TO_FIT_HELP_STRING				"Stellt eine für Bildschirm und Bild optimale Fenstergröße ein."
#define	GERMAN_RESIZE_TO_FIT_STRING						"Optimale Fenstergröße"
#define	GERMAN_RIGHT_STRING								"rechts"
#define	GERMAN_ROTATE_ALL_LAYERS_HELP_STRING			"Dreht alle Ebenen."
#define	GERMAN_ROTATE_LAYER_HELP_STRING					"Dreht die aktive Ebene."
#define	GERMAN_ROTATE_STRING							"Drehen…"
#define	GERMAN_ROTATE_CW_STRING							"Drehen +90°"
#define	GERMAN_ROTATE_CCW_STRING						"Drehen -90°"
#define	GERMAN_ROTATION_STRING							"Drehung"
#define	GERMAN_ROTATING_STRING							"Drehe"
#define	GERMAN_SAVE_COLOR_SET_STRING					"Speichern…"
#define	GERMAN_SAVE_COLOR_SET_AS_STRING					"Speichern als…"
#define	GERMAN_SAVE_FORMAT_STRING						"Dateiformat"
#define GERMAN_SAVE_IMAGE_AS_HELP_STRING				"Speichert das Bild auf Festplatte."
#define	GERMAN_SAVE_IMAGE_AS_STRING						"Bild speichern als…"
#define	GERMAN_SAVE_IMAGE_HELP_STRING					"Speichert das Bild unter seinem aktuellen Namen auf Festplatte."
#define	GERMAN_SAVE_IMAGE_STRING						"Bild speichern"
#define	GERMAN_SAVE_PROJECT_AS_HELP_STRING				"Speichert das Projekt auf Festplatte."
#define	GERMAN_SAVE_PROJECT_AS_STRING					"Projekt speichern als…"
#define	GERMAN_SAVE_PROJECT_HELP_STRING					"Speichert das Projekt unter seinem aktuellen Namen auf Festplatte."
#define	GERMAN_SAVE_PROJECT_STRING						"Projekt speichern"
#define	GERMAN_SCALE_ALL_LAYERS_HELP_STRING				"Skaliert das Bild."
#define	GERMAN_SCALE_STRING								"Skalieren…"
#define	GERMAN_SELECT_CANVAS_SIZE_STRING				"Wählen Sie die gewünschte Bildgröße."
#define	GERMAN_SELECTED_COLORS_VIEW_MESSAGE1_STRING		"Hier klicken um die Farbpalette zu öffnen."
#define	GERMAN_SET_GRID_STRING							"Raster setzen"
#define	GERMAN_SET_ZOOM_LEVEL_STRING					"Zoom-Stufe setzen"
#define	GERMAN_SETTINGS_HELP_STRING						"Öffnet das Einstellungs-Fenster."
#define	GERMAN_SETTINGS_STRING							"Einstellungen…"
#define	GERMAN_SHAPE_STRING								"Form"
#define	GERMAN_SHEAR_STRING								"Neigung"
#define	GERMAN_SHORTCUTS_HELP_STRING					"Öffnet den Web-Browser mit der Beschreibung Tastaturkürzel von ArtPaint."
#define	GERMAN_SHORTCUTS_STRING							"Tastaturkürzel…"
#define	GERMAN_SHOW_BRUSH_WINDOW_HELP_STRING			"Öffnet die Stiftauswahl."
#define	GERMAN_SHOW_BRUSH_WINDOW_STRING					"Stiftauswahl"
#define	GERMAN_SHOW_LAYER_WINDOW_HELP_STRING			"Öffnet die Ebenenauswahl."
#define	GERMAN_SHOW_LAYER_WINDOW_STRING					"Ebenenauswahl"
#define	GERMAN_SHOW_PALETTE_WINDOW_HELP_STRING			"Öffnet das Farbpaletten-Fenster."
#define	GERMAN_SHOW_PALETTE_WINDOW_STRING				"Farbpalette"
#define	GERMAN_SHOW_TOOL_SETUP_WINDOW_HELP_STRING		"Öffnet das Werkzeugeinstellungs-Fenster."
#define	GERMAN_SHOW_TOOL_SETUP_WINDOW_STRING			"Werkzeugeinstellungen"
#define	GERMAN_SHOW_TOOL_WINDOW_HELP_STRING				"Öffnet das Werkzeug-Fenster."
#define	GERMAN_SHOW_TOOL_WINDOW_STRING					"Werkzeuge"
#define	GERMAN_SHRINK_SELECTION_HELP_STRING				"Verkleinert die Markierung in jede Richtung ein bißchen."
#define	GERMAN_SHRINK_SELECTION_STRING					"Markierung verkleinern"
#define	GERMAN_SIZE_STRING								"Größe"
#define	GERMAN_STORE_BRUSH_STRING						"speichern"
#define	GERMAN_SPEED_STRING								"Stärke"
#define	GERMAN_SPRAY_STRING								"Spraydose"
#define	GERMAN_SUBTRACT_AREA_STRING						"Bereich substrahieren"
#define	GERMAN_TEXT_COLOR_STRING						"Text-Farbe"
#define	GERMAN_TEXT_STRING								"Text"
#define	GERMAN_TOLERANCE_STRING							"Toleranz"
#define	GERMAN_TOOLS_STRING								"Werkzeuge"
#define	GERMAN_TOOL_SETUP_STRING						"Werkzeugeinstellungen"
#define	GERMAN_TOP_STRING								"oben"
#define	GERMAN_TRANSLATE_ALL_LAYERS_HELP_STRING			"Verschiebt alle Ebenen."
#define	GERMAN_TRANSLATE_LAYER_HELP_STRING				"Verschiebt die aktive Ebene."
#define	GERMAN_TRANSLATE_STRING							"Verschieben…"
#define	GERMAN_TRANSPARENT_STRING						"Transparent"
#define	GERMAN_TRANSPARENCY_STRING						"Transparenz"
#define	GERMAN_UNDO_HELP_STRING							"Macht die vorige Aktion rückgängig."
#define	GERMAN_UNDO_NOT_AVAILABLE_STRING				"Rückgängigmachen nicht möglich"
#define	GERMAN_UNDO_SETTINGS_STRING						"Puffer"
#define	GERMAN_UNDO_STRING								"Rückgängigmachen: "
#define	GERMAN_UNLIMITED_STRING							"Unbegrenzt"
#define	GERMAN_UNTITLED_STRING							"Ohne Name"
#define GERMAN_EMPTY_PAINT_WINDOW_STRING				"Ohne Bild"
#define	GERMAN_USE_THE_TOOL_STRING						"Drücken sie den Maus-Knopf, um dieses Werkzeug zu benutzen."
#define	GERMAN_USER_MANUAL_HELP_STRING					"Öffnet den Web-Browser mit der Dokumentation zu ArtPaint."
#define	GERMAN_USER_MANUAL_STRING						"Handbuch…"
#define	GERMAN_USING_THE_TOOL_STRING					"Wende Werkzeug an."
#define	GERMAN_VISIBLE_STRING							"Sichtbar"
#define	GERMAN_WAND_TOLERANCE_STRING					"Zauberstab"
#define	GERMAN_WIDTH_STRING								"Breite"
#define	GERMAN_WINDOW_FLOATING_STRING					"Fenster"
#define	GERMAN_WINDOW_STRING							"Fenster"
#define	GERMAN_ZOOM_IN_HELP_STRING						"Bilddarstellung vergrößern."
#define	GERMAN_ZOOM_IN_STRING							"Hineinzoomen"
#define	GERMAN_ZOOM_OUT_HELP_STRING						"Bilddarstellung verkleinern."
#define	GERMAN_ZOOM_OUT_STRING							"Herauszoomen"
#define	GERMAN_ZOOM_LEVEL_25_HELP_STRING				"Setzt die Zoom-Stufe auf 25%."
#define	GERMAN_ZOOM_LEVEL_50_HELP_STRING				"Setzt die Zoom-Stufe auf 50%."
#define	GERMAN_ZOOM_LEVEL_100_HELP_STRING				"Setzt die Zoom-Stufe auf 100%."
#define	GERMAN_ZOOM_LEVEL_200_HELP_STRING				"Setzt die Zoom-Stufe auf 200%."
#define	GERMAN_ZOOM_LEVEL_400_HELP_STRING				"Setzt die Zoom-Stufe auf 400%."
#define	GERMAN_ZOOM_LEVEL_800_HELP_STRING				"Setzt die Zoom-Stufe auf 800%."
#define GERMAN_BRUSH_STRING								"Stift"
#define GERMAN_COLOR_SET_STRING							"Farbpalette"
#define	GERMAN_ALPHA_STRING								"Alpha"
#define	GERMAN_BLUE_STRING								"Blau"
#define	GERMAN_GREEN_STRING								"Grün"
#define	GERMAN_RED_STRING								"Rot"
#define	GERMAN_COLOR_MODEL_STRING						"Farbmodell"


// Here begin the tool name and help-string definitions.
#define	GERMAN_AIR_BRUSH_TOOL_NAME_STRING				"Spraydose"
#define	GERMAN_BLUR_TOOL_NAME_STRING					"Unschärfe"
#define	GERMAN_BRUSH_TOOL_NAME_STRING					"Stift"
#define	GERMAN_COLOR_SELECTOR_TOOL_NAME_STRING			"Farbselektion"
#define	GERMAN_ELLIPSE_TOOL_NAME_STRING					"Ellipse"
#define	GERMAN_ERASER_TOOL_NAME_STRING					"Radiergummi"
#define	GERMAN_FILL_TOOL_NAME_STRING					"Füllen"
#define	GERMAN_FREE_LINE_TOOL_NAME_STRING				"Freihandlinie"
#define	GERMAN_HAIRY_BRUSH_TOOL_NAME_STRING				"Pinsel"
#define	GERMAN_RECTANGLE_TOOL_NAME_STRING				"Rechteck"
#define	GERMAN_SELECTOR_TOOL_NAME_STRING				"Markierungen"
#define	GERMAN_STRAIGHT_LINE_TOOL_NAME_STRING			"Linie"
#define	GERMAN_TEXT_TOOL_NAME_STRING					"Text"
#define	GERMAN_TRANSPARENCY_TOOL_NAME_STRING			"Transparenz"

#define	GERMAN_AIR_BRUSH_TOOL_READY_STRING				"Drücken Sie die Maustaste um mit der Spraydose zu malen."
#define	GERMAN_BLUR_TOOL_READY_STRING					"Drücken Sie die Maustaste um das Bild unscharf zu machen."
#define	GERMAN_BRUSH_TOOL_READY_STRING					"Drücken Sie die Maustaste um mit dem Stift zu zeichnen."
#define	GERMAN_COLOR_SELECTOR_TOOL_READY_STRING			"Drücken Sie die Maustaste um eine Farbe herauszupicken."
#define	GERMAN_ELLIPSE_TOOL_READY_STRING				"Drücken Sie die Maustaste um eine Ellipse zu zeichen."
#define	GERMAN_ERASER_TOOL_READY_STRING					"Drücken Sie die Maustaste um den Radierer."
#define	GERMAN_FILL_TOOL_READY_STRING					"Drücken Sie die Maustaste um einen Bereich zu füllen."
#define	GERMAN_FREE_LINE_TOOL_READY_STRING				"Drücken Sie die Maustaste um eine Freihand-Linie zu zeichnen."
#define	GERMAN_HAIRY_BRUSH_TOOL_READY_STRING			"Drücken Sie die Maustaste um mit dem Pinsel zu malen."
#define	GERMAN_RECTANGLE_TOOL_READY_STRING				"Drücken Sie die Maustaste um ein Rechteck zu zeichen."
#define	GERMAN_SELECTOR_TOOL_READY_STRING				"Drücken Sie die Maustaste um eine Markierung zu machen."
#define	GERMAN_STRAIGHT_LINE_TOOL_READY_STRING			"Drücken Sie die Maustaste um eine Linie zu zeichen."
#define	GERMAN_TEXT_TOOL_READY_STRING					"Drücken Sie die Maustaste um Text in das Bild einzufügen."
#define	GERMAN_TRANSPARENCY_TOOL_READY_STRING			"Drücken Sie die Maustaste um die Transparenz der Ebene einzustellen."
#define GERMAN_CLICK_TO_SELECT_COLOR_STRING				"Drücken Sie die Maustaste um eine Farbe auszuwählen."

#define	GERMAN_DO_CHANGE_TRANSPARENCY_HELP_STRING		"Stellen sie die gewünschte Transparenz ein."
#define	GERMAN_DO_CROP_HELP_STRING						"Zum Ändern der Ränder den Rahmen an den Griffen verschieben oder die Werte direkt eingeben."
#define	GERMAN_DO_ROTATE_HELP_STRING					"Benutzen sie den Hauptmaustaste zum drehen, den Nebenmaustaste um den Mittelpunkt der Drehung einzustellen."
#define	GERMAN_DO_TRANSLATE_HELP_STRING					"Ziehen sie das Bild an die gewünschte Position."
#define	GERMAN_DO_SCALE_HELP_STRING						"Zum Skalieren Maustaste im Bild ziehen oder die Werte direkt eingeben."

#define	GERMAN_AIR_BRUSH_TOOL_IN_USE_STRING				"Sprühen mit der Spraydose."
#define	GERMAN_BLUR_TOOL_IN_USE_STRING					"Bild unscharf machen."
#define	GERMAN_BRUSH_TOOL_IN_USE_STRING					"Zeichnen mit dem Stift."
#define	GERMAN_COLOR_SELECTOR_TOOL_IN_USE_STRING		"Farbwert aus dem Bild auslesen."
#define	GERMAN_ELLIPSE_TOOL_IN_USE_STRING				"Zeichnen einer Ellipse."
#define	GERMAN_ERASER_TOOL_IN_USE_STRING				"Radieren."
#define	GERMAN_FILL_TOOL_IN_USE_STRING					"Füllen."
#define	GERMAN_FREE_LINE_TOOL_IN_USE_STRING				"Freihandlinie zeichnen."
#define	GERMAN_HAIRY_BRUSH_TOOL_IN_USE_STRING			"Malen mit dem Pinsel."
#define	GERMAN_RECTANGLE_TOOL_IN_USE_STRING				"Rechteck zeichnen."
#define	GERMAN_SELECTOR_TOOL_IN_USE_STRING				"Markieren."
#define	GERMAN_STRAIGHT_LINE_TOOL_IN_USE_STRING			"Linie zeichnen."
#define	GERMAN_TRANSPARENCY_TOOL_IN_USE_STRING			"Transparenz der Ebene einstellen."
#define	GERMAN_TEXT_TOOL_IN_USE_STRING					"Ziehen sie den Text mit der Maus an die richtige Position."

#define	GERMAN_CONFIRM_QUIT_STRING						"Beenden bestätigen"
#define GERMAN_CROSS_HAIR_CURSOR_STRING					"nur Fadenkreuz"
#define GERMAN_CURSOR_STRING							"Zeiger"
#define	GERMAN_DO_NOT_SAVE_STRING						"Verwerfen"
#define	GERMAN_MISCELLANEOUS_STRING						"Verschiedenes"
#define	GERMAN_RECENT_IMAGES_STRING						"Zuletzt geöffnete Bilder"
#define	GERMAN_RECENT_PROJECTS_STRING					"Zuletzt geöffnete Projekte"
#define	GERMAN_SAVE_STRING								"Speichern"
#define	GERMAN_SAVE_CHANGES_STRING						"Sie haben %d Veränderungen seit dem letzten Speichern des Projektes vorgenommen Wollen Sie die Veränderungen speichern?"
#define GERMAN_TOOL_CURSOR_STRING						"mit Werkzeugsymbol"

#define GERMAN_MEMORY_ALERT_1_STRING					"Der freie Speicher reicht leider nicht zum Hinzufügen einer weiteren Ebene."\
														"Sie können Speicher freisetzen, indem sie die Puffer-Option ausschalten oder andere offene Bilder schliessen. "\
														"Auch sollten sie das Bild jetzt zu speichern, da Speichermangel dazu führen kann, daß es später "\
														"schwierig oder gar unmöglich wird, dies zu tun. Wir bitten diese Unannehmlichkeit zu entschuldigen!"

#define	GERMAN_MEMORY_ALERT_2_STRING					"Nicht genügend Speicher für den gewünschten Effekt frei. Sie können andere Bilder schliessen und es sodann nochmal versuchen. "\
														"Es kann auch helfen die Puffer-Tiefe zu reduzieren oder den Puffer ganz abzuschalten. "\
														"Ebenso kann das Schliessen anderer eventuell laufender Applikationen den freien Speicher erhöhen. "\
														"Auch sollten sie das Bild jetzt zu speichern, da Speichermangel dazu führen kann, daß es später "\
														"schwierig oder gar unmöglich wird, dies zu tun. Wir bitten diese Unannehmlichkeit zu entschuldigen!"

#define	GERMAN_MEMORY_ALERT_3_STRING					"Nicht genügend Speicher frei, um den Effekt zu beenden. Sie können andere Bilder schliessen und es sodann nochmal versuchen. "\
														"Auch kann es helfen die Puffer-Tiefe zu reduzieren oder den Puffer ganz abzuschalten. "\
														"Auch das Schliessen anderer eventuell laufender Applikationen kann den freien Speicher erhöhen. "\
														"Auch sollten sie das Bild jetzt zu speichern, da Speichermangel dazu führen kann, daß es später "\
														"schwierig oder gar unmöglich wird, dies zu tun. Wir bitten diese Unannehmlichkeit zu entschuldigen!"

#define	GERMAN_ABOUT_ARTPAINT_STRING					"Über ArtPaint…"
#define	GERMAN_ABOUT_HELP_STRING						"Zeigt in einem Fenster Informationen über dieses Programm."

#define	GERMAN_ABOUT_1_TEXT_STRING						"ArtPaint ist ein Mal- und Bildbearbeitungsprogramm für BeOS."\
														" Es eignet sich für einen weiten Bereich von Web-Grafiken bis hin zu Bildverarbeitung."\
														" Werfen Sie einen Blick in die Anleitung. Dort finden Sie eine Einleitung."

#define	GERMAN_ABOUT_3_TEXT_STRING						"Neue Versionen finden sie hier:"

#define	GERMAN_ABOUT_4_TEXT_STRING						"Vorschläge und Fragen betreffs dieses Programmes schicken Sie bitte an:"

#define	GERMAN_ABOUT_5_TEXT_STRING						"Programmierung & Design"
#define	GERMAN_ABOUT_6_TEXT_STRING						"Englische Dokumentation"
#define	GERMAN_ABOUT_7_TEXT_STRING						"Deutsche Lokalisierung"
#define	GERMAN_ABOUT_8_TEXT_STRING						"Besonderen Dank an"

#define	GERMAN_UNSUPPORTED_FILE_TYPE_STRING				"Die Datei %s hat ein nicht unterstützes Format. Wenn möglich sollten Sie einen Translator dafür installieren."
#define	GERMAN_RELEASE_DATE_STRING						"Veröffentlicht am: %s"

#endif
